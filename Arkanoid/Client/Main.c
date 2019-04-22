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
		return -1;
	}

	if(receiveMessage(LOGIN_REQUEST) == REQUEST_ACCEPTED)
	{
		_tprintf(TEXT("Sucessufully connected to the server!\n\n"));
	}
	else
	{
		_tprintf(TEXT("Could not connect to the server...\nTry again!"));
		_gettchar();
		return -1;
	}

	int option;

	do
	{
		option = initialMenu();

		switch (option)
		{
			case TOP10:
				//TODO: Send message requesting top 10
				break;
			default:
				break;
		}
	}
	while (option != LOGOUT);

	logout();
}
