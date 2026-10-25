// SPDX-License-Identifier: GPL-2.0
// SUSFS command channel for this fork: the stock ksu_susfs tool talks to the
// kernel via prctl(0xDEADBEEF, CMD_SUSFS_*, arg3, arg4, arg5). Our driver has
// no generic prctl handler, so this one is wired from the manual prctl hook in
// kernel/sys.c and dispatches only the SUSFS commands. Returns true when the
// command was a SUSFS command (so the syscall returns 0 to userspace).

#include <linux/susfs.h>
#include <linux/susfs_def.h>

#define KSU_SUSFS_OPTION 0xDEADBEEF

// arg3 carries a user struct; arg5 receives the int result.
#define SUSFS_USER_CALL(name, type, fn)                                        \
	if (arg2 == (name)) {                                                  \
		int error = 0;                                                 \
		if (!access_ok((void __user *)arg3, sizeof(type)) ||           \
		    !access_ok((void __user *)arg5, sizeof(error)))            \
			return true;                                           \
		error = fn((type __user *)arg3);                               \
		if (copy_to_user((void __user *)arg5, &error, sizeof(error)))  \
			pr_info("susfs: copy_to_user() failed\n");             \
		return true;                                                   \
	}

bool ksu_handle_prctl(int option, unsigned long arg2, unsigned long arg3,
		      unsigned long arg4, unsigned long arg5)
{
	if (option != (int)KSU_SUSFS_OPTION)
		return false;

	// Only root (ksud/the susfs tool running as root) may drive susfs.
	if (current_uid().val != 0)
		return false;

#ifdef CONFIG_KSU_SUSFS_SUS_PATH
	SUSFS_USER_CALL(CMD_SUSFS_ADD_SUS_PATH, struct st_susfs_sus_path,
			susfs_add_sus_path)
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_MOUNT
	SUSFS_USER_CALL(CMD_SUSFS_ADD_SUS_MOUNT, struct st_susfs_sus_mount,
			susfs_add_sus_mount)
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_KSTAT
	SUSFS_USER_CALL(CMD_SUSFS_ADD_SUS_KSTAT, struct st_susfs_sus_kstat,
			susfs_add_sus_kstat)
	SUSFS_USER_CALL(CMD_SUSFS_ADD_SUS_KSTAT_STATICALLY,
			struct st_susfs_sus_kstat, susfs_add_sus_kstat)
	SUSFS_USER_CALL(CMD_SUSFS_UPDATE_SUS_KSTAT, struct st_susfs_sus_kstat,
			susfs_update_sus_kstat)
#endif
#ifdef CONFIG_KSU_SUSFS_TRY_UMOUNT
	SUSFS_USER_CALL(CMD_SUSFS_ADD_TRY_UMOUNT, struct st_susfs_try_umount,
			susfs_add_try_umount)
	if (arg2 == CMD_SUSFS_RUN_UMOUNT_FOR_CURRENT_MNT_NS) {
		susfs_run_try_umount_for_current_mnt_ns();
		return true;
	}
#endif
#ifdef CONFIG_KSU_SUSFS_SPOOF_UNAME
	SUSFS_USER_CALL(CMD_SUSFS_SET_UNAME, struct st_susfs_uname,
			susfs_set_uname)
#endif
#ifdef CONFIG_KSU_SUSFS_ENABLE_LOG
	if (arg2 == CMD_SUSFS_ENABLE_LOG) {
		susfs_set_log(arg3 == 1);
		return true;
	}
#endif
#ifdef CONFIG_KSU_SUSFS_SPOOF_CMDLINE_OR_BOOTCONFIG
	if (arg2 == CMD_SUSFS_SET_CMDLINE_OR_BOOTCONFIG) {
		int error = 0;
		if (!access_ok((void __user *)arg5, sizeof(error)))
			return true;
		error = susfs_set_cmdline_or_bootconfig((char __user *)arg3);
		if (copy_to_user((void __user *)arg5, &error, sizeof(error)))
			pr_info("susfs: copy_to_user() failed\n");
		return true;
	}
#endif
#ifdef CONFIG_KSU_SUSFS_OPEN_REDIRECT
	SUSFS_USER_CALL(CMD_SUSFS_ADD_OPEN_REDIRECT,
			struct st_susfs_open_redirect, susfs_add_open_redirect)
#endif
	if (arg2 == CMD_SUSFS_SHOW_VERSION) {
		const char *v = SUSFS_VERSION;
		int error, len = strlen(v) + 1;
		if (!access_ok((void __user *)arg3, len) ||
		    !access_ok((void __user *)arg5, sizeof(error)))
			return true;
		error = copy_to_user((void __user *)arg3, v, len);
		if (copy_to_user((void __user *)arg5, &error, sizeof(error)))
			pr_info("susfs: copy_to_user() failed\n");
		return true;
	}
	if (arg2 == CMD_SUSFS_SHOW_VARIANT) {
		const char *v = SUSFS_VARIANT;
		int error, len = strlen(v) + 1;
		if (!access_ok((void __user *)arg3, len) ||
		    !access_ok((void __user *)arg5, sizeof(error)))
			return true;
		error = copy_to_user((void __user *)arg3, v, len);
		if (copy_to_user((void __user *)arg5, &error, sizeof(error)))
			pr_info("susfs: copy_to_user() failed\n");
		return true;
	}
	if (arg2 == CMD_SUSFS_SHOW_ENABLED_FEATURES) {
		u64 f = 0;
		int error;
		if (!access_ok((void __user *)arg3, sizeof(u64)) ||
		    !access_ok((void __user *)arg5, sizeof(error)))
			return true;
#ifdef CONFIG_KSU_SUSFS_SUS_PATH
		f |= (1 << 0);
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_MOUNT
		f |= (1 << 1);
#endif
#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_KSU_DEFAULT_MOUNT
		f |= (1 << 2);
#endif
#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_BIND_MOUNT
		f |= (1 << 3);
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_KSTAT
		f |= (1 << 4);
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_OVERLAYFS
		f |= (1 << 5);
#endif
#ifdef CONFIG_KSU_SUSFS_TRY_UMOUNT
		f |= (1 << 6);
#endif
#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_TRY_UMOUNT_FOR_BIND_MOUNT
		f |= (1 << 7);
#endif
#ifdef CONFIG_KSU_SUSFS_SPOOF_UNAME
		f |= (1 << 8);
#endif
#ifdef CONFIG_KSU_SUSFS_ENABLE_LOG
		f |= (1 << 9);
#endif
#ifdef CONFIG_KSU_SUSFS_HIDE_KSU_SUSFS_SYMBOLS
		f |= (1 << 10);
#endif
#ifdef CONFIG_KSU_SUSFS_SPOOF_CMDLINE_OR_BOOTCONFIG
		f |= (1 << 11);
#endif
#ifdef CONFIG_KSU_SUSFS_OPEN_REDIRECT
		f |= (1 << 12);
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_SU
		f |= (1 << 13);
#endif
#ifdef CONFIG_KSU_SUSFS_HAS_MAGIC_MOUNT
		f |= (1 << 14);
#endif
		error = copy_to_user((void __user *)arg3, &f, sizeof(f));
		if (copy_to_user((void __user *)arg5, &error, sizeof(error)))
			pr_info("susfs: copy_to_user() failed\n");
		return true;
	}

	// Unknown SUSFS command under our option: consume it quietly.
	return true;
}
