#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>

#include "DLL.h"
#include "setup.h"

DWORD WINAPI ClientRequests(LPVOID lpParam);

HANDLE hClientRequestThread;
DWORD dwClientRequestThreadId;

ClientMessageControl* pClientRequestMemory;
ServerMessageControl* pServerResponseMemory;

HANDLE hClientRequestSemaphoreItems;
HANDLE hClientRequestSemaphoreEmpty;

HANDLE hServerResponseSemaphoreItems;
HANDLE hServerResponseSemaphoreEmpty;

int main(int argc, char* argv[])
{
	int currentUsers = 0;
	Player* users;

	ClientMessageControl clientRequests;
	ServerMessageControl serverResponses;

	HANDLE hClientRequestMemoryMap;
	HANDLE hServerResponseMemoryMap;

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

	//TODO: Add verification == NULL

	//TODO: Initialize struct?
	pClientRequestMemory->clientInput = 0;
	pClientRequestMemory->serverOutput = 0;
	pServerResponseMemory->clientOutput = 0;

	//Create Semaphores
	createClientsRequestSemaphores(&hClientRequestSemaphoreItems, &hClientRequestSemaphoreEmpty);
	createServersResponseSemaphores(&hServerResponseSemaphoreItems, &hServerResponseSemaphoreEmpty);

	//TODO: Add verification == NULL

	hClientRequestThread = CreateThread(
		NULL,						// default security attributes
		0,							// use default stack size  
		ClientRequests,				// thread function name
		NULL,						// argument to thread function 
		0,							// use default creation flags 
		&dwClientRequestThreadId);  // returns the thread identifier 

	//TODO: Add verification == NULL

	WaitForSingleObject(hClientRequestThread, INFINITE);

	CloseHandle(hClientRequestThread);

	UnmapViewOfFile(pClientRequestMemory);
	UnmapViewOfFile(pServerResponseMemory);

	CloseHandle(hClientRequestMemoryMap);
	CloseHandle(hServerResponseMemoryMap);
}

DWORD WINAPI ClientRequests(LPVOID lpParam)
{
	while (TRUE)
	{
		WaitForSingleObject(hClientRequestSemaphoreItems, INFINITE);
		//TODO: Make a login function
		int position = pClientRequestMemory->serverOutput;
		_tprintf(TEXT("Request Type: %d\nUsername: %s\n"), pClientRequestMemory->clientMessageBuffer[position].type, pClientRequestMemory->clientMessageBuffer[position].username);
		pClientRequestMemory->serverOutput = (position + 1) % BUFFER_SIZE;
		ReleaseSemaphore(hClientRequestSemaphoreEmpty, 1, NULL);
	}
}