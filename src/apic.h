#include <stdint.h>
#include <stdbool.h>

void apic_init(uint32_t lapic_address, bool legacy_pic);
void apic_timer_calibrate(void);
