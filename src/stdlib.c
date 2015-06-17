#include <stdlib.h>
#include <ctype.h>
#include <stddef.h>
#include <stdbool.h>

unsigned long int strtoul(const char *str, const char **end, int base) {
	const char *s = str;
	int c;

	do {
		c = *s++;
	} while (isspace(c));

	bool negative = false;
	if (c == '-') {
		negative = true;
		c = *s++;
	} else if (c == '+') {
		c = *s++;
	}

	if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	} else if ((base == 0 || base == 2) && c == '0' && (*s == 'b' || *s == 'B')) {
		c = s[1];
		s += 2;
		base = 2;
	} else if (base == 0)
		base = c == '0' ? 8 : 10;

	unsigned long int x;
	bool any = false;
	for (x = 0; ; c = *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;

		if (c >= base)
			break;

		any = true;
		x = x * base + c;
	}

	if (negative)
		x = -x;
	
	if (end != NULL)
		*end = any ? s - 1 : str;

	return x;
}
