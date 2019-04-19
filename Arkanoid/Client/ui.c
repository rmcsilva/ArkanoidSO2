#include "ui.h"

void loginUser(TCHAR username[])
{
	_tprintf(TEXT("Enter your username > "));
	_tscanf_s(TEXT("%s"), username, TAM);
	_tprintf(TEXT("Hello %s\nWelcome to Arkanoid!\n"), username);

	login(username);
}
