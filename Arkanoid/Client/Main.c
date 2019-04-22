#include "ui.h"
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include "DLL.h"


int main(int argc, char* argv[])
{

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

	if(loginUser(username) == -1)
	{
		_gettchar();
		_gettchar();
		return -1;
	}

	receiveMessage(LOGIN_REQUEST);

	_gettchar();
	_gettchar();

	logout();
}
