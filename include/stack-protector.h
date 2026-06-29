// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * stack-protector.h - support for gcc -fstack-protector
 * Copyright Peter Jones <pjones@redhat.com>
 */

#pragma once

extern uintptr_t __stack_chk_guard;
extern uintptr_t stack_protector_init(void);

/*
 * This call must only be called from functions that do not return.  That
 * is, the caller must call BS->Exit() to end the task rather than
 * returning, or else the stack protector will trigger on the function
 * postamble.
 */
static inline __attribute__((__always_inline__)) void
update_stack_guard(void)
{
	uintptr_t guard;

	guard = stack_protector_init();
	if (guard)
		__stack_chk_guard = guard;
}

// vim:fenc=utf-8:tw=75:noet
