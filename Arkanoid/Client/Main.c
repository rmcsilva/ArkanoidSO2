#include "ui.h"
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include "DLL.h"

DWORD WINAPI GameUpdate(LPVOID lpParam);

HANDLE hGameThread;
DWORD dwGameThreadId;

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
		logout();
		return -1;
	}


	//Thread to handle the game updates
	hGameThread = CreateThread(
		NULL,						// default security attributes
		0,							// use default stack size  
		GameUpdate,					// thread function name
		NULL,						// argument to thread function 
		0,							// use default creation flags 
		&dwGameThreadId);			// returns the thread identifier 

	int option;

	do
	{
		option = initialMenu();

		switch (option)
		{
			case TOP10:
				if(!inGame)
				{
					sendMessage(TOP10);
					receiveMessage(TOP10);
					showTop10();
				}
				break;
			default:
				break;
		}
	}
	while (option != LOGOUT);

	TerminateThread(hGameThread, 1);
	WaitForSingleObject(hGameThread, INFINITE);

	logout();
}

DWORD WINAPI GameUpdate(LPVOID lpParam)
{

	while (receiveBroadcast() != GAME_OVER)
	{
		//TODO: Update GUI
		//Export the game memory pointer and update?
	}

	inGame = IN_LOBBY;

	//Receives the top10
	sendMessage(TOP10);
	receiveMessage(TOP10);
	showTop10();

	return 1;
}