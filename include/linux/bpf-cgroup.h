/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _BPF_CGROUP_H
#define _BPF_CGROUP_H

#include <linux/bpf.h>
#include <linux/bpf-cgroup-defs.h>
#include <linux/errno.h>
#include <linux/jump_label.h>
#include <linux/percpu.h>
#include <linux/rbtree.h>
#include <net/sock.h>
#include <uapi/linux/bpf.h>

struct sock;
struct sockaddr;
struct cgroup;
struct sk_buff;
struct bpf_map;
struct bpf_prog;
struct bpf_sock_ops_kern;
struct bpf_cgroup_storage;
struct ctl_table;
struct ctl_table_header;
struct task_struct;

unsigned int __cgroup_bpf_run_lsm_sock(const void *ctx,
				       const struct bpf_insn *insn);
unsigned int __cgroup_bpf_run_lsm_socket(const void *ctx,
					 const struct bpf_insn *insn);
unsigned int __cgroup_bpf_run_lsm_current(const void *ctx,
					  const struct bpf_insn *insn);


static inline void cgroup_bpf_lifetime_notifier_init(void)
{
	return;
}

static inline int cgroup_bpf_prog_attach(const union bpf_attr *attr,
					 enum bpf_prog_type ptype,
					 struct bpf_prog *prog)
{
	return -EINVAL;
}

static inline int cgroup_bpf_prog_detach(const union bpf_attr *attr,
					 enum bpf_prog_type ptype)
{
	return -EINVAL;
}

static inline int cgroup_bpf_link_attach(const union bpf_attr *attr,
					 struct bpf_prog *prog)
{
	return -EINVAL;
}

static inline int cgroup_bpf_prog_query(const union bpf_attr *attr,
					union bpf_attr __user *uattr)
{
	return -EINVAL;
}

static inline const struct bpf_func_proto *
cgroup_common_func_proto(enum bpf_func_id func_id, const struct bpf_prog *prog)
{
	return NULL;
}

static inline int bpf_cgroup_storage_assign(struct bpf_prog_aux *aux,
					    struct bpf_map *map) { return 0; }
static inline struct bpf_cgroup_storage *bpf_cgroup_storage_alloc(
	struct bpf_prog *prog, enum bpf_cgroup_storage_type stype) { return NULL; }
static inline void bpf_cgroup_storage_free(
	struct bpf_cgroup_storage *storage) {}
static inline int bpf_percpu_cgroup_storage_copy(struct bpf_map *map, void *key,
						 void *value, u64 flags) {
	return 0;
}
static inline int bpf_percpu_cgroup_storage_update(struct bpf_map *map,
					void *key, void *value, u64 flags) {
	return 0;
}

#define cgroup_bpf_enabled(atype) (0)
#define BPF_CGROUP_RUN_SA_PROG_LOCK(sk, uaddr, uaddrlen, atype, t_ctx) ({ 0; })
#define BPF_CGROUP_RUN_SA_PROG(sk, uaddr, uaddrlen, atype) ({ 0; })
#define BPF_CGROUP_PRE_CONNECT_ENABLED(sk) (0)
#define BPF_CGROUP_RUN_PROG_INET_INGRESS(sk,skb) ({ 0; })
#define BPF_CGROUP_RUN_PROG_INET_EGRESS(sk,skb) ({ 0; })
#define BPF_CGROUP_RUN_PROG_INET_SOCK(sk) ({ 0; })
#define BPF_CGROUP_RUN_PROG_INET_SOCK_RELEASE(sk) ({ 0; })
#define BPF_CGROUP_RUN_PROG_INET_BIND_LOCK(sk, uaddr, uaddrlen, atype, flags) ({ 0; })
#define BPF_CGROUP_RUN_PROG_INET4_POST_BIND(sk) ({ 0; })
#define BPF_CGROUP_RUN_PROG_INET6_POST_BIND(sk) ({ 0; })
#define BPF_CGROUP_RUN_PROG_INET4_CONNECT(sk, uaddr, uaddrlen) ({ 0; })
#define BPF_CGROUP_RUN_PROG_INET4_CONNECT_LOCK(sk, uaddr, uaddrlen) ({ 0; })
#define BPF_CGROUP_RUN_PROG_INET6_CONNECT(sk, uaddr, uaddrlen) ({ 0; })
#define BPF_CGROUP_RUN_PROG_INET6_CONNECT_LOCK(sk, uaddr, uaddrlen) ({ 0; })
#define BPF_CGROUP_RUN_PROG_UNIX_CONNECT_LOCK(sk, uaddr, uaddrlen) ({ 0; })
#define BPF_CGROUP_RUN_PROG_UDP4_SENDMSG_LOCK(sk, uaddr, uaddrlen, t_ctx) ({ 0; })
#define BPF_CGROUP_RUN_PROG_UDP6_SENDMSG_LOCK(sk, uaddr, uaddrlen, t_ctx) ({ 0; })
#define BPF_CGROUP_RUN_PROG_UNIX_SENDMSG_LOCK(sk, uaddr, uaddrlen, t_ctx) ({ 0; })
#define BPF_CGROUP_RUN_PROG_UDP4_RECVMSG_LOCK(sk, uaddr, uaddrlen) ({ 0; })
#define BPF_CGROUP_RUN_PROG_UDP6_RECVMSG_LOCK(sk, uaddr, uaddrlen) ({ 0; })
#define BPF_CGROUP_RUN_PROG_UNIX_RECVMSG_LOCK(sk, uaddr, uaddrlen) ({ 0; })
#define BPF_CGROUP_RUN_PROG_SOCK_OPS(sock_ops) ({ 0; })
#define BPF_CGROUP_RUN_PROG_DEVICE_CGROUP(atype, major, minor, access) ({ 0; })
#define BPF_CGROUP_RUN_PROG_SYSCTL(head,table,write,buf,count,pos) ({ 0; })
#define BPF_CGROUP_RUN_PROG_GETSOCKOPT(sock, level, optname, optval, \
				       optlen, max_optlen, retval) ({ retval; })
#define BPF_CGROUP_RUN_PROG_GETSOCKOPT_KERN(sock, level, optname, optval, \
					    optlen, retval) ({ retval; })
#define BPF_CGROUP_RUN_PROG_SETSOCKOPT(sock, level, optname, optval, optlen, \
				       kernel_optval) ({ 0; })

#endif /* _BPF_CGROUP_H */
