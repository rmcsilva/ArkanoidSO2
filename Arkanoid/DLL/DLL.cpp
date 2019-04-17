// DLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include "DLL.h"

int isLocalUser = 1;

int test(void)
{
	_tprintf(TEXT("Hello\n"));
	return 123;
}

