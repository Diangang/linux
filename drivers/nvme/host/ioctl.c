// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2011-2014, Intel Corporation.
 * Copyright (c) 2017-2021 Christoph Hellwig.
 */
#include <linux/blk-integrity.h>
#include <linux/compat.h>
#include <linux/ptrace.h>	/* for force_successful_syscall_return */
#include <linux/nvme_ioctl.h>
#include "nvme.h"

enum {
	NVME_IOCTL_VEC		= (1 << 0),
	NVME_IOCTL_PARTITION	= (1 << 1),
};

static bool nvme_cmd_allowed(struct nvme_ns *ns, struct nvme_command *c,
		unsigned int flags, bool open_for_write)
{
	u32 effects;

	/*
	 * Do not allow unprivileged passthrough on partitions, as that allows an
	 * escape from the containment of the partition.
	 */
	if (flags & NVME_IOCTL_PARTITION)
		goto admin;

	/*
	 * Do not allow unprivileged processes to send vendor specific or fabrics
	 * commands as we can't be sure about their effects.
	 */
	if (c->common.opcode >= nvme_cmd_vendor_start ||
	    c->common.opcode == nvme_fabrics_command)
		goto admin;

	/*
	 * Do not allow unprivileged passthrough of admin commands except
	 * for a subset of identify commands that contain information required
	 * to form proper I/O commands in userspace and do not expose any
	 * potentially sensitive information.
	 */
	if (!ns) {
		if (c->common.opcode == nvme_admin_identify) {
			switch (c->identify.cns) {
			case NVME_ID_CNS_NS:
			case NVME_ID_CNS_CS_NS:
			case NVME_ID_CNS_NS_CS_INDEP:
			case NVME_ID_CNS_CS_CTRL:
			case NVME_ID_CNS_CTRL:
				return true;
			}
		}
		goto admin;
	}

	/*
	 * Check if the controller provides a Commands Supported and Effects log
	 * and marks this command as supported.  If not reject unprivileged
	 * passthrough.
	 */
	effects = nvme_command_effects(ns->ctrl, ns, c->common.opcode);
	if (!(effects & NVME_CMD_EFFECTS_CSUPP))
		goto admin;

	/*
	 * Don't allow passthrough for command that have intrusive (or unknown)
	 * effects.
	 */
	if (effects & ~(NVME_CMD_EFFECTS_CSUPP | NVME_CMD_EFFECTS_LBCC |
			NVME_CMD_EFFECTS_UUID_SEL |
			NVME_CMD_EFFECTS_SCOPE_MASK))
		goto admin;

	/*
	 * Only allow I/O commands that transfer data to the controller or that
	 * change the logical block contents if the file descriptor is open for
	 * writing.
	 */
	if ((nvme_is_write(c) || (effects & NVME_CMD_EFFECTS_LBCC)) &&
	    !open_for_write)
		goto admin;

	return true;
admin:
	return capable(CAP_SYS_ADMIN);
}

/*
 * Convert integer values from ioctl structures to user pointers, silently
 * ignoring the upper bits in the compat case to match behaviour of 32-bit
 * kernels.
 */
static void __user *nvme_to_user_ptr(uintptr_t ptrval)
{
	if (in_compat_syscall())
		ptrval = (compat_uptr_t)ptrval;
	return (void __user *)ptrval;
}

static struct request *nvme_alloc_user_request(struct request_queue *q,
		struct nvme_command *cmd, blk_opf_t rq_flags,
		blk_mq_req_flags_t blk_flags)
{
	struct request *req;

	req = blk_mq_alloc_request(q, nvme_req_op(cmd) | rq_flags, blk_flags);
	if (IS_ERR(req))
		return req;
	nvme_init_request(req, cmd);
	nvme_req(req)->flags |= NVME_REQ_USERCMD;
	return req;
}

