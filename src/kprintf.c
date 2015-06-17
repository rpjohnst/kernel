#include <kprintf.h>
#include "serial.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>

void kprintf(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	kvprintf(fmt, ap);

	va_end(ap);
}

enum format_type {
	FORMAT_NONE,

	FORMAT_BYTE,
	FORMAT_SHORT,
	FORMAT_INT,
	FORMAT_LONG,
	FORMAT_LLONG,
	FORMAT_SIZE,
	FORMAT_PTRDIFF,

	FORMAT_CHAR,
	FORMAT_PTR,
	FORMAT_STR,
	FORMAT_PERCENT,
};

#define FLAG_LEFT 1
#define FLAG_SIGN 2
#define FLAG_SPACE 4
#define FLAG_ALT 8
#define FLAG_ZERO 16
#define FLAG_SIGNED 32
#define FLAG_UPPER 64

struct format {
	uint8_t type;
	uint8_t flags;
	uint8_t base;
	int16_t width;
	int16_t precision;
};

static int read_format(const char *fmt, struct format *spec, va_list ap) {
	const char *start = fmt;

	// skip leading non-specs
	spec->type = FORMAT_NONE;

	while (*fmt != '\0' && *fmt != '%')
		fmt++;

	if (fmt != start || !*fmt)
		return fmt - start;

	// flags
	spec->flags = 0;
	while (true) {
		fmt++;

		bool done = false;
		switch (*fmt) {
		case '-': spec->flags |= FLAG_LEFT; break;
		case '+': spec->flags |= FLAG_SIGN; break;
		case '#': spec->flags |= FLAG_ALT; break;
		case ' ': spec->flags |= FLAG_SPACE; break;
		case '0': spec->flags |= FLAG_ZERO; break;
		default: done = true; break;
		}
		if (done) break;
	}

	// width
	spec->width = -1;
	if (*fmt == '*') {
		spec->width = va_arg(ap, int);
		if (spec->width < 0) {
			spec->flags |= FLAG_LEFT;
			spec->width = -spec->width;
		}
		fmt++;
	} else {
		spec->width = strtoul(fmt, &fmt, 10);
	}

	// precision
	spec->precision = -1;
	if (*fmt == '.') {
		fmt++;

		if (*fmt == '*') {
			spec->precision = va_arg(ap, int);
			fmt++;
		} else {
			spec->precision = strtoul(fmt, &fmt, 10);
		}
		if (spec->precision < 0)
			spec->precision = 0;

		spec->flags &= ~FLAG_ZERO;
	}

	// length
	spec->type = FORMAT_INT;
	switch (*fmt) {
	case 'h':
		fmt++;
		if (*fmt == 'h') {
			fmt++;
			spec->type = FORMAT_BYTE;
		} else {
			spec->type = FORMAT_SHORT;
		}
		break;
	case 'l':
		fmt++;
		if (*fmt == 'l') {
			fmt++;
			spec->type = FORMAT_LLONG;
		} else {
			spec->type = FORMAT_LONG;
		}
		break;
	case 'z':
		fmt++;
		spec->type = FORMAT_SIZE;
		break;
	case 't':
		fmt++;
		spec->type = FORMAT_PTRDIFF;
		break;
	}

	// base
	spec->base = 10;
	switch (*fmt) {
	case 'c':
		spec->type = FORMAT_CHAR;
		break;
	case 's':
		spec->type = FORMAT_STR;
		break;
	case 'p':
		spec->type = FORMAT_PTR;
		break;
	case '%':
		spec->type = FORMAT_PERCENT;
		break;

	case 'o':
		spec->base = 8;
		break;

	case 'X':
		spec->flags |= FLAG_UPPER;
	case 'x':
		spec->base = 16;
		break;

	case 'd':
	case 'i':
		spec->flags |= FLAG_SIGNED;
	case 'u':
		spec->base = 10;
		break;

	default:
		return fmt - start;
	}

	return ++fmt - start;
}

static void string(const char *s, struct format spec) {
	if (s == NULL)
		s = "(null)";

	int len = strnlen(s, spec.precision);

	if (!(spec.flags & FLAG_LEFT)) {
		while (len < spec.width--)
			serial_write_chars(" ", 1);
	}

	serial_write_chars(s, len);

	while (len < spec.width--)
		serial_write_chars(" ", 1);
}

