TARGETS := img/EFI/BOOT/BOOTX64.EFI img/kernel
CFLAGS := -std=c11 -ffreestanding -fbuiltin -MMD -MP -ffunction-sections -fdata-sections -O2

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	$(RM) -r obj/ img/

.SECONDEXPANSION:
.SECONDARY:
.SUFFIXES:

obj/ img/EFI/BOOT/: ; mkdir -p $@
obj/%/: ; mkdir -p $@

# bootloader

boot_CC := x86_64-w64-mingw32-gcc
boot_LD := x86_64-w64-mingw32-ld

boot_SOURCES := src/boot.c
boot_OBJECTS := $(patsubst %.c,%.o,$(boot_SOURCES:src=obj))
boot_DEPENDS := $(patsubst %.c,%.d,$(boot_SOURCES:src=obj))

img/EFI/BOOT/BOOTX64.EFI: $(boot_SOURCES) | obj/ img/EFI/BOOT/
	$(boot_CC) $(CFLAGS) -Wall -Wextra -c -Iinclude -Iedk2/MdePkg/Include -Iedk2/MdePkg/Include/X64 -o obj/boot.o src/boot.c
	$(boot_LD) --oformat pei-x86-64 --subsystem 10 -pie -e UefiMain -o img/EFI/BOOT/BOOTX64.EFI obj/boot.o

# kernel

kernel_CC := x86_64-elf-gcc
kernel_LD := x86_64-elf-ld

kernel_OBJECTS := obj/startup.o obj/trampoline.o obj/segment.o obj/kernel.o obj/entry.o obj/interrupt.o obj/memory.o obj/paging.o obj/page.o obj/cache.o obj/hpet.o obj/apic.o obj/tsc.o obj/smp.o obj/pci.o obj/serial.o obj/kprintf.o obj/panic.o obj/acpi/parse.o obj/acpi/osl.o obj/acpi/acpica.o obj/libc/stdlib.o obj/libc/string.o obj/libc/ctype.o
kernel_DEPENDS := $(patsubst %.o,%.d,$(kernel_OBJECTS))

img/kernel: $(kernel_OBJECTS) src/kernel.ld | img/EFI/BOOT/
	$(kernel_CC) -ffreestanding -nostdlib -T src/kernel.ld -n -Wl,--gc-sections -o $@ $(kernel_OBJECTS)

obj/%.o: src/%.c | $$(dir $$@)
	$(kernel_CC) $(CFLAGS) -Wall -Wextra -Wpedantic -Wno-unused-parameter -Werror -mno-mmx -mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel -Iinclude -c -o $@ $<

obj/%.o: src/%.S | $$(dir $$@)
	$(kernel_CC) -Iinclude -D__ASSEMBLY__ -c -o $@ $<

acpica_SOURCES := $(wildcard src/acpi/acpica/*.c)
acpica_OBJECTS := $(patsubst %.c,%.o,$(acpica_SOURCES:src/%=obj/%))
acpica_DEPENDS := $(patsubst %.c,%.d,$(acpica_SOURCES:src/%=obj/%))

obj/acpi/acpica.o: $(acpica_OBJECTS)
	$(kernel_LD) -r -o $@ $(acpica_OBJECTS)

obj/acpi/acpica/%.o: src/acpi/acpica/%.c | $$(dir $$@)
	$(kernel_CC) $(CFLAGS) -mno-red-zone -mcmodel=kernel -c -Iinclude -Iinclude/acpi -o $@ $<

# dependencies

ifeq ($(filter clean, $(MAKECMDGOALS)),)
-include $(boot_DEPENDS) $(kernel_DEPENDS) $(acpica_DEPENDS)
endif
