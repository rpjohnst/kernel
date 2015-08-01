#include <string.h>

/// mem

int memcmp(const void *aptr, const void *bptr, size_t n) {
	const unsigned char *a = aptr, *b = bptr;
	for (size_t i = 0; i < n; i++) {
		if (a[i] < b[i]) return -1;
		else if (a[i] > b[i]) return 1;
	}
	return 0;
}

void *memcpy(void *dest, const void *src, size_t n) {
	const unsigned char *s = src;
	unsigned char *d = dest;
	while (n--)
		*d++ = *s++;
	return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
	const unsigned char *s = src;
	unsigned char *d = dest;

	if (dest <= src) {
		while (n--)
			*d++ = *s++;
	} else {
		s += n;
		d += n;
		while (n--)
			*--d = *--s;
	}

	return dest;
}

void *memset(void *s, int c, size_t n) {
	unsigned char *d = s;
	while (n--)
		*d++ = c;
	return s;
}

/// str

char *strcat(char *dest, const char *src) {
	size_t i = strlen(dest), j = 0;
	for (; src[j] != '\0'; j++)
		dest[i + j] = src[j];
	dest[i + j] = '\0';
	return dest;
}

int strcmp(const char *s1, const char *s2) {
	for (size_t i = 0; ; i++) {
		if (s1[i] < s2[i]) return -1;
		else if (s1[i] > s2[i]) return 1;
		else if (s1[i] == '\0') return 0;
	}
	return 0;
}

char *strcpy(char *dest, const char *src) {
	while ((*dest++ = *src++) != '\0');
	return dest;
}

size_t strlen(const char *str) {
	size_t len = 0;
	while (*str++)
		len++;
	return len;
}

int strncmp(const char *s1, const char *s2, size_t n) {
	for (size_t i = 0; i < n; i++) {
		if (s1[i] < s2[i]) return -1;
		else if (s1[i] > s2[i]) return 1;
		else if (s1[i] == '\0') return 0;
	}
	return 0;
}

char *strncpy(char *dest, const char *src, size_t n) {
	while (n-- && (*dest++ = *src++) != '\0');
	return dest;
}

size_t strnlen(const char *str, size_t n) {
	size_t len = 0;
	while (len < n && *str++)
		len++;
	return len;
}
