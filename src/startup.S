#include <paging.h>

// kernel_start(void *map, size_t map_size, size_t descriptor_size, void *rsdp)
// main kernel entry point- takes UEFI memory map and ACPI RSDP in SysV ABI
	.section .text.kernel_start
	.global kernel_start
kernel_start:
	// page tables
	movq $(kernel_pml4 - KERNEL_BASE), %rax
	movq %rax, %cr3

	// stack
	leaq stack, %rsp

	// gdt
	lgdt gdt_desc
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

	// load cs and jump to high addresses
	leaq 1f, %rax
	pushq $0x08
	pushq %rax
	lretq
1:

	// clear identity mapping
	movq $0, kernel_pml4
	movq $(kernel_pml4 - KERNEL_BASE), %rax
	movq %rax, %cr3

	// zero bss
	mov %rdi, %r10
	mov %rcx, %r11

	cld
	xor %rax, %rax
	leaq bss_start, %rdi
	leaq bss_end, %rcx
	subq %rdi, %rcx
	shrq $3, %rcx
	rep stosq

	mov %r10, %rdi
	mov %r11, %rcx

	// jump to c
	jmp kernel_init

	.data

	// initial page tables for the kernel
	// this includes a temporary identity mapping before we jump to the higher half,
	// the start of the direct mapping at DIRECT_BASE to bootstrap the VIRT_DIRECT macro,
	// and the kernel mapping itself in the last entry of the kernel_pml4

	.global kernel_pml4
	.align 4096
kernel_pml4:
	.quad pdpt_direct - KERNEL_BASE + PAGE_PRESENT + PAGE_WRITE
	.org kernel_pml4 + 8 * PML4_INDEX(DIRECT_BASE)
	.quad pdpt_direct - KERNEL_BASE + PAGE_PRESENT + PAGE_WRITE
	.org kernel_pml4 + 8 * PML4_INDEX(KERNEL_BASE)
	.quad pdpt - KERNEL_BASE + PAGE_PRESENT + PAGE_WRITE

	.global pdpt_direct
	.align 4096
pdpt_direct:
	.quad pd - KERNEL_BASE + PAGE_PRESENT + PAGE_WRITE
	.fill 511, 8, 0

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

	.global stack
	.fill 4096, 1, 0
stack:
