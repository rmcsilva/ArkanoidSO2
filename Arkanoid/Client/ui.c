#include "ui.h"

int loginUser(TCHAR username[])
{
	_tprintf(TEXT("Enter your username > "));
	_getts_s(username, TAM-1);
	_tprintf(TEXT("Hello %s\nWelcome to Arkanoid!\n"), username);

	return login(username);
}