static void number(uintmax_t x, struct format spec) {
	int written = 0;

	// calculate sign
	char sign = 0;
	if (spec.flags & FLAG_SIGNED) {
		if ((intmax_t)x < 0) {
			sign = '-';
			x = -(intmax_t)x;
			written++;
		} else if (spec.flags & FLAG_SIGN) {
			sign = '+';
			written++;
		} else if (spec.flags & FLAG_SPACE) {
			sign = ' ';
			written++;
		}
	}

	// calculate prefix
	bool prefix = (spec.flags & FLAG_ALT) && spec.base != 10;
	if (prefix) {
		if (spec.base == 16) {
			written += 2;
		} else if (x != 0) {
			written++;
		}
	}

	// calculate digits
	static const char digits[16] = "0123456789ABCDEF";
	char mask = (spec.flags & FLAG_UPPER) ? 0 : 0x20;

	char buf[CHAR_BIT * sizeof(x) / 3 + 2];
	char *const out = buf + sizeof(buf);
	int i = 0;
	do {
		int digit = x % spec.base;
		x /= spec.base;

		out[-++i] = digits[digit] | mask;
	} while (x != 0);

	written += i > spec.precision ? i : spec.precision;

	// width - minimum number of chars, space padding
	if (!(spec.flags & FLAG_LEFT) && !(spec.flags & FLAG_ZERO)) {
		while (++written <= spec.width)
			serial_write_chars(" ", 1);
	}

	// sign
	if (sign != 0) {
		serial_write_chars(&sign, 1);
	}

	// prefix
	if (prefix) {
		serial_write_chars("0", 1);
		if (spec.base == 16) {
			char p = 'X' | mask;
			serial_write_chars(&p, 1);
		}
	}

	// width - minimum number of chars, zero padding
	if (!(spec.flags & FLAG_LEFT) && (spec.flags & FLAG_ZERO)) {
		while (++written <= spec.width)
			serial_write_chars("0", 1);
	}

	// precision - minimum number of digits
	for (int c = spec.precision - i; c > 0; c--)
		serial_write_chars("0", 1);

	// digits
	serial_write_chars(out - i, i);

	// width - minimum number of chars, left alignment
	if (spec.flags & FLAG_LEFT) {
		while (++written <= spec.width)
			serial_write_chars(" ", 1);
	}
}

void kvprintf(const char *fmt, va_list ap) {
	while (*fmt != '\0') {
		struct format spec = { 0 };

		const char *start = fmt;
		int read = read_format(fmt, &spec, ap);
		fmt += read;

		switch (spec.type) {
		case FORMAT_NONE:
			serial_write_chars(start, read);
			break;

		case FORMAT_CHAR: {
			if (!(spec.flags & FLAG_LEFT)) {
				while (--spec.width > 0)
					serial_write_chars(" ", 1);
			}

			char c = (unsigned char)va_arg(ap, int);
			serial_write_chars(&c, 1);

			while (--spec.width > 0)
				serial_write_chars(" ", 1);

			break;
		}

		case FORMAT_STR: {
			const char *s = va_arg(ap, char*);
			string(s, spec);
			break;
		}

		case FORMAT_PTR: {
			void *ptr = va_arg(ap, void*);
			if (ptr == NULL) {
				string("(null)", spec); // use len above
				break;
			}

			spec.width = 2 * sizeof(void*) + 2;
			spec.flags |= FLAG_ALT | FLAG_ZERO;
			spec.base = 16;

			number((unsigned long)ptr, spec);
			break;
		}

		case FORMAT_PERCENT:
			serial_write_chars("%", 1);
			break;

		default: {
			uintmax_t num = 0;
			if (spec.flags & FLAG_SIGNED) {
				switch (spec.type) {
				case FORMAT_BYTE: num = (signed char)va_arg(ap, int); break;
				case FORMAT_SHORT: num = (short)va_arg(ap, int); break;
				case FORMAT_INT: num = va_arg(ap, int); break;
				case FORMAT_LONG: num = va_arg(ap, long); break;
				case FORMAT_LLONG: num = va_arg(ap, long long); break;
				case FORMAT_SIZE: num = va_arg(ap, size_t); break;
				}
			} else {
				switch (spec.type) {
				case FORMAT_BYTE: num = (unsigned char)va_arg(ap, int); break;
				case FORMAT_SHORT: num = (unsigned short)va_arg(ap, int); break;
				case FORMAT_INT: num = va_arg(ap, unsigned int); break;
				case FORMAT_LONG: num = va_arg(ap, unsigned long); break;
				case FORMAT_LLONG: num = va_arg(ap, unsigned long long); break;
				case FORMAT_SIZE: num = va_arg(ap, size_t); break;
				case FORMAT_PTRDIFF: num = va_arg(ap, ptrdiff_t); break;
				}
			}

			number(num, spec);
			break;
		}
		}
	}
}
