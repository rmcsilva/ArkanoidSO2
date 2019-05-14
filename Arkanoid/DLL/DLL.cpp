// DLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include "DLL.h"
#include "setup.h"

//Indicate if user is local or remote
BOOL isLocalUser = TRUE;

TCHAR username[TAM];
int id = UNDEFINED_ID;
BOOL inGame = FALSE;

TCHAR top10[TOP10_SIZE];

//Shared Memory Variables
//Memory Map
HANDLE hClientRequestMemoryMap;
HANDLE hServerResponseMemoryMap;
HANDLE hGameDataMemoryMap;

ClientMessageControl* pClientRequestMemory;
ServerMessageControl* pServerResponseMemory;
GameData* pGameDataMemory;

//Mutex
HANDLE hClientRequestMutex;
HANDLE hServerResponseMutex;

//Semaphore
HANDLE hClientRequestSemaphoreItems;
HANDLE hClientRequestSemaphoreEmpty;

HANDLE hServerResponseSemaphoreItems;
HANDLE hServerResponseSemaphoreEmpty;

//Event
HANDLE hClientMessageCheckEvent;
HANDLE hGameUpdateEvent;

//Named Pipe Variables
TCHAR pipeClientRequestsName[MAX];
TCHAR pipeServerResponsesName[MAX];
TCHAR pipeGameName[MAX];

HANDLE hClientRequestPipe;
HANDLE hServerResponsePipe;
HANDLE hGamePipe;

int login(TCHAR* loginUsername, TCHAR* ip)
{
	if(ip == NULL)
	{
		return loginSharedMemory(loginUsername);
	} else
	{
		isLocalUser = FALSE;

		int bufferSize = MAX - 1;
		_stprintf_s(pipeClientRequestsName, bufferSize, NAMED_PIPE_CLIENT_REQUESTS, ip);
		_stprintf_s(pipeServerResponsesName, bufferSize, NAMED_PIPE_SERVER_RESPONSES, ip);
		_stprintf_s(pipeGameName, bufferSize, NAMED_PIPE_GAME, ip);

		_tprintf(TEXT("Client pipe: %s\nServer pipe: %s\nGame pipe: %s\n"), pipeClientRequestsName, pipeServerResponsesName, pipeGameName);
		
		return loginNamedPipe(loginUsername);
	}
}

void sendMessage(int messageType)
{
	if(isLocalUser == TRUE)
	{
		return sendMessageSharedMemory(messageType);
	} else
	{
		return sendMessageNamedPipe(messageType);
	}
}

int receiveMessage(int messageType)
{
	if(isLocalUser == TRUE)
	{
		return receiveMessageSharedMemory(messageType);
	} else
	{
		return receiveMessageNamedPipe(messageType);
	}
}

int receiveBroadcast()
{
	if(isLocalUser == TRUE)
	{
		WaitForSingleObject(hGameUpdateEvent, INFINITE);
		_tprintf(TEXT("\nBall position x: %d y: %d\n"), pGameDataMemory->ball[0].position.x, pGameDataMemory->ball[0].position.y);
		return pGameDataMemory->gameStatus;
	} else
	{
		DWORD nBytes;
		GameData gameData;

		BOOL fSuccess = ReadFile(
			hGamePipe,				// pipe handle 
			&gameData,				// buffer to receive reply 
			sizeof(gameData),		// size of buffer 
			&nBytes,				// number of bytes read 
			NULL);					// not overlapped 

		//TODO: Add verifications

		_tprintf(TEXT("\nBall position x: %d y: %d\n"), gameData.ball[0].position.x, gameData.ball[0].position.y);

		return gameData.gameStatus;
		//TODO: Add pipe broadcast
	}
	
}

void logout()
{
	if(isLocalUser == TRUE)
	{
		return logoutSharedMemory();
	} else
	{
		return logoutNamedPipe();
	}
}