static int nvme_map_user_request(struct request *req, u64 ubuffer,
		unsigned bufflen, void __user *meta_buffer, unsigned meta_len,
		struct iov_iter *iter, unsigned int flags)
{
	struct request_queue *q = req->q;
	struct nvme_ns *ns = q->queuedata;
	struct block_device *bdev = ns ? ns->disk->part0 : NULL;
	bool supports_metadata = bdev && blk_get_integrity(bdev->bd_disk);
	struct nvme_ctrl *ctrl = nvme_req(req)->ctrl;
	bool has_metadata = meta_buffer && meta_len;
	struct bio *bio = NULL;
	int ret;

	if (!nvme_ctrl_sgl_supported(ctrl))
		dev_warn_once(ctrl->device, "using unchecked data buffer\n");
	if (has_metadata) {
		if (!supports_metadata)
			return -EINVAL;

		if (!nvme_ctrl_meta_sgl_supported(ctrl))
			dev_warn_once(ctrl->device,
				      "using unchecked metadata buffer\n");
	}

	if (iter)
		ret = blk_rq_map_user_iov(q, req, NULL, iter, GFP_KERNEL);
	else
		ret = blk_rq_map_user_io(req, NULL, nvme_to_user_ptr(ubuffer),
				bufflen, GFP_KERNEL, flags & NVME_IOCTL_VEC, 0,
				0, rq_data_dir(req));
	if (ret)
		return ret;

	if (has_metadata) {
		ret = blk_rq_integrity_map_user(req, meta_buffer, meta_len);
		if (ret)
			goto out_unmap;
	}

	return ret;

out_unmap:
	if (bio)
		blk_rq_unmap_user(bio);
	return ret;
}

static int nvme_submit_user_cmd(struct request_queue *q,
		struct nvme_command *cmd, u64 ubuffer, unsigned bufflen,
		void __user *meta_buffer, unsigned meta_len,
		u64 *result, unsigned timeout, unsigned int flags)
{
	struct nvme_ns *ns = q->queuedata;
	struct nvme_ctrl *ctrl;
	struct request *req;
	struct bio *bio;
	u32 effects;
	int ret;

	req = nvme_alloc_user_request(q, cmd, 0, 0);
	if (IS_ERR(req))
		return PTR_ERR(req);

	req->timeout = timeout;
	if (ubuffer && bufflen) {
		ret = nvme_map_user_request(req, ubuffer, bufflen, meta_buffer,
				meta_len, NULL, flags);
		if (ret)
			goto out_free_req;
	}

	bio = req->bio;
	ctrl = nvme_req(req)->ctrl;

	effects = nvme_passthru_start(ctrl, ns, cmd->common.opcode);
	ret = nvme_execute_rq(req, false);
	if (result)
		*result = le64_to_cpu(nvme_req(req)->result.u64);
	if (bio)
		blk_rq_unmap_user(bio);
	blk_mq_free_request(req);

	if (effects)
		nvme_passthru_end(ctrl, ns, effects, cmd, ret);
	return ret;

out_free_req:
	blk_mq_free_request(req);
	return ret;
}

static int nvme_submit_io(struct nvme_ns *ns, struct nvme_user_io __user *uio)
{
	struct nvme_user_io io;
	struct nvme_command c;
	unsigned length, meta_len;
	void __user *metadata;

	if (copy_from_user(&io, uio, sizeof(io)))
		return -EFAULT;
	if (io.flags)
		return -EINVAL;

	switch (io.opcode) {
	case nvme_cmd_write:
	case nvme_cmd_read:
	case nvme_cmd_compare:
		break;
	default:
		return -EINVAL;
	}

	length = (io.nblocks + 1) << ns->head->lba_shift;

	if ((io.control & NVME_RW_PRINFO_PRACT) &&
	    (ns->head->ms == ns->head->pi_size)) {
		/*
		 * Protection information is stripped/inserted by the
		 * controller.
		 */
		if (nvme_to_user_ptr(io.metadata))
			return -EINVAL;
		meta_len = 0;
		metadata = NULL;
	} else {
		meta_len = (io.nblocks + 1) * ns->head->ms;
		metadata = nvme_to_user_ptr(io.metadata);
	}

	if (ns->head->features & NVME_NS_EXT_LBAS) {
		length += meta_len;
		meta_len = 0;
	} else if (meta_len) {
		if ((io.metadata & 3) || !io.metadata)
			return -EINVAL;
	}

	memset(&c, 0, sizeof(c));
	c.rw.opcode = io.opcode;
	c.rw.flags = io.flags;
	c.rw.nsid = cpu_to_le32(ns->head->ns_id);
	c.rw.slba = cpu_to_le64(io.slba);
	c.rw.length = cpu_to_le16(io.nblocks);
	c.rw.control = cpu_to_le16(io.control);
	c.rw.dsmgmt = cpu_to_le32(io.dsmgmt);
	c.rw.reftag = cpu_to_le32(io.reftag);
	c.rw.lbat = cpu_to_le16(io.apptag);
	c.rw.lbatm = cpu_to_le16(io.appmask);

	return nvme_submit_user_cmd(ns->queue, &c, io.addr, length, metadata,
			meta_len, NULL, 0, 0);
}

