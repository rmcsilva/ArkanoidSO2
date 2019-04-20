// DLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include "DLL.h"
#include "setup.h"

int isLocalUser = 1;

TCHAR username[TAM];
int id = -1;

//Memory Map
HANDLE hClientRequestMemoryMap;
HANDLE hServerResponseMemoryMap;

ClientMessageControl* pClientRequestMemory;
ServerMessageControl* pServerResponseMemory;

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

int test(void)
{
	_tprintf(TEXT("Hello\n"));
	return 123;
}

int login(TCHAR* loginUsername)
{
	openClientsSharedMemory(&hClientRequestMemoryMap);
	openServersSharedMemory(&hServerResponseMemoryMap);
	
	pClientRequestMemory = mapClientsSharedMemory(&hClientRequestMemoryMap, sizeof(ClientMessageControl));
	pServerResponseMemory = mapServersSharedMemory(&hServerResponseMemoryMap, sizeof(ServerMessageControl));

	if(pClientRequestMemory == NULL || pServerResponseMemory == NULL)
	{
		_tprintf(TEXT("Error connecting to server!\n"));
		return -1;
	}

	_tprintf(TEXT("Client Request Memory Value: %d\n"), pClientRequestMemory->clientInput);
	_tprintf(TEXT("Server Request Memory Value: %d\n"), pServerResponseMemory->clientOutput);

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

	if (hClientMessageCheckEvent == NULL)
	{
		_tprintf(TEXT("Error creating event!\n"));
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

	//Only copies the username on the login then it uses the ID
	if (messageType == LOGIN_REQUEST)
		_tcscpy_s(clientRequest->username, TAM, username);
	else
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

		if (pServerResponseMemory->counter == pServerResponseMemory->numUsers || serverMessage->id == id || messageType == LOGIN_REQUEST)
		{
			_tprintf(TEXT("Server Response\nUsername: %s\nID: %d\n"), serverMessage->username, serverMessage->id);
			
			if (messageType == LOGIN_REQUEST && _tcscmp(username, serverMessage->username) == 0)
			{
				id = serverMessage->id;

				if(pServerResponseMemory->counter > 0)
				{
					pServerResponseMemory->counter = 0;
					SetEvent(hClientMessageCheckEvent);
				}
			}

			pServerResponseMemory->clientOutput = (position + 1) % BUFFER_SIZE;
			ReleaseMutex(hServerResponseMutex);
			ReleaseSemaphore(hServerResponseSemaphoreEmpty, 1, NULL);
			ResetEvent(hClientMessageCheckEvent);

			return serverMessage->type;
		}
		else
		{
			pServerResponseMemory->counter++;
			_tprintf(TEXT("Waiting for the response: %d\n"), pServerResponseMemory->counter);
			ReleaseMutex(hServerResponseMutex);
			ReleaseSemaphore(hServerResponseSemaphoreItems, 1, NULL);
			//Waits on the event until a client reads his response or the response gets ignored if its for a client that logged out
			WaitForSingleObject(hClientMessageCheckEvent, INFINITE);
		}
	}
}

void logout()
{
	//TODO: Send Response To Server
	UnmapViewOfFile(pClientRequestMemory);
	UnmapViewOfFile(pServerResponseMemory);

	CloseHandle(hClientRequestMemoryMap);
	CloseHandle(hServerResponseMemoryMap);

	CloseHandle(hClientRequestMutex);
	CloseHandle(hServerResponseMutex);

	CloseHandle(hClientRequestSemaphoreItems);
	CloseHandle(hClientRequestSemaphoreEmpty);
	CloseHandle(hServerResponseSemaphoreItems);
	CloseHandle(hServerResponseSemaphoreEmpty);
}