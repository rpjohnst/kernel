#include <stdnoreturn.h>

#define assert(x) do { if (!(x)) \
	panic("assert: %s:%d/%s(): %s\n", __FILE__, __LINE__, __func__, #x); \
} while (0)

noreturn void panic(const char *fmt, ...) __attribute__((format (printf, 1, 2)));
