#include "pci.h"
#include "cpu.h"
#include <paging.h>
#include <kprintf.h>
#include <stdint.h>

struct pci_function {
	uint16_t vendor_id;
	uint16_t device_id;
	uint16_t command;
	uint16_t status;
	uint8_t revision_id;
	uint8_t prog_if;
	uint8_t subclass;
	uint8_t class;
	uint8_t cache_line_size;
	uint8_t latency_timer;
	uint8_t header_type;
	uint8_t bist;
	uint32_t bar[6];
	uint32_t cardbus;
	uint16_t subsystem_vendor_id;
	uint16_t subsystem_id;
	uint32_t expansion_rom;
	uint8_t capabilities;
	uint8_t reserved[7];
	uint8_t interrupt_line;
	uint8_t interrupt_pin;
	uint8_t min_grant;
	uint8_t max_latency;
};

// TODO: actually support non-zero groups
static size_t segments = 0;
static uint64_t segment_groups[1];

static volatile void *ecam;

void pci_add_segment(uint16_t segment, uint64_t ecam_address, uint8_t bus_start, uint8_t bus_end) {
	segment_groups[segments] = ecam_address;
	segments++;

	kprintf("pci: [segment %d] %#018lx buses %d-%d\n", segment, ecam_address, bus_start, bus_end);
}

static inline struct pci_function *function_address(uint8_t bus, uint8_t device, uint8_t function) {
	return (struct pci_function*)((char*)ecam +
		((bus & 0xff) << 20) +
		((device & 0x1f) << 15) +
		((function & 0x7) << 12));
}

static void enumerate_function(uint8_t bus_id, uint8_t device_id, uint8_t function_id) {
	struct pci_function *function = function_address(bus_id, device_id, function_id);

	kprintf(
		"  function %d: %x:%x %x\n",
		function_id, function->vendor_id, function->device_id, function->header_type
	);
}

static void enumerate_device(uint8_t bus_id, uint8_t device_id) {
	struct pci_function *device = function_address(bus_id, device_id, 0);
	if (device->vendor_id == 0xffff) {
		return;
	}

	kprintf(" device %d\n", device_id);

	if ((device->header_type & 0x80) == 0) {
		enumerate_function(bus_id, device_id, 0);
	} else {
		for (uint8_t function_id = 0; function_id < 8; function_id++) {
			struct pci_function *function = function_address(bus_id, device_id, function_id);
			if (function->vendor_id == 0xffff) {
				continue;
			}

			enumerate_function(bus_id, device_id, function_id);
		}
	}
}

static void enumerate_bus(uint8_t bus_id) {
	kprintf("bus %d\n", bus_id);

	for (uint8_t device_id = 0; device_id < 32; device_id++) {
		enumerate_device(bus_id, device_id);
	}
}

void pci_enumerate(void) {
	uint64_t ecam_address = segment_groups[0];

	// TODO: factor out temporary mappings
	extern uint64_t kernel_pml4[], pd_map[];
	for (int i = 0; i < 8; i += 2) {
		pd_map[1 + i / 2] =
			ecam_address | PAGE_PRESENT | PAGE_WRITE | PAGE_CACHE_UC | PAGE_LARGE | PAGE_GLOBAL;
	}
	write_cr3(PHYS_KERNEL(kernel_pml4));
	ecam = (volatile void*)0xffffffffc0200000;

	struct pci_function *root = function_address(0, 0, 0);
	if ((root->header_type & 0x80) == 0) {
		enumerate_bus(0);
	} else {
		for (uint8_t function_id = 0; function_id < 8; function_id++) {
			struct pci_function *bus = function_address(0, 0, function_id);
			if (bus->vendor_id != 0xffff) {
				break;
			}

			enumerate_bus(function_id);
		}
	}
}
