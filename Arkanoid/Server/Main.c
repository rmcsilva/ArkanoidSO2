#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>

#include "DLL.h"
#include "setup.h"
#include "ui.h"

DWORD WINAPI ClientRequests(LPVOID lpParam);
ServerMessage userLogin(ClientMessage* clientMessage);
void userLogout(int userID);
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

int keepAlive = 1;

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

	//Thread to handle client requests
	hClientRequestThread = CreateThread(
		NULL,						// default security attributes
		0,							// use default stack size  
		ClientRequests,				// thread function name
		NULL,						// argument to thread function 
		0,							// use default creation flags 
		&dwClientRequestThreadId);  // returns the thread identifier 

	if(hClientRequestThread == NULL)
	{
		_tprintf(TEXT("Error creating client request thread.\n"));
		return -1;
	}

	_tprintf(TEXT("%hs"), WELCOME_MESSAGE);

	int option;

	do
	{
		option = initialMenu();

		switch (option)
		{
			case START_GAME:
				//TODO: Create Game Data Shared Memory
				break;
			case SHOW_TOP10:
				break;
			case LIST_USERS:
				break;
			default:
				break;
		}

	} while (option != SHUTDOWN);

	keepAlive = 0;
	//Unlocks the client request thread
	ReleaseSemaphore(hClientRequestSemaphoreItems, 1, NULL);

	WaitForSingleObject(hClientRequestThread, INFINITE);

	UnmapViewOfFile(pClientRequestMemory);
	UnmapViewOfFile(pServerResponseMemory);

	CloseHandle(hClientRequestMemoryMap);
	CloseHandle(hServerResponseMemoryMap);

	closeHandles();
}

DWORD WINAPI ClientRequests(LPVOID lpParam)
{
	while (keepAlive)
	{
		WaitForSingleObject(hClientRequestSemaphoreItems, INFINITE);

		int position = pClientRequestMemory->serverOutput;
		ClientMessage* clientRequest = &pClientRequestMemory->clientMessageBuffer[position];
		ServerMessage serverResponse;

		switch (clientRequest->type)
		{
			case LOGIN_REQUEST:
				serverResponse = userLogin(clientRequest);
				sendResponse(serverResponse);
				showResponseMessageInformation(serverResponse, clientRequest->type);
				break;
			case LOGOUT:
				userLogout(clientRequest->id);
				break;
				//TODO: implement
			default:
				continue;
		}

		pClientRequestMemory->serverOutput = (position + 1) % BUFFER_SIZE;
		ReleaseSemaphore(hClientRequestSemaphoreEmpty, 1, NULL);

		//TODO: Put in a thread?
		//sendResponse(serverResponse);
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

void userLogout(int userID)
{
	for(int i = 0; i < currentUsers; i++)
	{
		if(users[i].id == userID)
		{
			_tprintf(TEXT("User %s logged out\n"), users[i].username);
			currentUsers--;
			for(int j = i; j < currentUsers; j++)
			{
				users[j].id = users[j+1].id;
				users[j].score = users[j+1].score;
				users[j].inGame = users[j + 1].inGame;
				_tcscpy_s(users[j].username, TAM, users[j+1].username);
			}

			break;
		}
	}
}

void closeHandles()
{
	CloseHandle(hClientRequestThread);

	CloseHandle(hClientRequestSemaphoreItems);
	CloseHandle(hClientRequestSemaphoreEmpty);
	CloseHandle(hServerResponseSemaphoreItems);
	CloseHandle(hServerResponseSemaphoreEmpty);
}