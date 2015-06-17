#include "segment.h"
#include <stdint.h>

struct gdt_entry {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_middle;
	uint8_t type: 4, s: 1, dpl: 2, p: 1;
	uint8_t limit_high: 4, avl: 1, l: 1, d: 1, g: 1;
	uint8_t base_high;
} gdt[] = {
	[GDT_KERNEL_CODE] = { .type = 0xa, .s = 1, .p = 1, .l = 1 },
	[GDT_KERNEL_DATA] = { .type = 0x2, .s = 1, .p = 1 },
	[GDT_USER_CODE] = { .type = 0xa, .s = 1, .dpl = 3, .l = 1 },
	[GDT_USER_DATA] = { .type = 0x2, .s = 1, .dpl = 3 },
};

struct gdt_desc {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed)) gdt_desc = {
	.limit = sizeof(gdt) - sizeof(*gdt),
	.base = (uint64_t)gdt,
};
