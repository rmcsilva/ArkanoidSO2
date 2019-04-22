#include "ui.h"
#include "DLL.h"
#include "util.h"

int loginUser(TCHAR username[])
{
	_tprintf(TEXT("Enter your username > "));
	_getts_s(username, TAM-1);
	_tprintf(TEXT("Hello %s\n%hs"), username, CLIENT_WELCOME_MESSAGE);

	return login(username);
}

int initialMenu()
{
	int option;
	_tprintf(TEXT("1 - Show player top 10\n"));
	_tprintf(TEXT("2 - Shutdown client\n"));
	_tprintf(TEXT("Enter your option >> "));

	option = readInt();
	switch (option)
	{
		case 1: return TOP10;
		case 2: return LOGOUT;
		default: return -1;
	}
}