static bool nvme_validate_passthru_nsid(struct nvme_ctrl *ctrl,
					struct nvme_ns *ns, __u32 nsid)
{
	if (ns && nsid != ns->head->ns_id) {
		dev_err(ctrl->device,
			"%s: nsid (%u) in cmd does not match nsid (%u) of namespace\n",
			current->comm, nsid, ns->head->ns_id);
		return false;
	}

	return true;
}

static int nvme_user_cmd(struct nvme_ctrl *ctrl, struct nvme_ns *ns,
		struct nvme_passthru_cmd __user *ucmd, unsigned int flags,
		bool open_for_write)
{
	struct nvme_passthru_cmd cmd;
	struct nvme_command c;
	unsigned timeout = 0;
	u64 result;
	int status;

	if (copy_from_user(&cmd, ucmd, sizeof(cmd)))
		return -EFAULT;
	if (cmd.flags)
		return -EINVAL;
	if (!nvme_validate_passthru_nsid(ctrl, ns, cmd.nsid))
		return -EINVAL;

	memset(&c, 0, sizeof(c));
	c.common.opcode = cmd.opcode;
	c.common.flags = cmd.flags;
	c.common.nsid = cpu_to_le32(cmd.nsid);
	c.common.cdw2[0] = cpu_to_le32(cmd.cdw2);
	c.common.cdw2[1] = cpu_to_le32(cmd.cdw3);
	c.common.cdw10 = cpu_to_le32(cmd.cdw10);
	c.common.cdw11 = cpu_to_le32(cmd.cdw11);
	c.common.cdw12 = cpu_to_le32(cmd.cdw12);
	c.common.cdw13 = cpu_to_le32(cmd.cdw13);
	c.common.cdw14 = cpu_to_le32(cmd.cdw14);
	c.common.cdw15 = cpu_to_le32(cmd.cdw15);

	if (!nvme_cmd_allowed(ns, &c, 0, open_for_write))
		return -EACCES;

	if (cmd.timeout_ms)
		timeout = msecs_to_jiffies(cmd.timeout_ms);

	status = nvme_submit_user_cmd(ns ? ns->queue : ctrl->admin_q, &c,
			cmd.addr, cmd.data_len, nvme_to_user_ptr(cmd.metadata),
			cmd.metadata_len, &result, timeout, 0);

	if (status >= 0) {
		if (put_user(result, &ucmd->result))
			return -EFAULT;
	}

	return status;
}

