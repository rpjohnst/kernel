#include <paging.h>

	.section .trampoline, "awx"
	.code16

	.global trampoline
trampoline:

	// trampoline starts execution at some unknown address but with IP=0
	// set all the segments to CS so it can access trampoline data
	mov %cs, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss

	// notify the BSP that this core has started up
	movl $1, smp_ap_started

	// calculate linear address
	mov %cs, %ax
	movzx %ax, %esi
	shll $4, %esi

	// fix up absolute pointers
	leal trampoline_gdt(%esi), %eax
	movl %eax, trampoline_gdt_desc + 2
	leal trampoline_64(%esi), %eax
	movl %eax, trampoline_64_vector

	// load temporary gdt
	lgdtl trampoline_gdt_desc

	// enable PAE and PGE
	movl %cr4, %eax
	orl $0b10100000, %eax
	movl %eax, %cr4

	// load the page tables
	movl $(kernel_pml4 - KERNEL_BASE), %eax
	movl %eax, %cr3

	// enable LME
	movl $0xc0000080, %ecx
	rdmsr
	orl $0x100, %eax
	wrmsr

	// enable paging and switch to long mode
	movl %cr0, %eax
	orl $0x80000001, %eax
	movl %eax, %cr0

	ljmpl *trampoline_64_vector

	.code64
trampoline_64:
	lea ap_start, %rax
	jmp *%rax

	.global smp_ap_started
	.align 4
smp_ap_started:
	.long 0

	.align 4
trampoline_gdt:
	.quad 0
	.quad 0x00209a0000000000
	.quad 0x0000920000000000

trampoline_gdt_desc:
	.short trampoline_gdt_desc - trampoline_gdt
	.long trampoline_gdt

	.align 4
trampoline_64_vector:
	.long trampoline_64
	.word 0x8
