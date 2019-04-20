#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>

#include "DLL.h"
#include "setup.h"

DWORD WINAPI ClientRequests(LPVOID lpParam);
ServerMessage userLogin(ClientMessage* clientMessage);
void sendResponse(ServerMessage serverMessage);
void closeHandles();

HANDLE hClientRequestThread;
DWORD dwClientRequestThreadId;

ClientMessageControl* pClientRequestMemory;
ServerMessageControl* pServerResponseMemory;

HANDLE hClientRequestSemaphoreItems;
HANDLE hClientRequestSemaphoreEmpty;

HANDLE hServerResponseSemaphoreItems;
HANDLE hServerResponseSemaphoreEmpty;

int currentUsers = 0;
int id = 0;
int maxPlayers;
Player* users;

int gameStatus = IN_LOBBY;

int main(int argc, char* argv[])
{
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
		_tprintf(TEXT("Error creating shared memory resources.\n"));
		return -1;
	}

	//Map Shared Memory
	pClientRequestMemory = mapClientsSharedMemory(&hClientRequestMemoryMap, sizeof(clientRequests));
	pServerResponseMemory = mapServersSharedMemory(&hServerResponseMemoryMap, sizeof(serverResponses));

	if (pClientRequestMemory == NULL || pServerResponseMemory == NULL)
	{
		_tprintf(TEXT("Error maping shared memory.\n"));
		return -1;
	}

	//Create Semaphores
	createClientsRequestSemaphores(&hClientRequestSemaphoreItems, &hClientRequestSemaphoreEmpty);
	createServersResponseSemaphores(&hServerResponseSemaphoreItems, &hServerResponseSemaphoreEmpty);

	if (hClientRequestSemaphoreItems == NULL || hClientRequestSemaphoreEmpty == NULL || hServerResponseSemaphoreItems == NULL || hServerResponseSemaphoreEmpty == NULL)
	{
		_tprintf(TEXT("Error creating semaphores.\n"));
		return -1;
	}

	//TODO: Read from text file to setup
	maxPlayers = MAX_PLAYERS;
	users = malloc(sizeof(Player) * maxPlayers);

	hClientRequestThread = CreateThread(
		NULL,						// default security attributes
		0,							// use default stack size  
		ClientRequests,				// thread function name
		NULL,						// argument to thread function 
		0,							// use default creation flags 
		&dwClientRequestThreadId);  // returns the thread identifier 

	//TODO: Add verification == NULL

	//TODO: Create Game Data Shared Memory

	WaitForSingleObject(hClientRequestThread, INFINITE);

	CloseHandle(hClientRequestThread);

	UnmapViewOfFile(pClientRequestMemory);
	UnmapViewOfFile(pServerResponseMemory);

	CloseHandle(hClientRequestMemoryMap);
	CloseHandle(hServerResponseMemoryMap);

	closeHandles();
}

DWORD WINAPI ClientRequests(LPVOID lpParam)
{
	//TODO: Change loop condition
	while (TRUE)
	{
		WaitForSingleObject(hClientRequestSemaphoreItems, INFINITE);

		int position = pClientRequestMemory->serverOutput;
		ClientMessage* clientRequest = &pClientRequestMemory->clientMessageBuffer[position];
		ServerMessage serverResponse;

		switch (clientRequest->type)
		{
			case LOGIN_REQUEST:
				serverResponse = userLogin(clientRequest);
				break;
			case LOGOUT:
				//TODO: implement
			default:
				continue;
		}

		pClientRequestMemory->serverOutput = (position + 1) % BUFFER_SIZE;
		ReleaseSemaphore(hClientRequestSemaphoreEmpty, 1, NULL);

		//TODO: Put in a thread?
		sendResponse(serverResponse);
		_tprintf(TEXT("User ID: %d\nUsername: %s\n"), users[currentUsers-1].id, users[currentUsers-1].username);
	}
}

void sendResponse(ServerMessage serverMessage)
{
	WaitForSingleObject(hServerResponseSemaphoreEmpty, INFINITE);
	int position = pServerResponseMemory->serverInput;
	ServerMessage* serverResponse = &pServerResponseMemory->serverMessageBuffer[position];

	_tcscpy_s(serverResponse->username, TAM, serverMessage.username);
	serverResponse->id = serverMessage.id;
	serverResponse->type = serverMessage.type;

	pServerResponseMemory->serverInput = (position + 1) % BUFFER_SIZE;
	ReleaseSemaphore(hServerResponseSemaphoreItems, 1, NULL);
}

ServerMessage userLogin(ClientMessage* clientMessage)
{
	ServerMessage serverResponse;
	_tcscpy_s(serverResponse.username, TAM, clientMessage->username);

	//TODO: Check if game is going, if it is increment total users in the shared memory pointer
	if (currentUsers < maxPlayers)
	{
		users[currentUsers].id = id;
		serverResponse.id = id;
		_tcscpy_s(users[currentUsers].username, TAM, serverResponse.username);		
		serverResponse.type = REQUEST_ACCEPTED;
		pServerResponseMemory->numUsers++;
		currentUsers++;
		id++;
	} else
	{
		serverResponse.id = -1;
		serverResponse.type = REQUEST_DENIED;
	}

	return serverResponse;
}

void closeHandles()
{
	CloseHandle(hClientRequestThread);

	CloseHandle(hClientRequestSemaphoreItems);
	CloseHandle(hClientRequestSemaphoreEmpty);
	CloseHandle(hServerResponseSemaphoreItems);
	CloseHandle(hServerResponseSemaphoreEmpty);
}