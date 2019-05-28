#include "ui.h"
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include "DLL.h"

DWORD WINAPI GameUpdate(LPVOID lpParam);
void serverShutdown();

BOOL keepAlive = TRUE;
BOOL serverLogout = FALSE;

int _tmain(int argc, TCHAR* argv[])
{
	HANDLE hGameThread;
	DWORD dwGameThreadId;

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
			case LOGOUT:
				keepAlive = FALSE;
				break;
			default:
				break;
		}
	}
	while (keepAlive != FALSE);

	TerminateThread(hGameThread, 1);
	WaitForSingleObject(hGameThread, INFINITE);

	if(serverLogout == FALSE)
	{
		logout();
	}
}

DWORD WINAPI GameUpdate(LPVOID lpParam)
{
	GameData gameData;
	int status;

	while (TRUE)
	{
		gameData = receiveBroadcast();
		status = gameData.gameStatus;

		if(status == GAME_OVER)
		{
			break;
		} else if (status == LOGOUT)
		{ 
			serverShutdown();
			inGame = FALSE;
			return -1;
		}
		//TODO: Update GUI
		//Export the game memory pointer and update?
	}

	inGame = FALSE;

	//Receives the top10
	sendMessage(TOP10);
	receiveMessage(TOP10);
	showTop10();

	return 1;
}

void serverShutdown()
{
	serverLogout = TRUE;
	keepAlive = FALSE;

	//Simulates a key press of 9 + ENTER causing the gets to trigger and the main thread to end
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD dwTmp;
	INPUT_RECORD ir[2];

	ir[0].EventType = KEY_EVENT;
	ir[0].Event.KeyEvent.bKeyDown = TRUE;
	ir[0].Event.KeyEvent.dwControlKeyState = MAPVK_VK_TO_VSC;
	ir[0].Event.KeyEvent.uChar.UnicodeChar = '9';
	ir[0].Event.KeyEvent.wRepeatCount = 1;
	ir[0].Event.KeyEvent.wVirtualKeyCode = '9';
	ir[0].Event.KeyEvent.wVirtualScanCode = MapVirtualKey('9', MAPVK_VK_TO_VSC);

	ir[1].EventType = KEY_EVENT;
	ir[1].Event.KeyEvent.bKeyDown = TRUE;
	ir[1].Event.KeyEvent.dwControlKeyState = MAPVK_VK_TO_VSC;
	ir[1].Event.KeyEvent.uChar.UnicodeChar = VK_RETURN;
	ir[1].Event.KeyEvent.wRepeatCount = 1;
	ir[1].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
	ir[1].Event.KeyEvent.wVirtualScanCode = MapVirtualKey(VK_RETURN, MAPVK_VK_TO_VSC);

	WriteConsoleInput(hStdin, ir, 2, &dwTmp);
}