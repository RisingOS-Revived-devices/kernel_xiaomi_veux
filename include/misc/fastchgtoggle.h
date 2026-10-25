/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _MISC_FASTCHGTOGGLE_H
#define _MISC_FASTCHGTOGGLE_H

#include <linux/types.h>

/* Values of /sys/kernel/fastchgtoggle/mode */
#define FAST_CHARGE_NORMAL	0	/* charge pump off, 5V switching charger only */
#define FAST_CHARGE_18W		1
#define FAST_CHARGE_33W		2
#define FAST_CHARGE_67W		3	/* stock, default */

#ifdef CONFIG_FASTCHG_TOGGLE
int fast_chg_get_mode(void);
/* Battery current cap in mA for the charge pump, 0 = no extra cap. */
int fast_chg_ibat_limit(void);
/* False when the selected mode keeps the charge pump (PPS) off. */
bool fast_chg_cp_allowed(void);
#else
static inline int fast_chg_get_mode(void) { return FAST_CHARGE_67W; }
static inline int fast_chg_ibat_limit(void) { return 0; }
static inline bool fast_chg_cp_allowed(void) { return true; }
#endif

#endif
