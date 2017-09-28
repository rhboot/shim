/*
 * Copyright 2013 Red Hat, Inc <pjones@redhat.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef SHIM_REPLACEMENTS_H
#define SHIM_REPLACEMENTS_H

extern EFI_SYSTEM_TABLE *get_active_systab(void);

typedef enum {
	VERIFIED_BY_NOTHING,
	VERIFIED_BY_CERT,
	VERIFIED_BY_HASH
} verification_method_t;

extern verification_method_t verification_method;
extern int loader_is_participating;

extern void hook_system_services(EFI_SYSTEM_TABLE *local_systab);
extern void unhook_system_services(void);

extern void hook_exit(EFI_SYSTEM_TABLE *local_systab);
extern void unhook_exit(void);

extern EFI_STATUS install_shim_protocols(void);
extern void uninstall_shim_protocols(void);

#endif /* SHIM_REPLACEMENTS_H */
