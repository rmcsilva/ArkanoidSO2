#include "stdafx.h"
#include "util.h"

int readInt() {
	int integer, end = 0;
	TCHAR tmp;
	while (end < 1) {
		_tscanf_s(TEXT(" %d"), &integer) == 1 ? end++ : 0;
		_tscanf_s(TEXT("%c"), &tmp, 1);
	}
	return integer;
}

TCHAR* trimWhiteSpace(TCHAR* str)
{
	TCHAR* end;

	// Trim leading space
	while (_istspace((TCHAR)*str)) str++;

	if (*str == 0)  // All spaces?
		return str;

	// Trim trailing space
	end = str + _tcslen(str) - 1;
	while (end > str && _istspace((TCHAR)*end)) end--;

	// Write new null terminator character
	end[1] = '\0';

	return str;
}