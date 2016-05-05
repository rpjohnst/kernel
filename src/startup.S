#include <paging.h>

// kernel_start(void *map, size_t map_size, size_t descriptor_size, void *rsdp)
// main kernel entry point- takes UEFI memory map and ACPI RSDP in SysV ABI
	.section .startup.text, "awx"
	.global kernel_start
kernel_start:
	// save args
	movq %rdi, %r8
	movq %rdx, %r10
	movq %rcx, %r11

	// page tables
	movq $(kernel_pml4 - KERNEL_BASE), %rax
	movq %rax, %cr3

	// zero bss
	cld
	xorq %rax, %rax
	leaq bss_begin, %rdi
	leaq bss_end, %rcx
	subq %rdi, %rcx
	shrq $3, %rcx
	rep stosq

	.global ap_start
ap_start:

	// stack
	movq startup_stack(%rip), %rsp

	// gdt
	lgdt gdt_desc
	xorl %eax, %eax
	movl %eax, %ds
	movl %eax, %es
	movl %eax, %fs
	movl %eax, %gs
	movl %eax, %ss

	// per-cpu %gs
	movl $0xc0000101, %ecx
	movq startup_gs(%rip), %rax
	movq %rax, %rdx
	shrq $32, %rdx
	wrmsr

	// restore args
	movq %r8, %rdi
	movq %r10, %rdx
	movq %r11, %rcx

	// load cs and jump to high addresses
	movq startup_code(%rip), %rax
	pushq $0x08
	pushq %rax
	lretq

	.section .startup.data, "a"

	.global startup_gs
startup_gs:
	.quad 0

	.global startup_stack
startup_stack:
	.quad bsp_stack

	.global startup_code
startup_code:
	.quad kernel_init

	// initial page tables for the kernel
	// this includes a temporary identity mapping before we jump to the higher half,
	// the start of the direct mapping at DIRECT_BASE to bootstrap the VIRT_DIRECT macro,
	// and the kernel mapping itself in the last entry of the kernel_pml4

	.global pdpt_direct
	.align 4096
pdpt_direct:
	.quad pd - KERNEL_BASE + PAGE_PRESENT + PAGE_WRITE
	.fill 511, 8, 0

	.data

	.global kernel_pml4
	.align 4096
kernel_pml4:
	.quad pdpt_direct - KERNEL_BASE + PAGE_PRESENT + PAGE_WRITE
	.org kernel_pml4 + 8 * PML4_INDEX(DIRECT_BASE)
	.quad pdpt_direct - KERNEL_BASE + PAGE_PRESENT + PAGE_WRITE
	.org kernel_pml4 + 8 * PML4_INDEX(KERNEL_BASE)
	.quad pdpt - KERNEL_BASE + PAGE_PRESENT + PAGE_WRITE

	.global pdpt
	.align 4096
pdpt:
	.fill 510, 8, 0
	.quad pd - KERNEL_BASE + PAGE_PRESENT + PAGE_WRITE
	.quad pd_map - KERNEL_BASE + PAGE_PRESENT + PAGE_WRITE

	.global pd
	.align 4096
pd:
	i = 0
	.rept 512
	.quad (i << 21) + PAGE_PRESENT + PAGE_WRITE + PAGE_LARGE + PAGE_GLOBAL
	i = i + 1
	.endr

	.global pd_map
	.align 4096
pd_map:
	.quad pt_map - KERNEL_BASE + PAGE_PRESENT + PAGE_WRITE + PAGE_GLOBAL
	.fill 511, 8, 0

	.bss

	.global pt_map
	.align 4096
pt_map:
	.fill 512, 8, 0

	.global bsp_stack
	.align 4096
	.fill 4096, 1, 0
bsp_stack:
