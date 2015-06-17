ENTRY(kernel_start_phys)

kernel_base = 0xffffffff80000000;
kernel_base_phys = 0x200000;

kernel_start_phys = kernel_start - kernel_base;

SECTIONS {
	. = kernel_base + kernel_base_phys;

	kernel_start = .;

	.text : AT(ADDR(.text) - kernel_base) { KEEP(*(.text.kernel_start)) *(.text.*) }
	.data : AT(ADDR(.data) - kernel_base) { *(.data.*) *(.rodata.*) }

	.bss : AT(ADDR(.bss) - kernel_base) {
		bss_start = .;

		*(.bss)
		*(.bss.*)
		*(COMMON)

		bss_end = .;
	}

	kernel_end = .;

	/DISCARD/ : { *(.eh_frame) *(.comment) }
}
