#include "stdafx.h"
#include "gameLogic.h"
#include "DLL.h"
#include "setup.h"
#include "ui.h"

#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>


DWORD WINAPI ClientRequests(LPVOID lpParam);
ServerMessage userLogin(ClientMessage* clientMessage);
ServerMessage sendTop10(ClientMessage* clientMessage);
void userLogout(int userID);
void sendResponse(ServerMessage serverMessage);
void serverShutdownClients();
void closeHandles();

HANDLE hClientRequestThread;
DWORD dwClientRequestThreadId;

HANDLE hGameThread;
DWORD dwGameThreadId;

ClientMessageControl* pClientRequestMemory;
ServerMessageControl* pServerResponseMemory;
GameData* pGameDataMemory;

HANDLE hClientRequestSemaphoreItems;
HANDLE hClientRequestSemaphoreEmpty;

HANDLE hServerResponseSemaphoreItems;
HANDLE hServerResponseSemaphoreEmpty;

HANDLE hGameUpdateEvent;

HANDLE hResgistryTop10Key;
TCHAR top10Value[TOP10_SIZE];
DWORD top10PlayerCount;
TopPlayer topPlayers[MAX_TOP_PLAYERS];

int currentUsers = 0;
int id = 0;
int maxPlayers;
Player* users;

int keepAlive = 1;

int _tmain(int argc, TCHAR* argv[])
{
	ClientMessageControl clientRequests;
	ServerMessageControl serverResponses;
	GameData gameData;
	GameConfigs gameConfigs;

	HANDLE hClientRequestMemoryMap;
	HANDLE hServerResponseMemoryMap;
	HANDLE hGameDataMemoryMap;

	GameVariables gameVariables;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

	if(argc != 2)
	{
		_tprintf(TEXT("Invalid number of arguments!\n"));
		_tprintf(TEXT("%hs <ConfigsFilename>\n"), argv[0]);
		return -1;
	}

	//Setup Shared Memory
	createClientsSharedMemory(&hClientRequestMemoryMap, sizeof(clientRequests));
	createServersSharedMemory(&hServerResponseMemoryMap, sizeof(serverResponses));
	createGameSharedMemory(&hGameDataMemoryMap, sizeof(GameData));

	if (hClientRequestMemoryMap == NULL || hServerResponseMemoryMap == NULL || hGameDataMemoryMap == NULL)
	{
		_tprintf(TEXT("Error creating shared memory resources.\n"));
		return -1;
	}

	//Map Shared Memory
	pClientRequestMemory = mapClientsSharedMemory(&hClientRequestMemoryMap, sizeof(clientRequests));
	pServerResponseMemory = mapServersSharedMemory(&hServerResponseMemoryMap, sizeof(serverResponses));
	pGameDataMemory = mapReadWriteGameSharedMemory(&hGameDataMemoryMap, sizeof(gameData));

	if (pClientRequestMemory == NULL || pServerResponseMemory == NULL || pServerResponseMemory == NULL)
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

	//Setup Registry and Top Players
	setupRegistryTopPlayers(&hResgistryTop10Key, top10Value, &top10PlayerCount);
	convertStringToTopPlayers(&topPlayers, top10Value, &top10PlayerCount);

	//Events
	createGameUpdateEvent(&hGameUpdateEvent);

	if(hGameUpdateEvent == NULL)
	{
		_tprintf(TEXT("Error creating event.\n"));
		return -1;
	}

	if(setupInitialGameConfigs(argv[1], &gameConfigs) == -1)
	{
		_gettchar();
		return -1;
	}

	maxPlayers = gameConfigs.maxPlayers;
	users = malloc(sizeof(Player) * gameConfigs.maxPlayers);

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

	pGameDataMemory->level = 5;

	do
	{
		//TODO: Send the game status to change menu accordingly
		option = initialMenu();

		switch (option)
		{
			case START_GAME:
				if(pGameDataMemory->gameStatus != GAME_ACTIVE)
				{
					pGameDataMemory->gameStatus = GAME_ACTIVE;

					//TODO: Setup Game Variables
					gameVariables.hGameUpdateEvent = hGameUpdateEvent;
					gameVariables.pGameData = pGameDataMemory;

					//Thread to handle client requests
					hGameThread = CreateThread(
						NULL,						// default security attributes
						0,							// use default stack size  
						GameLogic,					// thread function name
						(LPVOID)&gameVariables,		// argument to thread function 
						0,							// use default creation flags 
						&dwGameThreadId);			// returns the thread identifier 
				} else
				{
					pGameDataMemory->gameStatus = GAME_OVER;
					WaitForSingleObject(hGameThread, INFINITE);
				}
				break;
			case SHOW_TOP10:
				showTopPlayers(topPlayers, top10PlayerCount);
				break;
			case LIST_USERS:
				break;
			default:
				break;
		}

	} while (option != SHUTDOWN);

	//TODO: Update TOP10

	//Ends the game
	pGameDataMemory->gameStatus = GAME_OVER;
	WaitForSingleObject(hGameThread, INFINITE);

	keepAlive = 0;
	//Unlocks the client request thread
	ReleaseSemaphore(hClientRequestSemaphoreItems, 1, NULL);

	//Sends message to clients that server is shutting down
	serverShutdownClients();

	WaitForSingleObject(hClientRequestThread, INFINITE);

	UnmapViewOfFile(pClientRequestMemory);
	UnmapViewOfFile(pServerResponseMemory);
	UnmapViewOfFile(pGameDataMemory);

	CloseHandle(hClientRequestMemoryMap);
	CloseHandle(hServerResponseMemoryMap);
	CloseHandle(hGameDataMemoryMap);

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
			case TOP10:
				serverResponse = sendTop10(clientRequest);
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

	return 1;
}

