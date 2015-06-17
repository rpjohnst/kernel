#include <kprintf.h>
#include <assert.h>
#include <stdarg.h>

void panic(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	kvprintf(fmt, ap);

	va_end(ap);

	for (;;);
}
