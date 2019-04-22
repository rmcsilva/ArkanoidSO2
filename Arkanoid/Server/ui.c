#include "stdafx.h"
#include "ui.h"
#include "messages.h"

int initialMenu()
{
	int option;
	_tprintf(TEXT("1 - Start the game\n"));
	_tprintf(TEXT("2 - Show player top 10\n"));
	_tprintf(TEXT("3 - List connected users\n"));
	_tprintf(TEXT("4 - Shutdown server\n"));
	_tprintf(TEXT("Enter your option >> "));

	option = readInt();
	switch (option)
	{
		case 1: return START_GAME;
		case 2: return SHOW_TOP10;
		case 3: return LIST_USERS;
		case 4: return SHUTDOWN;
		default: return -1;
	}
}

void showResponseMessageInformation(ServerMessage serverMessage, int requestType)
{
	_tprintf(TEXT("\n\n"));
	switch (requestType)
	{
		case LOGIN_REQUEST:
			_tprintf(TEXT("Login Request\n"));
			break;
		case TOP10:
			_tprintf(TEXT("User requested the top10\n"));
		default:
			_tprintf(TEXT("Invalid request!\n\n"));
	}

	_tprintf(TEXT("User: %s\n"), serverMessage.username);
	_tprintf(TEXT("ID: %d\n"), serverMessage.id);

	if(serverMessage.type == REQUEST_ACCEPTED)
	{
		_tprintf(TEXT("Request Acepted!\n"));
	} else if (serverMessage.type == REQUEST_DENIED)
	{
		_tprintf(TEXT("Request Denied!\n"));
	}
}

int readInt() {
	int integer, end = 0;
	TCHAR tmp;
	while (end < 1) {
		_tscanf_s(TEXT(" %d"), &integer, 1) == 1 ? end++ : 0;
		_tscanf_s(TEXT("%c"), &tmp, 1);
	}
	return integer;
}