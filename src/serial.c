#include "serial.h"
#include "cpu.h"

void serial_init(uint16_t port) {
	outb(port + 1, 0x00); // disable interrupts

	outb(port + 3, 0x80); // enable DLAB
	outb(port + 0, 0x01); // set divisor to 1
	outb(port + 1, 0x00);

	outb(port + 3, 0x03); // 8 bits, no parity, one stop bit
	outb(port + 2, 0xc7); // enable fifo, clear with 14-byte threshold
	outb(port + 4, 0x0b); // enable interrupts, rts/dsr set
}

bool serial_available(uint16_t port) {
	return inb(port + 5) & 0x1;
}

uint8_t serial_read(uint16_t port) {
	while (!serial_available(port));
	return inb(port);
}

bool serial_empty(uint16_t port) {
	return inb(port + 5) & 0x20;
}

void serial_write(uint16_t port, uint8_t value) {
	while (!serial_empty(port));
	outb(port, value);
}

void serial_write_chars(const char *buf, size_t n) {
	while (n--) {
		if (*buf == '\n')
			serial_write(COM1, '\r');
		
		serial_write(COM1, *buf++);
	}
}