static int nvme_user_cmd64(struct nvme_ctrl *ctrl, struct nvme_ns *ns,
		struct nvme_passthru_cmd64 __user *ucmd, unsigned int flags,
		bool open_for_write)
{
	struct nvme_passthru_cmd64 cmd;
	struct nvme_command c;
	unsigned timeout = 0;
	int status;

	if (copy_from_user(&cmd, ucmd, sizeof(cmd)))
		return -EFAULT;
	if (cmd.flags)
		return -EINVAL;
	if (!nvme_validate_passthru_nsid(ctrl, ns, cmd.nsid))
		return -EINVAL;

	memset(&c, 0, sizeof(c));
	c.common.opcode = cmd.opcode;
	c.common.flags = cmd.flags;
	c.common.nsid = cpu_to_le32(cmd.nsid);
	c.common.cdw2[0] = cpu_to_le32(cmd.cdw2);
	c.common.cdw2[1] = cpu_to_le32(cmd.cdw3);
	c.common.cdw10 = cpu_to_le32(cmd.cdw10);
	c.common.cdw11 = cpu_to_le32(cmd.cdw11);
	c.common.cdw12 = cpu_to_le32(cmd.cdw12);
	c.common.cdw13 = cpu_to_le32(cmd.cdw13);
	c.common.cdw14 = cpu_to_le32(cmd.cdw14);
	c.common.cdw15 = cpu_to_le32(cmd.cdw15);

	if (!nvme_cmd_allowed(ns, &c, flags, open_for_write))
		return -EACCES;

	if (cmd.timeout_ms)
		timeout = msecs_to_jiffies(cmd.timeout_ms);

	status = nvme_submit_user_cmd(ns ? ns->queue : ctrl->admin_q, &c,
			cmd.addr, cmd.data_len, nvme_to_user_ptr(cmd.metadata),
			cmd.metadata_len, &cmd.result, timeout, flags);

	if (status >= 0) {
		if (put_user(cmd.result, &ucmd->result))
			return -EFAULT;
	}

	return status;
}

static bool is_ctrl_ioctl(unsigned int cmd)
{
	if (cmd == NVME_IOCTL_ADMIN_CMD || cmd == NVME_IOCTL_ADMIN64_CMD)
		return true;
	if (is_sed_ioctl(cmd))
		return true;
	return false;
}

static int nvme_ctrl_ioctl(struct nvme_ctrl *ctrl, unsigned int cmd,
		void __user *argp, bool open_for_write)
{
	switch (cmd) {
	case NVME_IOCTL_ADMIN_CMD:
		return nvme_user_cmd(ctrl, NULL, argp, 0, open_for_write);
	case NVME_IOCTL_ADMIN64_CMD:
		return nvme_user_cmd64(ctrl, NULL, argp, 0, open_for_write);
	default:
		return sed_ioctl(ctrl->opal_dev, cmd, argp);
	}
}

#ifdef COMPAT_FOR_U64_ALIGNMENT
struct nvme_user_io32 {
	__u8	opcode;
	__u8	flags;
	__u16	control;
	__u16	nblocks;
	__u16	rsvd;
	__u64	metadata;
	__u64	addr;
	__u64	slba;
	__u32	dsmgmt;
	__u32	reftag;
	__u16	apptag;
	__u16	appmask;
} __attribute__((__packed__));
#define NVME_IOCTL_SUBMIT_IO32	_IOW('N', 0x42, struct nvme_user_io32)
#endif /* COMPAT_FOR_U64_ALIGNMENT */

static int nvme_ns_ioctl(struct nvme_ns *ns, unsigned int cmd,
		void __user *argp, unsigned int flags, bool open_for_write)
{
	switch (cmd) {
	case NVME_IOCTL_ID:
		force_successful_syscall_return();
		return ns->head->ns_id;
	case NVME_IOCTL_IO_CMD:
		return nvme_user_cmd(ns->ctrl, ns, argp, flags, open_for_write);
	/*
	 * struct nvme_user_io can have different padding on some 32-bit ABIs.
	 * Just accept the compat version as all fields that are used are the
	 * same size and at the same offset.
	 */
#ifdef COMPAT_FOR_U64_ALIGNMENT
	case NVME_IOCTL_SUBMIT_IO32:
#endif
	case NVME_IOCTL_SUBMIT_IO:
		return nvme_submit_io(ns, argp);
	case NVME_IOCTL_IO64_CMD_VEC:
		flags |= NVME_IOCTL_VEC;
		fallthrough;
	case NVME_IOCTL_IO64_CMD:
		return nvme_user_cmd64(ns->ctrl, ns, argp, flags,
				       open_for_write);
	default:
		return -ENOTTY;
	}
}

