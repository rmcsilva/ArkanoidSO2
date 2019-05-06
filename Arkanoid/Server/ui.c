#include "stdafx.h"
#include "ui.h"
#include "messages.h"
#include "util.h"

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
			break;
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

void showTopPlayers(TopPlayer* topPlayers, DWORD playerCount)
{
	_tprintf(TEXT("\nTop Players:\n"));

	for(int i=0; i < (int)playerCount; i++)
	{
		_tprintf(TEXT("Top %d\n"), i+1);
		_tprintf(TEXT("Username: %s\nScore: %d\n\n"), topPlayers[i].username, topPlayers[i].topScore);
	}
}

void showConnectedUsers(Player* users, int connectedUsers)
{
	if(connectedUsers == 0)
	{
		_tprintf(TEXT("\nThere are no connected users!\n\n"));
	} else
	{
		_tprintf(TEXT("\nConnected Users:\n"));
	}

	for(int i=0; i < connectedUsers; i++)
	{
		_tprintf(TEXT("Username: %s\nId: %d\n"), users[i].username, users[i].id);
		if(users[i].inGame == TRUE)
		{
			_tprintf(TEXT("User is in game!\n"));
			_tprintf(TEXT("Lives: %d\nScore: %d\n"), users[i].lives, users[i].score);
		} else
		{
			_tprintf(TEXT("User is not in game!\n\n"));
		}		
	}
}