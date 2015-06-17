#include <stdarg.h>

void kprintf(const char *fmt, ...) __attribute__((format (printf, 1, 2)));
void kvprintf(const char *fmt, va_list ap);
