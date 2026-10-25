// SPDX-License-Identifier: GPL-2.0
/*
 * Charging power profile for the veux PD/PPS policy manager.
 * Based on fastchgtoggle by KamiKaonashi (kernel_xiaomi_stone).
 */

#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <misc/fastchgtoggle.h>

static int fast_chg_mode = FAST_CHARGE_67W;

/*
 * Battery current caps in mA, roughly W * ~0.93 efficiency / ~3.9V.
 * 67W keeps the stock pd_policy_manager tables (up to 12200 mA).
 */
static const int fast_chg_ibat_caps[] = {
	[FAST_CHARGE_NORMAL]	= 0,
	[FAST_CHARGE_18W]	= 4300,
	[FAST_CHARGE_33W]	= 7900,
	[FAST_CHARGE_67W]	= 0,
};

int fast_chg_get_mode(void)
{
	return READ_ONCE(fast_chg_mode);
}

int fast_chg_ibat_limit(void)
{
	return fast_chg_ibat_caps[fast_chg_get_mode()];
}

bool fast_chg_cp_allowed(void)
{
	return fast_chg_get_mode() != FAST_CHARGE_NORMAL;
}

static ssize_t mode_show(struct kobject *kobj, struct kobj_attribute *attr,
			 char *buf)
{
	return sprintf(buf, "%d\n", fast_chg_get_mode());
}

static ssize_t mode_store(struct kobject *kobj, struct kobj_attribute *attr,
			  const char *buf, size_t count)
{
	int ret, val;

	ret = kstrtoint(buf, 10, &val);
	if (ret < 0)
		return ret;

	if (val < FAST_CHARGE_NORMAL || val > FAST_CHARGE_67W)
		return -EINVAL;

	WRITE_ONCE(fast_chg_mode, val);
	return count;
}

static struct kobj_attribute mode_attr = __ATTR(mode, 0664, mode_show, mode_store);

static struct attribute *attrs[] = {
	&mode_attr.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

static struct kobject *fastchg_kobj;

static int __init fastchgtoggle_init(void)
{
	int ret;

	fastchg_kobj = kobject_create_and_add("fastchgtoggle", kernel_kobj);
	if (!fastchg_kobj)
		return -ENOMEM;

	ret = sysfs_create_group(fastchg_kobj, &attr_group);
	if (ret)
		kobject_put(fastchg_kobj);

	return ret;
}
late_initcall(fastchgtoggle_init);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("KamiKaonashi");
MODULE_DESCRIPTION("Fast charge power profile");
