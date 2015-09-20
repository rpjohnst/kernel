#include <stdint.h>
#include <stdbool.h>

void apic_init(uint64_t lapic, bool legacy_pic);
void apic_timer_calibrate(void);
