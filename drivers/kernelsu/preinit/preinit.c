// SPDX-License-Identifier: GPL-2.0
// Module preinit: run module actions before SELinux policy loads, for managers
// that mark preinit modules as supported (SukiSU, KOWX).
//
// A static nolibc runner is embedded in .init.rodata (preinit/blob.S), copied
// into an unlinked shmem file at init, and swapped in for PID 1's
// "/system/bin/init selinux_setup" exec via our manual do_execve hook. The
// runner parses /metadata/.../preinit.conf, keeps a 3-strike boot counter,
// skips charger mode, then execs the real init. Disable with preinit=0.

#include <linux/err.h>
#include <linux/fcntl.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/moduleparam.h>
#include <linux/namei.h>
#include <linux/sched.h>
#include <linux/shmem_fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#include "preinit/preinit.h"

#define INIT_PATH "/system/bin/init"

// The runner, embedded by preinit/blob.S.
extern const char ksu_preinit_runner_start[];
extern const char ksu_preinit_runner_end[];

static bool ksu_preinit_enabled = true;
module_param_named(preinit, ksu_preinit_enabled, bool, 0);

// Owned here until PID 1 execs it; the init blob is freed by then, so it lives
// in an unlinked shmem file.
static struct file *ksu_preinit_file;

void __init ksu_preinit_init(void)
{
	size_t size = ksu_preinit_runner_end - ksu_preinit_runner_start;
	struct file *file;
	loff_t pos = 0;
	ssize_t written;

	if (!ksu_preinit_enabled) {
		pr_info("preinit: disabled\n");
		return;
	}

	file = shmem_file_setup("ksu_preinit", size, 0);
	if (IS_ERR(file)) {
		pr_err("preinit: shmem_file_setup failed: %ld\n", PTR_ERR(file));
		return;
	}

	written = kernel_write(file, ksu_preinit_runner_start, size, &pos);
	if (written != (ssize_t)size) {
		pr_err("preinit: write runner failed: %zd\n", written);
		fput(file);
		return;
	}

	ksu_preinit_file = file;
	pr_info("preinit: runner staged (%zu bytes)\n", size);
}

bool ksu_preinit_enabled_param(void)
{
	return ksu_preinit_enabled;
}

void ksu_preinit_release(void)
{
	struct file *file = xchg(&ksu_preinit_file, NULL);

	if (file) {
		pr_info("preinit: selinux_setup not seen, dropping runner\n");
		fput(file);
	}
}

void ksu_preinit_exit(void)
{
	ksu_preinit_release();
}

static bool is_selinux_setup(const char __user *const __user *argv_user)
{
	static const char expected[] = "selinux_setup";
	const char __user *arg;
	char buf[sizeof(expected) + 1];

	if (!argv_user || get_user(arg, argv_user + 1) || !arg)
		return false;

	memset(buf, 0, sizeof(buf));
	if (strncpy_from_user(buf, arg, sizeof(buf) - 1) < 0)
		return false;

	return !strcmp(buf, expected);
}

// Called from do_execve. When PID 1 execs "/system/bin/init selinux_setup",
// swap the kernel filename to the staged runner's /proc/self/fd/N and let
// do_execve run it; the runner then execs the real init.
void ksu_preinit_handle_kexecve(struct filename **filename_ptr,
				const char __user *const __user *argv_user)
{
	struct filename *old, *new;
	char fd_path[32];
	struct file *file;
	struct path p;
	int fd;

	if (likely(!READ_ONCE(ksu_preinit_file)))
		return;

	if (!current->mm || current->pid != 1)
		return;

	if (!filename_ptr || !*filename_ptr || !(*filename_ptr)->name)
		return;

	if (strcmp((*filename_ptr)->name, INIT_PATH) ||
	    !is_selinux_setup(argv_user))
		return;

	// One shot: the runner's own exec of the real init must pass through.
	file = xchg(&ksu_preinit_file, NULL);
	if (!file)
		return;

	fd = get_unused_fd_flags(O_CLOEXEC);
	if (fd < 0) {
		pr_err("preinit: get fd failed: %d\n", fd);
		fput(file);
		return;
	}
	// The fd owns the file now; close-on-exec frees it once the runner loads.
	fd_install(fd, file);

	snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%d", fd);
	if (kern_path(fd_path, LOOKUP_FOLLOW, &p)) {
		pr_err("preinit: %s not reachable, booting without preinit\n",
		       fd_path);
		close_fd(fd);
		return;
	}
	path_put(&p);

	new = getname_kernel(fd_path);
	if (IS_ERR(new)) {
		pr_err("preinit: getname_kernel failed, booting normally\n");
		close_fd(fd);
		return;
	}

	old = *filename_ptr;
	*filename_ptr = new;
	putname(old);
	pr_info("preinit: exec runner via %s\n", fd_path);
}