void sendResponse(ServerMessage serverMessage)
{
	WaitForSingleObject(hServerResponseSemaphoreEmpty, INFINITE);
	int position = pServerResponseMemory->serverInput;
	ServerMessage* serverResponse = &pServerResponseMemory->serverMessageBuffer[position];

	_tcscpy_s(serverResponse->username, TAM, serverMessage.username);
	_tcscpy_s(serverResponse->content, TOP10_SIZE, serverMessage.content);
	serverResponse->id = serverMessage.id;
	serverResponse->type = serverMessage.type;

	pServerResponseMemory->serverInput = (position + 1) % BUFFER_SIZE;
	ReleaseSemaphore(hServerResponseSemaphoreItems, 1, NULL);
}

ServerMessage userLogin(ClientMessage* clientMessage)
{
	ServerMessage serverResponse;
	_tcscpy_s(serverResponse.username, TAM, clientMessage->username);
	_tcscpy_s(serverResponse.content, TAM, TEXT(""));

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

ServerMessage sendTop10(ClientMessage* clientMessage)
{
	ServerMessage serverResponse;

	_tcscpy_s(serverResponse.username, TAM, clientMessage->username);
	_tcscpy_s(serverResponse.content, TOP10_SIZE, top10Value);
	serverResponse.id = clientMessage->id;
	serverResponse.type = REQUEST_ACCEPTED;

	return serverResponse;
}

void userLogout(int userID)
{
	for(int i = 0; i < currentUsers; i++)
	{
		if(users[i].id == userID)
		{
			_tprintf(TEXT("\n\nUser %s logged out\n\n"), users[i].username);
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

void serverShutdownClients()
{
	ServerMessage logout;
	logout.type = LOGOUT;
	logout.id = -1;
	TCHAR empty[] = TEXT("");
	_tcscpy_s(logout.username, sizeof(empty), empty);
	_tcscpy_s(logout.content, sizeof(empty), empty);
	sendResponse(logout);
}

void closeHandles()
{
	RegCloseKey(hResgistryTop10Key);

	CloseHandle(hClientRequestThread);

	CloseHandle(hClientRequestSemaphoreItems);
	CloseHandle(hClientRequestSemaphoreEmpty);
	CloseHandle(hServerResponseSemaphoreItems);
	CloseHandle(hServerResponseSemaphoreEmpty);
}