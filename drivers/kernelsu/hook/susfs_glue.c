// SPDX-License-Identifier: GPL-2.0
// SUSFS <-> KSU glue: the SELinux domain tracking and the umount helper that
// the fs-side SUSFS code (fs/susfs.c and the patched fs/* files) calls into.
// From simonpunk/susfs4ksu's 10_enable_susfs_for_ksu.patch, adapted to this
// fork. Built only with CONFIG_KSU_SUSFS.

#include <linux/susfs.h>
#include <linux/security.h>
#include "objsec.h"

#define SUSFS_SU_SECCTX "u:r:su:s0"
#define SUSFS_INIT_SECCTX "u:r:init:s0"
#define SUSFS_ZYGOTE_SECCTX "u:r:zygote:s0"

u32 susfs_zygote_sid;
u32 susfs_ksu_sid;
u32 susfs_init_sid;

static void susfs_set_sid(const char *secctx_name, u32 *out_sid)
{
	int err;

	if (!secctx_name || !out_sid)
		return;

	err = security_secctx_to_secid(secctx_name, strlen(secctx_name),
				       out_sid);
	if (err) {
		pr_err("susfs: failed setting sid for '%s', err: %d\n",
		       secctx_name, err);
		return;
	}
	pr_info("susfs: sid '%u' set for '%s'\n", *out_sid, secctx_name);
}

bool susfs_is_sid_equal(void *sec, u32 sid2)
{
	struct task_security_struct *tsec = (struct task_security_struct *)sec;

	if (!tsec)
		return false;
	return tsec->sid == sid2;
}

u32 susfs_get_sid_from_name(const char *secctx_name)
{
	u32 out_sid = 0;
	int err;

	if (!secctx_name)
		return 0;
	err = security_secctx_to_secid(secctx_name, strlen(secctx_name),
				       &out_sid);
	if (err)
		return 0;
	return out_sid;
}

u32 susfs_get_current_sid(void)
{
	return current_sid();
}

void susfs_set_zygote_sid(void)
{
	susfs_set_sid(SUSFS_ZYGOTE_SECCTX, &susfs_zygote_sid);
}

bool susfs_is_current_zygote_domain(void)
{
	return unlikely(current_sid() == susfs_zygote_sid);
}

void susfs_set_ksu_sid(void)
{
	susfs_set_sid(SUSFS_SU_SECCTX, &susfs_ksu_sid);
}

bool susfs_is_current_ksu_domain(void)
{
	return unlikely(current_sid() == susfs_ksu_sid);
}

void susfs_set_init_sid(void)
{
	susfs_set_sid(SUSFS_INIT_SECCTX, &susfs_init_sid);
}

bool susfs_is_current_init_domain(void)
{
	return unlikely(current_sid() == susfs_init_sid);
}

#ifdef CONFIG_KSU_SUSFS_TRY_UMOUNT
extern void ksu_try_umount(const char *mnt, bool check_mnt, int flags,
			   uid_t uid);

void susfs_try_umount_all(uid_t uid)
{
	susfs_try_umount(uid);
	ksu_try_umount("/system", true, 0, uid);
	ksu_try_umount("/system_ext", true, 0, uid);
	ksu_try_umount("/vendor", true, 0, uid);
	ksu_try_umount("/product", true, 0, uid);
	ksu_try_umount("/odm", true, 0, uid);
	ksu_try_umount("/data/adb/modules", false, MNT_DETACH, uid);
	ksu_try_umount("/debug_ramdisk", true, MNT_DETACH, uid);
}
#endif

// Resolve the zygote/ksu/init domains once the SELinux policy is loaded.
void susfs_capture_domains(void)
{
	susfs_set_zygote_sid();
	susfs_set_ksu_sid();
	susfs_set_init_sid();
}
