#include <ctype.h>

int isalpha(int c) {
	return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

int isdigit(int c) {
	return '0' <= c && c <= '9';
}

int islower(int c) {
	return 'a' <= c && c <= 'z';
}

int isprint(int c) {
	return c > 0x1f && c < 0x7f;
}

int isspace(int c) {
	return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

int isupper(int c) {
	return 'A' <= c && c <= 'Z';
}

int isxdigit(int c) {
	return isdigit(c) || ('A' <= toupper(c) && toupper(c) <= 'F');
}

int toupper(int c) {
	if ('a' <= c && c <= 'z')
		return c - 32;
	else
		return c;
}

int tolower(int c) {
	if ('A' <= c && c <= 'Z')
		return c + 32;
	else
		return c;
}

