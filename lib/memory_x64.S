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

.code64
.text

#define SMALL_SIZE	47

#define ENTRY(E)					\
	.def	 E;					\
	.scl	2;					\
	.type	32;					\
	.endef;						\
	.global	E;					\
	.p2align 4, 0x90;				\
	E:

ENTRY(memset)
	cmpq $SMALL_SIZE, %r8
	movq %rdi, %r10
	movq %rcx, %r9
	cld
	movzbl %dl, %eax
	movq %rcx, %rdi
	jbe 1f
	movabs $0x0101010101010101, %rdx
	negl %ecx
	imulq %rdx, %rax
	andl $7, %ecx
	subq %rcx, %r8
	rep stosb
	movq %r8, %rcx
	andl $7, %r8d
	shrq $3, %rcx
	rep stosq
1:	movl %r8d, %ecx
	rep stosb
	movq %r9, %rax
	movq %r10, %rdi
	ret

ENTRY(memcpy)
	movq %rdi, %r10
	movq %rsi, %r9
	movq %rcx, %rax
.from_memmove:
	cmpq $SMALL_SIZE, %r8
	cld
	movq %rcx, %rdi
	movq %rdx, %rsi
	jbe 2f
	negl %ecx
	andl $7, %ecx
	subq %rcx, %r8
	rep movsb
	movq %r8, %rcx
	andl $7, %r8d
	shrq $3, %rcx
	rep movsq
2:	movl %r8d, %ecx
	rep movsb
	movq %r9, %rsi
	movq %r10, %rdi
	ret

ENTRY(memmove)
	cmpq %rdx, %rcx
	movq %rdi, %r10
	movq %rsi, %r9
	movq %rcx, %rax
	jbe .from_memmove
	cmpq $SMALL_SIZE, %r8
	std
	leaq -1(%rcx,%r8), %rdi
	leaq -1(%rdx,%r8), %rsi
	jbe 3f
	leaq 1(%rdi), %rcx
	andl $7, %ecx
	subq %rcx, %r8
	rep movsb
	movq %r8, %rcx
	subq $7, %rsi
	subq $7, %rdi
	shrq $3, %rcx
	andl $7, %r8d
	rep movsq
	addq $7, %rsi
	addq $7, %rdi
3:	movl %r8d, %ecx
	rep movsb
	movq %r9, %rsi
	movq %r10, %rdi
	cld
	ret

ENTRY(memcmp)
	cmpq $SMALL_SIZE, %r8
	movq %rsi, %r10
	movq %rdi, %r9
	cld
	movq %rcx, %rsi
	movq %rdx, %rdi
	jbe 5f
	negl %ecx
	andl $7, %ecx
	jz 4f
	subq %rcx, %r8
	repe cmpsb
	jne 6f
4:	movq %r8, %rcx
	andl $7, %r8d
	shrq $3, %rcx
	repe cmpsq
	je 5f
	orl $-1, %r8d		/* unlimited size */
	subq $8, %rsi
	subq $8, %rdi
5:	xorl %ecx, %ecx		/* ZF=1, CF=0 */
	movl %r8d, %ecx
	repe cmpsb
6:	seta %al		/* above (CF=0 and ZF=0): 1 */
	movq %r9, %rdi
	movzbl %al, %eax
	movq %r10, %rsi
	sbb $0, %eax		/* below (CF=1): -1 */
	ret
