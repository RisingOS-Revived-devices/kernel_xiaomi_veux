#ifndef __KSU_H_APP_PROFILE
#define __KSU_H_APP_PROFILE

/*
 * Thread flag: this task's su-session asked us not to escalate it again.
 * Bit 63 is well above every arch TIF_* bit, so no arch header change is needed.
 */
#define TIF_KSU_DISABLE_ESCAPE_WITH_ROOT 63

// Escalate current process to root with the appropriate profile
int escape_with_root_profile(void);

void escape_to_root_forced(void);

#endif
