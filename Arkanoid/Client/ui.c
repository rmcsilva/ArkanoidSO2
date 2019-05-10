#include "ui.h"
#include "DLL.h"
#include "util.h"

int loginUser(TCHAR username[])
{
	_tprintf(TEXT("Enter your username > "));
	_getts_s(username, TAM-1);

	username = trimWhiteSpace(username);

	_tprintf(TEXT("Hello %s\n%hs"), username, CLIENT_WELCOME_MESSAGE);

	return login(username);
}

int initialMenu()
{
	int option;
	if(!inGame)
	{
		_tprintf(TEXT("1 - Show player top 10\n"));
		_tprintf(TEXT("2 - Shutdown client\n"));
		_tprintf(TEXT("Enter your option >> "));
	}
	
	option = readInt();
	switch (option)
	{
		case 1: return TOP10;
		case 2: return LOGOUT;
		default: return -1;
	}
}

void showTop10()
{
	TCHAR* nextToken = NULL;
	TCHAR value[TOP10_SIZE];
	_tcscpy_s(value, TOP10_SIZE, top10);
	TCHAR* token = _tcstok_s(value, TEXT(";"), &nextToken);
	int counter = 1;

	while (token != NULL)
	{
		//Copy username
		_tprintf(TEXT("Top %d\n"), counter);
		_tprintf(TEXT("Username: %s\n"), token);
		//Get score
		token = _tcstok_s(NULL, TEXT(";"), &nextToken);
		_tprintf(TEXT("Score: %s\n\n"), token);

		token = _tcstok_s(NULL, TEXT(";"), &nextToken);

		counter++;
	}
}