int loginSharedMemory(TCHAR* loginUsername)
{
	openClientsSharedMemory(&hClientRequestMemoryMap);
	openServersSharedMemory(&hServerResponseMemoryMap);
	openGameSharedMemory(&hGameDataMemoryMap);

	pClientRequestMemory = mapClientsSharedMemory(&hClientRequestMemoryMap, sizeof(ClientMessageControl));
	pServerResponseMemory = mapServersSharedMemory(&hServerResponseMemoryMap, sizeof(ServerMessageControl));
	pGameDataMemory = mapReadOnlyGameSharedMemory(&hGameDataMemoryMap, sizeof(GameData));

	if (pClientRequestMemory == NULL || pServerResponseMemory == NULL || pGameDataMemory == NULL)
	{
		_tprintf(TEXT("Error connecting to server!\n"));
		return -1;
	}

	createClientsRequestMutex(&hClientRequestMutex);
	createServersResponseMutex(&hServerResponseMutex);

	if (hServerResponseMutex == NULL || hClientRequestMutex == NULL)
	{
		_tprintf(TEXT("Error creating mutex!\n"));
		return -1;
	}

	openClientsRequestSemaphores(&hClientRequestSemaphoreItems, &hClientRequestSemaphoreEmpty);
	openServersResponseSemaphores(&hServerResponseSemaphoreItems, &hServerResponseSemaphoreEmpty);

	if (hClientRequestSemaphoreItems == NULL || hClientRequestSemaphoreEmpty == NULL || hServerResponseSemaphoreItems == NULL || hServerResponseSemaphoreEmpty == NULL)
	{
		_tprintf(TEXT("Error opening semaphores!\n"));
		return -1;
	}

	createClientMessageCheckEvent(&hClientMessageCheckEvent);
	openGameUpdateEvent(&hGameUpdateEvent);

	if (hClientMessageCheckEvent == NULL || hGameUpdateEvent == NULL)
	{
		_tprintf(TEXT("Error creating/opening event!\n"));
		return -1;
	}

	_tcscpy_s(username, TAM, loginUsername);

	sendMessageSharedMemory(LOGIN_REQUEST);

	return 1;
}

void sendMessageSharedMemory(int messageType)
{
	WaitForSingleObject(hClientRequestSemaphoreEmpty, INFINITE);
	WaitForSingleObject(hClientRequestMutex, INFINITE);

	int position = pClientRequestMemory->clientInput;
	ClientMessage* clientRequest = &pClientRequestMemory->clientMessageBuffer[position];

	clientRequest->type = messageType;
	_tcscpy_s(clientRequest->username, TAM, username);
	clientRequest->id = id;

	pClientRequestMemory->clientInput = (position + 1) % BUFFER_SIZE;
	ReleaseMutex(hClientRequestMutex);
	ReleaseSemaphore(hClientRequestSemaphoreItems, 1, NULL);
}

int receiveMessageSharedMemory(int messageType)
{
	while (TRUE)
	{
		WaitForSingleObject(hServerResponseSemaphoreItems, INFINITE);
		WaitForSingleObject(hServerResponseMutex, INFINITE);

		int position = pServerResponseMemory->clientOutput;
		ServerMessage* serverMessage = &pServerResponseMemory->serverMessageBuffer[position];

		//Only reads messages for the corresponding client
		if (pServerResponseMemory->counter == pServerResponseMemory->numUsers || serverMessage->id == id || messageType == LOGIN_REQUEST || serverMessage->type == LOGOUT)
		{
			//_tprintf(TEXT("Server Response\nUsername: %s\nID: %d\n"), serverMessage->username, serverMessage->id);
			switch (messageType)
			{
			case LOGIN_REQUEST:
				if (_tcscmp(username, serverMessage->username) == 0)
				{
					if (serverMessage->type == REQUEST_ACCEPTED)
					{
						id = serverMessage->id;
					}

				}
				else
				{
					pServerResponseMemory->counter++;
					waitForResponseOnEvent();
					continue;
				}
				break;
			case TOP10:
				_tcscpy_s(top10, TOP10_SIZE, serverMessage->content);
				break;
			case LOGOUT:
				if (serverMessage->type == LOGOUT)
				{
					ReleaseMutex(hServerResponseMutex);
					ReleaseSemaphore(hServerResponseSemaphoreItems, 1, NULL);
					SetEvent(hClientMessageCheckEvent);
					return LOGOUT;
				}
				else
				{
					waitForResponseOnEvent();
					continue;
				}
			}

			pServerResponseMemory->counter = 0;
			SetEvent(hClientMessageCheckEvent);

			pServerResponseMemory->clientOutput = (position + 1) % BUFFER_SIZE;
			ReleaseMutex(hServerResponseMutex);
			ReleaseSemaphore(hServerResponseSemaphoreEmpty, 1, NULL);
			ResetEvent(hClientMessageCheckEvent);

			return serverMessage->type;
		}
		else
		{
			pServerResponseMemory->counter++;
			waitForResponseOnEvent();
		}
	}
}

void waitForResponseOnEvent()
{
	ReleaseMutex(hServerResponseMutex);
	ReleaseSemaphore(hServerResponseSemaphoreItems, 1, NULL);
	//_tprintf(TEXT("Waiting for the response: %d\n"), pServerResponseMemory->counter);
	//Waits on the event until a client reads his response or the response gets ignored if its for a client that logged out
	WaitForSingleObject(hClientMessageCheckEvent, INFINITE);
}

