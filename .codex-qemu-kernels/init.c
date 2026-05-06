#include <errno.h>
#include <fcntl.h>
#include <linux/reboot.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void say(const char *msg)
{
	write(1, msg, strlen(msg));
}

static void die(const char *where)
{
	perror(where);
	sync();
	reboot(LINUX_REBOOT_CMD_POWER_OFF);
	for (;;)
		pause();
}

static void mkdir_if_needed(const char *path)
{
	if (mkdir(path, 0755) && errno != EEXIST)
		die(path);
}

static void wait_block_device(const char *path)
{
	struct stat st;
	int i;

	for (i = 0; i < 60; i++) {
		if (!stat(path, &st) && S_ISBLK(st.st_mode))
			return;
		sleep(1);
	}

	errno = ENOENT;
	die(path);
}

static void write_full(int fd, const void *buf, size_t len)
{
	const char *p = buf;

	while (len) {
		ssize_t n = write(fd, p, len);

		if (n < 0)
			die("write");
		p += n;
		len -= n;
	}
}

static void read_full(int fd, void *buf, size_t len)
{
	char *p = buf;

	while (len) {
		ssize_t n = read(fd, p, len);

		if (n < 0)
			die("read");
		if (!n) {
			errno = EIO;
			die("short read");
		}
		p += n;
		len -= n;
	}
}

static void exercise_mount(const char *dev, const char *mnt, const char *tag)
{
	char path[128];
	char wbuf[4096];
	char rbuf[4096];
	int fd, i, loops;

	wait_block_device(dev);
	mkdir_if_needed(mnt);
	if (mount(dev, mnt, "minix", 0, "") < 0)
		die(dev);

	snprintf(path, sizeof(path), "%s/test.bin", mnt);
	fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0644);
	if (fd < 0)
		die(path);

	memset(wbuf, tag[0], sizeof(wbuf));
	loops = 1024;
	for (i = 0; i < loops; i++)
		write_full(fd, wbuf, sizeof(wbuf));
	if (fsync(fd) < 0)
		die("fsync");
	if (lseek(fd, 0, SEEK_SET) < 0)
		die("lseek");
	for (i = 0; i < loops; i++) {
		read_full(fd, rbuf, sizeof(rbuf));
		if (memcmp(wbuf, rbuf, sizeof(wbuf))) {
			errno = EIO;
			die("compare");
		}
	}
	close(fd);

	if (umount(mnt) < 0)
		die(mnt);
}

int main(void)
{
	say("CODEX_INITRAMFS_START\n");

	mkdir_if_needed("/dev");
	mkdir_if_needed("/proc");
	mkdir_if_needed("/sys");
	mkdir_if_needed("/mnt");
	mkdir_if_needed("/mnt/nvme");
	mkdir_if_needed("/mnt/hdd");

	mount("devtmpfs", "/dev", "devtmpfs", 0, "");
	mount("proc", "/proc", "proc", 0, "");
	mount("sysfs", "/sys", "sysfs", 0, "");

	exercise_mount("/dev/nvme0n1", "/mnt/nvme", "nvme");
	exercise_mount("/dev/sda", "/mnt/hdd", "hdd");

	sync();
	say("CODEX_MINIX_TEST_PASS\n");
	reboot(LINUX_REBOOT_CMD_POWER_OFF);
	for (;;)
		pause();
}
