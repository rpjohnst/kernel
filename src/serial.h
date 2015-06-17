#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define COM1 0x3f8

void serial_init(uint16_t port);
bool serial_available(uint16_t port);
uint8_t serial_read(uint16_t port);
bool serial_empty(uint16_t port);
void serial_write(uint16_t port, uint8_t value);

void serial_write_chars(const char *buf, size_t n);
