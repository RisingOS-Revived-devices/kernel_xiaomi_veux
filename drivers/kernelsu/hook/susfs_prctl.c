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

/*
 * Current managers (KernelSU-Next, SukiSU, ReSukiSU) detect SUSFS through the
 * v2 channel, reboot(0xDEADBEEF, 0xFAFAFAFA, cmd, struct *), and never try the
 * v1.5.5 prctl one, so they report SUSFS as missing. Answer their three query
 * commands with what this kernel has. Every other v2 command keeps the
 * ERR_CMD_NOT_SUPPORTED that ksud preloads into the struct, since the v2
 * structs differ from the v1.5.5 ones this kernel implements.
 */
#define SUSFS_V2_MAGIC 0xFAFAFAFA
#define SUSFS_V2_VERSION_BUFSIZE 16
#define SUSFS_V2_FEATURES_SIZE 8192

struct susfs_v2_short_str {
	char str[SUSFS_V2_VERSION_BUFSIZE];
	int err;
};

static const char *const susfs_v2_features[] = {
#ifdef CONFIG_KSU_SUSFS_SUS_PATH
	"CONFIG_KSU_SUSFS_SUS_PATH",
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_MOUNT
	"CONFIG_KSU_SUSFS_SUS_MOUNT",
#endif
#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_KSU_DEFAULT_MOUNT
	"CONFIG_KSU_SUSFS_AUTO_ADD_SUS_KSU_DEFAULT_MOUNT",
#endif
#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_BIND_MOUNT
	"CONFIG_KSU_SUSFS_AUTO_ADD_SUS_BIND_MOUNT",
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_KSTAT
	"CONFIG_KSU_SUSFS_SUS_KSTAT",
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_OVERLAYFS
	"CONFIG_KSU_SUSFS_SUS_OVERLAYFS",
#endif
#ifdef CONFIG_KSU_SUSFS_TRY_UMOUNT
	"CONFIG_KSU_SUSFS_TRY_UMOUNT",
#endif
#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_TRY_UMOUNT_FOR_BIND_MOUNT
	"CONFIG_KSU_SUSFS_AUTO_ADD_TRY_UMOUNT_FOR_BIND_MOUNT",
#endif
#ifdef CONFIG_KSU_SUSFS_SPOOF_UNAME
	"CONFIG_KSU_SUSFS_SPOOF_UNAME",
#endif
#ifdef CONFIG_KSU_SUSFS_ENABLE_LOG
	"CONFIG_KSU_SUSFS_ENABLE_LOG",
#endif
#ifdef CONFIG_KSU_SUSFS_HIDE_KSU_SUSFS_SYMBOLS
	"CONFIG_KSU_SUSFS_HIDE_KSU_SUSFS_SYMBOLS",
#endif
#ifdef CONFIG_KSU_SUSFS_SPOOF_CMDLINE_OR_BOOTCONFIG
	"CONFIG_KSU_SUSFS_SPOOF_CMDLINE_OR_BOOTCONFIG",
#endif
#ifdef CONFIG_KSU_SUSFS_OPEN_REDIRECT
	"CONFIG_KSU_SUSFS_OPEN_REDIRECT",
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_SU
	"CONFIG_KSU_SUSFS_SUS_SU",
#endif
#ifdef CONFIG_KSU_SUSFS_HAS_MAGIC_MOUNT
	"CONFIG_KSU_SUSFS_HAS_MAGIC_MOUNT",
#endif
};

static void susfs_v2_short_reply(void __user *arg, const char *value)
{
	struct susfs_v2_short_str reply = { .err = 0 };

	strscpy(reply.str, value, sizeof(reply.str));
	if (copy_to_user(arg, &reply, sizeof(reply)))
		pr_info("susfs: v2 reply failed\n");
}

static void susfs_v2_features_reply(void __user *arg)
{
	char *buf = kzalloc(SUSFS_V2_FEATURES_SIZE, GFP_KERNEL);
	size_t len = 0;
	int i, err = 0;

	if (!buf) {
		err = -ENOMEM;
		goto out;
	}
	for (i = 0; i < ARRAY_SIZE(susfs_v2_features); i++)
		len += scnprintf(buf + len, SUSFS_V2_FEATURES_SIZE - len, "%s\n",
				 susfs_v2_features[i]);
	if (copy_to_user(arg, buf, SUSFS_V2_FEATURES_SIZE))
		err = -EFAULT;
	kfree(buf);
out:
	if (copy_to_user((char __user *)arg + SUSFS_V2_FEATURES_SIZE, &err, sizeof(err)))
		pr_info("susfs: v2 reply failed\n");
}

void ksu_handle_susfs_v2_query(int magic2, unsigned int cmd, void __user *arg)
{
	if (magic2 != (int)SUSFS_V2_MAGIC || current_uid().val != 0 || !arg)
		return;

	switch (cmd) {
	case CMD_SUSFS_SHOW_VERSION:
		susfs_v2_short_reply(arg, SUSFS_VERSION);
		break;
	case CMD_SUSFS_SHOW_VARIANT:
		susfs_v2_short_reply(arg, SUSFS_VARIANT);
		break;
	case CMD_SUSFS_SHOW_ENABLED_FEATURES:
		susfs_v2_features_reply(arg);
		break;
	}
}
