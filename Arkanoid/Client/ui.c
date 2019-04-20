#include "ui.h"

int loginUser(TCHAR username[])
{
	_tprintf(TEXT("Enter your username > "));
	_tscanf_s(TEXT("%s"), username, TAM);
	_tprintf(TEXT("Hello %s\nWelcome to Arkanoid!\n"), username);

	return login(username);
}