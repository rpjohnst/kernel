#include <stdint.h>

struct hpet;

void hpet_init(uint64_t hpet_address);
void hpet_enable(void);

uint64_t hpet_now(void);
uint64_t hpet_period(void);
