// DLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include "DLL.h"
#include "setup.h"

int isLocalUser = 1;

HANDLE hClientRequestMemoryMap;
HANDLE hServerResponseMemoryMap;

ClientMessageControl* pClientRequestMemory;
ServerMessageControl* pServerResponseMemory;

int test(void)
{
	_tprintf(TEXT("Hello\n"));
	return 123;
}

int login(TCHAR* username)
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

	return 1;
}

void logout()
{
	UnmapViewOfFile(pClientRequestMemory);
	UnmapViewOfFile(pServerResponseMemory);

	CloseHandle(hClientRequestMemoryMap);
	CloseHandle(hServerResponseMemoryMap);
}
