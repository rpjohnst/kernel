ENTRY(kernel_start_phys)

kernel_base = 0xffffffff80000000;
kernel_base_phys = 0x100000;

kernel_start_phys = kernel_start - kernel_base;

/* TODO: use PHDRS to specify rwx attributes? */

SECTIONS {
	. = kernel_base + kernel_base_phys;

	kernel_begin = .;

	.text : AT(ADDR(.text) - kernel_base) { *(.text) *(.text.*) }
	.data : AT(ADDR(.data) - kernel_base) { *(.data) *(.data.*) *(.rodata) *(.rodata.*) }
	.bss : AT(ADDR(.bss) - kernel_base) {
		bss_begin = .;
		*(.bss) *(.bss.*) *(COMMON)
		bss_end = .;
	}

	/* startup data can be deallocated after boot */
	. = ALIGN(0x1000);
	startup_begin = .;

	/* KEEP() gives the linker's section gc a root to start from */
	.startup.text : AT(ADDR(.startup.text) - kernel_base) { KEEP(*(.startup.text)) }
	.startup.data : AT(ADDR(.startup.data) - kernel_base) { *(.startup.data) }

	/* smp trampoline needs to be moved to low 1MB */
	trampoline_begin = .;
	.trampoline 0 : AT(trampoline_begin - kernel_base) { KEEP(*(.trampoline)) }
	. = trampoline_begin + SIZEOF(.trampoline);
	trampoline_end = .;

	/* per-cpu data gets linked at 0 so it can be used as an offset to %gs */
	. = ALIGN(64);
	percpu_begin = .;
	.percpu 0 : AT(percpu_begin - kernel_base) { *(.percpu) }
	. = percpu_begin + SIZEOF(.percpu);
	. = ALIGN(64);
	percpu_end = .;

	. = ALIGN(0x1000);
	startup_end = .;

	kernel_end = .;

	/DISCARD/ : { *(.eh_frame) *(.comment) }
}