void logoutSharedMemory()
{
	if (id != UNDEFINED_ID)
	{
		sendMessageSharedMemory(LOGOUT);
	}

	UnmapViewOfFile(pClientRequestMemory);
	UnmapViewOfFile(pServerResponseMemory);
	UnmapViewOfFile(pGameDataMemory);

	CloseHandle(hClientRequestMemoryMap);
	CloseHandle(hServerResponseMemoryMap);
	CloseHandle(hGameDataMemoryMap);

	CloseHandle(hClientRequestMutex);
	CloseHandle(hServerResponseMutex);

	CloseHandle(hClientRequestSemaphoreItems);
	CloseHandle(hClientRequestSemaphoreEmpty);
	CloseHandle(hServerResponseSemaphoreItems);
	CloseHandle(hServerResponseSemaphoreEmpty);

	CloseHandle(hClientMessageCheckEvent);
	CloseHandle(hGameUpdateEvent);
}

int loginNamedPipe(TCHAR* loginUsername)
{
	//Client Requests -> write access
	openNamedPipe(&hClientRequestPipe, pipeClientRequestsName, GENERIC_WRITE);

	if(hClientRequestPipe == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() != ERROR_PIPE_BUSY)
		{
			_tprintf(TEXT("Could not open client request pipe. GLE=%d\n"), GetLastError());
			return -1;
		} else
		{
			_tprintf(TEXT("Server player limit reached! Try again later!\n"));
			return -1;
		}
	}
	
	//Server Responses -> read access 
	openNamedPipe(&hServerResponsePipe, pipeServerResponsesName, GENERIC_READ);

	if (hServerResponsePipe == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() != ERROR_PIPE_BUSY)
		{
			_tprintf(TEXT("Could not open server response pipe. GLE=%d\n"), GetLastError());
			return -1;
		} else
		{
			_tprintf(TEXT("Server player limit reached! Try again later!\n"));
			return -1;
		}
	}

	//Game Data -> read access 
	openNamedPipe(&hGamePipe, pipeGameName, GENERIC_READ);

	if (hGamePipe == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() != ERROR_PIPE_BUSY)
		{
			_tprintf(TEXT("Could not open game data pipe. GLE=%d\n"), GetLastError());
			return -1;
		} else
		{
			_tprintf(TEXT("Server player limit reached! Try again later!\n"));
			return -1;
		}
	}

	// The pipe connected; change to message-read mode. 

	if (changePipeToMessageMode(hClientRequestPipe) == FALSE)
	{
		return -1;
	}

	//if (changePipeToMessageMode(hServerResponsePipe) == FALSE)
	//{
	//	return -1;
	//}

	//if (changePipeToMessageMode(hGamePipe) == FALSE)
	//{
	//	return -1;
	//}

	sendMessageNamedPipe(LOGIN_REQUEST);

	return 1;
}

void sendMessageNamedPipe(int messageType)
{
	DWORD nBytes;
	ClientMessage clientRequest;

	clientRequest.type = messageType;
	_tcscpy_s(clientRequest.username, TAM, username);
	clientRequest.id = id;

	BOOL fSuccess = WriteFile(
		hClientRequestPipe,     // pipe handle 
		&clientRequest,         // message 
		sizeof(clientRequest),  // message length 
		&nBytes,				// bytes written 
		NULL);                  // not overlapped 

	if (!fSuccess)
	{
		_tprintf(TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError());
		return;
	}
}

int receiveMessageNamedPipe(int messageType)
{
	DWORD nBytes;
	ServerMessage serverResponse;

	BOOL fSuccess = ReadFile(
		hServerResponsePipe,    // pipe handle 
		&serverResponse,		// buffer to receive reply 
		sizeof(serverResponse), // size of buffer 
		&nBytes,				// number of bytes read 
		NULL);					// not overlapped 

	//TODO: Add verifications

	switch (messageType)
	{
		case LOGIN_REQUEST:
			if (serverResponse.type == REQUEST_ACCEPTED)
			{
				id = serverResponse.id;
			}
			break;
		case TOP10:
			_tcscpy_s(top10, TOP10_SIZE, serverResponse.content);
			break;
		case LOGOUT:
			if (serverResponse.type == LOGOUT)
			{
				return LOGOUT;
			}
	}

	return serverResponse.type;
}

void logoutNamedPipe()
{
	if (id != UNDEFINED_ID)
	{
		sendMessageNamedPipe(LOGOUT);
	}

	CloseHandle(hClientRequestPipe);
	CloseHandle(hServerResponsePipe);
	CloseHandle(hGamePipe);
}