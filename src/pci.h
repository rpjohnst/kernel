#include <stdint.h>

void pci_add_segment(uint16_t segment, uint64_t ecam_address, uint8_t bus_start, uint8_t bus_end);
void pci_enumerate(void);
