// DLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include "DLL.h"
#include "setup.h"

//Indicate if user is local or remote
int isLocalUser = 1;

TCHAR username[TAM];
int id = -1;
int inGame = 0;

TCHAR top10[TOP10_SIZE];

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

int login(TCHAR* loginUsername)
{
	openClientsSharedMemory(&hClientRequestMemoryMap);
	openServersSharedMemory(&hServerResponseMemoryMap);
	openGameSharedMemory(&hGameDataMemoryMap);
	
	pClientRequestMemory = mapClientsSharedMemory(&hClientRequestMemoryMap, sizeof(ClientMessageControl));
	pServerResponseMemory = mapServersSharedMemory(&hServerResponseMemoryMap, sizeof(ServerMessageControl));
	pGameDataMemory = mapReadOnlyGameSharedMemory(&hGameDataMemoryMap, sizeof(GameData));

	if(pClientRequestMemory == NULL || pServerResponseMemory == NULL || pGameDataMemory == NULL)
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

	sendMessage(LOGIN_REQUEST);

	return 1;
}

void sendMessage(int messageType)
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

int receiveMessage(int messageType)
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
					if(_tcscmp(username, serverMessage->username) == 0)
					{
						if(serverMessage->type == REQUEST_ACCEPTED)
						{
							id = serverMessage->id;
						}
						
					} else
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
					if(serverMessage->type == LOGOUT)
					{
						ReleaseMutex(hServerResponseMutex);
						ReleaseSemaphore(hServerResponseSemaphoreItems, 1, NULL);
						SetEvent(hClientMessageCheckEvent);
						return LOGOUT;
					} else
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

int receiveBroadcast()
{
	WaitForSingleObject(hGameUpdateEvent, INFINITE);
	_tprintf(TEXT("\nBall position x: %d y: %d\n"), pGameDataMemory->ball[0].position.x, pGameDataMemory->ball[0].position.y);
	return pGameDataMemory->gameStatus;
}

void logout()
{
	if(id != -1)
	{
		sendMessage(LOGOUT);
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