#include "ui.h"
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include "DLL.h"


int main(int argc, char* argv[])
{
	TCHAR username[TAM];

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

	loginUser(username);

	_gettchar();
	_gettchar();

	logout();
}
