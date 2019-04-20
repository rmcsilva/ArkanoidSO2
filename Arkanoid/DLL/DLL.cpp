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
	//TODO: Only copy on login then use the ID
	_tcscpy_s(clientRequest->username, TAM, username);

	pClientRequestMemory->clientInput = (position + 1) % BUFFER_SIZE;
	ReleaseMutex(hClientRequestMutex);
	ReleaseSemaphore(hClientRequestSemaphoreItems, 1, NULL);
}

int receiveMessage()
{
	WaitForSingleObject(hServerResponseSemaphoreItems, INFINITE);
	WaitForSingleObject(hServerResponseMutex, INFINITE);

	int position = pServerResponseMemory->clientOutput;
	pServerResponseMemory->counter++;

	ServerMessage* serverMessage = &pServerResponseMemory->serverMessageBuffer[position];

	if(pServerResponseMemory->counter == pServerResponseMemory->numUsers || serverMessage->id == id)
	{
		_tprintf(TEXT("Server Response\nUsername: %s\nID: %d"),serverMessage->username, serverMessage->id);
	} else
	{
		//TODO: Wait on event
	}

	pServerResponseMemory->clientOutput = (position + 1) % BUFFER_SIZE;
	ReleaseMutex(hServerResponseMutex);
	ReleaseSemaphore(hServerResponseSemaphoreEmpty, 1, NULL);

	return serverMessage->type;
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