int nvme_ioctl(struct block_device *bdev, blk_mode_t mode,
		unsigned int cmd, unsigned long arg)
{
	struct nvme_ns *ns = bdev->bd_disk->private_data;
	bool open_for_write = mode & BLK_OPEN_WRITE;
	void __user *argp = (void __user *)arg;
	unsigned int flags = 0;

	if (bdev_is_partition(bdev))
		flags |= NVME_IOCTL_PARTITION;

	if (is_ctrl_ioctl(cmd))
		return nvme_ctrl_ioctl(ns->ctrl, cmd, argp, open_for_write);
	return nvme_ns_ioctl(ns, cmd, argp, flags, open_for_write);
}

long nvme_ns_chr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct nvme_ns *ns =
		container_of(file_inode(file)->i_cdev, struct nvme_ns, cdev);
	bool open_for_write = file->f_mode & FMODE_WRITE;
	void __user *argp = (void __user *)arg;

	if (is_ctrl_ioctl(cmd))
		return nvme_ctrl_ioctl(ns->ctrl, cmd, argp, open_for_write);
	return nvme_ns_ioctl(ns, cmd, argp, 0, open_for_write);
}

static int nvme_dev_user_cmd(struct nvme_ctrl *ctrl, void __user *argp,
		bool open_for_write)
{
	struct nvme_ns *ns;
	int ret, srcu_idx;

	srcu_idx = srcu_read_lock(&ctrl->srcu);
	if (list_empty(&ctrl->namespaces)) {
		ret = -ENOTTY;
		goto out_unlock;
	}

	ns = list_first_or_null_rcu(&ctrl->namespaces, struct nvme_ns, list);
	if (ns != list_last_entry(&ctrl->namespaces, struct nvme_ns, list)) {
		dev_warn(ctrl->device,
			"NVME_IOCTL_IO_CMD not supported when multiple namespaces present!\n");
		ret = -EINVAL;
		goto out_unlock;
	}

	dev_warn(ctrl->device,
		"using deprecated NVME_IOCTL_IO_CMD ioctl on the char device!\n");
	if (!nvme_get_ns(ns)) {
		ret = -ENXIO;
		goto out_unlock;
	}
	srcu_read_unlock(&ctrl->srcu, srcu_idx);

	ret = nvme_user_cmd(ctrl, ns, argp, 0, open_for_write);
	nvme_put_ns(ns);
	return ret;

out_unlock:
	srcu_read_unlock(&ctrl->srcu, srcu_idx);
	return ret;
}

long nvme_dev_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	bool open_for_write = file->f_mode & FMODE_WRITE;
	struct nvme_ctrl *ctrl = file->private_data;
	void __user *argp = (void __user *)arg;

	switch (cmd) {
	case NVME_IOCTL_ADMIN_CMD:
		return nvme_user_cmd(ctrl, NULL, argp, 0, open_for_write);
	case NVME_IOCTL_ADMIN64_CMD:
		return nvme_user_cmd64(ctrl, NULL, argp, 0, open_for_write);
	case NVME_IOCTL_IO_CMD:
		return nvme_dev_user_cmd(ctrl, argp, open_for_write);
	case NVME_IOCTL_RESET:
		if (!capable(CAP_SYS_ADMIN))
			return -EACCES;
		dev_warn(ctrl->device, "resetting controller\n");
		return nvme_reset_ctrl_sync(ctrl);
	case NVME_IOCTL_SUBSYS_RESET:
		if (!capable(CAP_SYS_ADMIN))
			return -EACCES;
		return nvme_reset_subsystem(ctrl);
	case NVME_IOCTL_RESCAN:
		if (!capable(CAP_SYS_ADMIN))
			return -EACCES;
		nvme_queue_scan(ctrl);
		return 0;
	default:
		return -ENOTTY;
	}
}
