// DLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include "DLL.h"
#include "setup.h"

//Indicate if user is local or remote
BOOL isLocalUser = TRUE;

TCHAR username[TAM];
int id = -1;
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
		//TODO: Add pipe login
	}
}

void sendMessage(int messageType)
{
	if(isLocalUser == TRUE)
	{
		return sendMessageSharedMemory(messageType);
	} else
	{
		//TODO: Add message by pipe
	}
}

int receiveMessage(int messageType)
{
	if(isLocalUser == TRUE)
	{
		return receiveMessageSharedMemory(messageType);
	} else
	{
		//TODO: Receive message by pipe
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
		//TODO: add pipe logout
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

	//Shared memory test
	//_tprintf(TEXT("Client Request Memory Value: %d\n"), pClientRequestMemory->clientInput);
	//_tprintf(TEXT("Server Request Memory Value: %d\n"), pServerResponseMemory->clientOutput);

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
	if (id != -1)
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