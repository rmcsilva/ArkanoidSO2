#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>

#include "DLL.h"
#include "setup.h"

ClientMessageControl clientRequests;
ServerMessageControl serverResponses;

int main(int argc, char* argv[])
{
	int currentUsers = 0;
	Player* users;

	HANDLE hClientRequestMemoryMap;
	HANDLE hServerResponseMemoryMap;

	ClientMessageControl* pClientRequestMemory;
	ServerMessageControl* pServerResponseMemory;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

	//Setup Shared Memory
	createClientsSharedMemory(&hClientRequestMemoryMap, sizeof(clientRequests));
	createServersSharedMemory(&hServerResponseMemoryMap, sizeof(serverResponses));

	if (hClientRequestMemoryMap == NULL || hServerResponseMemoryMap == NULL)
	{
		_tprintf(TEXT("Error opening shared memory resources.\n"));
		return -1;
	}

	//Map Shared Memory
	pClientRequestMemory = mapClientsSharedMemory(&hClientRequestMemoryMap, sizeof(clientRequests));
	pServerResponseMemory = mapServersSharedMemory(&hServerResponseMemoryMap, sizeof(serverResponses));

	pClientRequestMemory->clientInput = 23;
	pServerResponseMemory->clientOutput = 45;

	_gettchar();

	UnmapViewOfFile(pClientRequestMemory);
	UnmapViewOfFile(pServerResponseMemory);

	CloseHandle(hClientRequestMemoryMap);
	CloseHandle(hServerResponseMemoryMap);
}


