/*
 * Copyright 2018 Ruslan Nikolaev <nruslandev@gmail.com>
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

.code32
.text

#define SMALL_SIZE	47

/* It also defines an entry@12 alias to work around
   issues with different naming conventions. */
#define ENTRY3(E)						\
	.def	 _##E;						\
	.scl	2;						\
	.type	32;						\
	.endef;							\
	.global	_##E, _##E##@12;				\
	.set _##E##@12, _##E;					\
	.p2align 4, 0x90;					\
	_##E:

ENTRY3(memset)
	cmpl $SMALL_SIZE, %ecx
	push %eax
	push %edi
	cld
	movb %dl, %dh
	movl %eax, %edi
	movl %edx, %eax
	jbe 1f
	shll $16, %eax
	movw %dx, %ax
	movl %ecx, %edx
	movl %edi, %ecx
	negl %ecx
	andl $3, %ecx
	subl %ecx, %edx
	rep stosb
	movl %edx, %ecx
	andl $3, %edx
	shrl $2, %ecx
	rep stosl
	movl %edx, %ecx
1:	rep stosb
	pop %edi
	pop %eax
	ret

ENTRY3(memcpy)
	push %edi
	push %esi
.from_memmove:
	cmpl $SMALL_SIZE, %ecx
	cld
	movl %eax, %edi
	movl %edx, %esi
	jbe 2f
	movl %ecx, %edx
	movl %edi, %ecx
	negl %ecx
	andl $3, %ecx
	subl %ecx, %edx
	rep movsb
	movl %edx, %ecx
	andl $3, %edx
	shrl $2, %ecx
	rep movsl
	movl %edx, %ecx
2:	rep movsb
	pop %esi
	pop %edi
	ret

ENTRY3(memmove)
	cmpl %edx, %eax
	push %edi
	push %esi
	jbe .from_memmove
	cmpl $SMALL_SIZE, %ecx
	std
	leal -1(%eax,%ecx), %edi
	leal -1(%edx,%ecx), %esi
	jbe 3f
	movl %ecx, %edx
	leal 1(%edi), %ecx
	andl $3, %ecx
	subl %ecx, %edx
	rep movsb
	movl %edx, %ecx
	subl $3, %esi
	subl $3, %edi
	shrl $2, %ecx
	andl $3, %edx
	rep movsl
	addl $3, %esi
	addl $3, %edi
	movl %edx, %ecx
3:	rep movsb
	pop %esi
	pop %edi
	cld
	ret

ENTRY3(memcmp)
	cmpl $SMALL_SIZE, %ecx
	push %edi
	movl %edx, %edi
	cld
	movl %esi, %edx
	movl %eax, %esi
	movl %ecx, %eax
	jbe 5f
	movl %esi, %ecx
	negl %ecx
	andl $3, %ecx
	jz 4f
	subl %ecx, %eax
	repe cmpsb
	jne 6f
4:	movl %eax, %ecx
	andl $3, %eax
	shrl $2, %ecx		/* ECX>0 */
	repe cmpsl
	je 5f			/* ECX=0 */
	orl $-1, %eax		/* unlimited size */
	subl $4, %esi
	subl $4, %edi
5:	orl %eax, %ecx		/* ECX=EAX, ZF=1 if ECX=0, CF=0 */
	repe cmpsb
6:	seta %al		/* above (CF=0 and ZF=0): 1 */
	movl %edx, %esi
	movzbl %al, %eax
	pop %edi
	sbb $0, %eax		/* below (CF=1): -1 */
	ret
