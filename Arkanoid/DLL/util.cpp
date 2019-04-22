#include "stdafx.h"
#include "util.h"

int readInt() {
	int integer, end = 0;
	TCHAR tmp;
	while (end < 1) {
		_tscanf_s(TEXT(" %d"), &integer, 1) == 1 ? end++ : 0;
		_tscanf_s(TEXT("%c"), &tmp, 1);
	}
	return integer;
}