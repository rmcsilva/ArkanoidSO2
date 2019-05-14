#include "stdafx.h"
#include "gameLogic.h"
#include "DLL.h"
#include "setup.h"
#include "ui.h"
#include "util.h"
#include "namedPipeLogic.h"

#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

//Named Pipes
DWORD WINAPI ClientRequestsNamedPipe(LPVOID lpParam);

//Shared Memory
DWORD WINAPI ClientRequestsSharedMemory(LPVOID lpParam);
void sendResponseSharedMemory(ServerMessage serverMessage);

//Server logic
ServerMessage userLogin(ClientMessage* clientMessage);
ServerMessage sendTop10(ClientMessage* clientMessage);
void userLogout(int userID);

void serverShutdownClients();
void closeHandles();

HANDLE hClientRequestSharedMemoryThread;
DWORD dwClientRequestSharedMemoryThreadId;

HANDLE hClientRequestsNamedPipeThread;
DWORD dwClientRequestsNamedPipeThreadId;

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

//Registry
HANDLE hResgistryTop10Key;
TCHAR top10Value[TOP10_SIZE];
DWORD top10PlayerCount;
TopPlayer topPlayers[MAX_TOP_PLAYERS];

//Server logic
int currentUsers = 0;
int id = 0;
int maxPlayers;
Player* users;
HANDLE hServerLogicMutex;

//Thread Shutdown
BOOL keepAlive = TRUE;

//Named pipe data
PipeData* namedPipesData;
HANDLE* hPipeEvents;

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
		_tprintf(TEXT("%ls <ConfigsFilename>\n"), argv[0]);
		_gettchar();
		return -1;
	}

	hServerLogicMutex = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initially not owned
		NULL);             // unnamed mutex

	//Setup Shared Memory
	createClientsSharedMemory(&hClientRequestMemoryMap, sizeof(clientRequests));
	createServersSharedMemory(&hServerResponseMemoryMap, sizeof(serverResponses));
	createGameSharedMemory(&hGameDataMemoryMap, sizeof(GameData));

	if(GetLastError() == ERROR_ALREADY_EXISTS)
	{
		_tprintf(TEXT("Server is already running!\n"));
		_gettchar();
		return -1;
	}

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
	users = malloc(sizeof(Player) * maxPlayers);

	hPipeEvents = malloc(sizeof(HANDLE) * maxPlayers);
	namedPipesData = malloc(sizeof(PipeData) * maxPlayers);

	setupNamedPipes(namedPipesData, hPipeEvents, maxPlayers);

	//Thread to handle client requests through shared memory
	hClientRequestSharedMemoryThread = CreateThread(
		NULL,										// default security attributes
		0,											// use default stack size  
		ClientRequestsSharedMemory,					// thread function name
		NULL,										// argument to thread function 
		0,											// use default creation flags 
		&dwClientRequestSharedMemoryThreadId);		// returns the thread identifier

	
	//Thread to handle client requests through named pipe
	hClientRequestsNamedPipeThread = CreateThread(
		NULL,										// default security attributes
		0,											// use default stack size  
		ClientRequestsNamedPipe,					// thread function name
		NULL,										// argument to thread function 
		0,											// use default creation flags 
		&dwClientRequestsNamedPipeThreadId);		// returns the thread identifier


	if(hClientRequestSharedMemoryThread == NULL)
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
					//TODO: Update user status
					gameVariables.namedPipesData = namedPipesData;
					gameVariables.gameConfigs = gameConfigs;
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
					//TODO: Update user status
					WaitForSingleObject(hGameThread, INFINITE);
				}
				break;
			case SHOW_TOP10:
				showTopPlayers(topPlayers, top10PlayerCount);
				break;
			case LIST_USERS:
				showConnectedUsers(users, currentUsers);
				break;
			default:
				break;
		}

	} while (option != SHUTDOWN);

	//TODO: Update TOP10

	//Ends the game
	pGameDataMemory->gameStatus = GAME_OVER;
	WaitForSingleObject(hGameThread, INFINITE);

	keepAlive = FALSE;
	//Unlocks the shared memory client request thread
	ReleaseSemaphore(hClientRequestSemaphoreItems, 1, NULL);
	//Unlocks the named pipe client request thread
	SetEvent(hPipeEvents[maxPlayers-1]);

	//Sends message to clients that server is shutting down
	serverShutdownClients();

	WaitForSingleObject(hClientRequestSharedMemoryThread, INFINITE);

	UnmapViewOfFile(pClientRequestMemory);
	UnmapViewOfFile(pServerResponseMemory);
	UnmapViewOfFile(pGameDataMemory);

	CloseHandle(hClientRequestMemoryMap);
	CloseHandle(hServerResponseMemoryMap);
	CloseHandle(hGameDataMemoryMap);

	free(users);
	free(hPipeEvents);
	free(namedPipesData);

	closeHandles();
}

DWORD WINAPI ClientRequestsNamedPipe(LPVOID lpParam)
{
	DWORD dwWait, nBytes, readBytes, dwErr;
	int i, fSuccess;
	ClientMessage clientRequest;

	while (keepAlive == TRUE)
	{
		// Wait for the event object to be signaled, indicating 
		// completion of an overlapped read, write, or 
		// connect operation. 

		dwWait = WaitForMultipleObjects(
			maxPlayers,   // number of event objects 
			hPipeEvents,  // array of event objects 
			FALSE,        // does not wait for all 
			INFINITE);    // waits indefinitely

		// dwWait shows which pipe completed the operation. 
		i = dwWait - WAIT_OBJECT_0;  // determines which pipe 

		if (i < 0 || i >(maxPlayers - 1))
		{
			_tprintf(TEXT("Index out of range.\n"));
			return 0;
		}

		// Get the result if the operation was pending. 
		if(namedPipesData[i].fPendingIO)
		{
			fSuccess = GetOverlappedResult(
				namedPipesData[i].hClientRequestsPipe,	// handle to pipe 
				&namedPipesData[i].overlapped,			// OVERLAPPED structure 
				&nBytes,								// bytes transferred 
				FALSE);									// do not wait 

			//Checks what operation was pending
			switch (namedPipesData[i].dwState)
			{
				// Pending connect operation 
				case CONNECTING_STATE:
					if (!fSuccess)
					{
						_tprintf(TEXT("Pipe Connecting State -> Error %d.\n"), GetLastError());
						return 0;
					}
					namedPipesData[i].dwState = READING_REQUEST_STATE;
					break;

				// Pending read request operation 
				case READING_REQUEST_STATE:
					//TODO: Same verifications as bellow?
					if (!fSuccess || nBytes == 0)
					{
						disconnectAndReconnectNamedPipes(&namedPipesData[i]);
						continue;
					}
					namedPipesData[i].dwState = READING_REQUEST_STATE;
					//Handle Client Request 
					receiveRequestNamedPipe(clientRequest, &namedPipesData[i]);
					continue;

				// Pending write response operation 
				case WRITING_RESPONSE_STATE:
					if (!fSuccess || nBytes != sizeof(ServerMessage))
					{
						disconnectAndReconnectNamedPipes(&namedPipesData[i]);
						continue;
					}
					namedPipesData[i].dwState = READING_REQUEST_STATE;
					continue;

				case WRITING_GAME_STATE:
					if (!fSuccess || nBytes != sizeof(GameData))
					{
						disconnectAndReconnectNamedPipes(&namedPipesData[i]);
						continue;
					}
					namedPipesData[i].dwState = READING_REQUEST_STATE;
					continue;

				default:
				{
					_tprintf(TEXT("Invalid pipe state.\n"));
					return 0;
				}
			}
		}

		// The pipe instance is connected to the client 
		// and is ready to read a request from the client.
		fSuccess = ReadFile(
			namedPipesData[i].hClientRequestsPipe,
			&clientRequest,
			sizeof(clientRequest),
			&readBytes,
			&namedPipesData[i].overlapped);

		// The read operation completed successfully. 
		if (fSuccess && readBytes != 0)
		{
			namedPipesData[i].fPendingIO = FALSE;
			//TODO: Add mutex?
			receiveRequestNamedPipe(clientRequest, &namedPipesData[i]);
			continue;
		}

		// The read operation is still pending. 
		dwErr = GetLastError();
		if (!fSuccess && (dwErr == ERROR_IO_PENDING))
		{
			namedPipesData[i].fPendingIO = TRUE;
			continue;
		}

		// An error occurred; disconnect from the client. 
		disconnectAndReconnectNamedPipes(&namedPipesData[i]);
	}

	return 1;
}


DWORD WINAPI ClientRequestsSharedMemory(LPVOID lpParam)
{
	while (keepAlive == TRUE)
	{
		WaitForSingleObject(hClientRequestSemaphoreItems, INFINITE);

		int position = pClientRequestMemory->serverOutput;
		ClientMessage* clientRequest = &pClientRequestMemory->clientMessageBuffer[position];
		ServerMessage serverResponse;

		switch (clientRequest->type)
		{
			case LOGIN_REQUEST:
				serverResponse = userLogin(clientRequest);
				sendResponseSharedMemory(serverResponse);
				showResponseMessageInformation(serverResponse, clientRequest->type);
				break;
			case TOP10:
				serverResponse = sendTop10(clientRequest);
				sendResponseSharedMemory(serverResponse);
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
		//sendResponseSharedMemory(serverResponse);
	}

	return 1;
}

void sendResponseSharedMemory(ServerMessage serverMessage)
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
	BOOL validUser = TRUE;
	ServerMessage serverResponse;
	_tcscpy_s(serverResponse.username, TAM, clientMessage->username);
	_tcscpy_s(serverResponse.content, TAM, TEXT(""));

	//TODO: Check if game is going, if it is increment total users in the shared memory pointer
	if (currentUsers < maxPlayers)
	{
		for(int i=0; i < currentUsers; i++)
		{
			//Checks if username already exists
			if(_tcscmp(serverResponse.username, users[i].username) == 0)
			{
				validUser = FALSE;
				break;
			}
		}

		if(validUser == TRUE)
		{
			serverResponse.id = id;
			_tcscpy_s(users[currentUsers].username, TAM, serverResponse.username);
			serverResponse.type = REQUEST_ACCEPTED;

			WaitForSingleObject(hServerLogicMutex, INFINITE);

			users[currentUsers].id = id;
			//TODO: Verify when game is going 
			pServerResponseMemory->numUsers++;
			currentUsers++;
			id++;

			ReleaseMutex(hServerLogicMutex);
			return serverResponse;
		}

	}

	//Invalid user
	serverResponse.id = UNDEFINED_ID;
	serverResponse.type = REQUEST_DENIED;

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
			WaitForSingleObject(hServerLogicMutex, INFINITE);

			currentUsers--;
			for(int j = i; j < currentUsers; j++)
			{
				users[j].id = users[j+1].id;
				users[j].score = users[j+1].score;
				users[j].inGame = users[j+1].inGame;
				_tcscpy_s(users[j].username, TAM, users[j+1].username);
			}

			ReleaseMutex(hServerLogicMutex);
			break;
		}
	}
}

void serverShutdownClients()
{
	ServerMessage logout;
	logout.type = LOGOUT;
	logout.id = UNDEFINED_ID;
	TCHAR empty[] = TEXT("");
	_tcscpy_s(logout.username, sizeof(empty), empty);
	_tcscpy_s(logout.content, sizeof(empty), empty);
	sendResponseSharedMemory(logout);
	for(int i = 0; i < maxPlayers; i++)
	{
		if(namedPipesData[i].userID != UNDEFINED_ID)
		{
			sendResponseNamedPipe(logout, &namedPipesData[i]);
		}
	}
}

void closeHandles()
{
	RegCloseKey(hResgistryTop10Key);

	CloseHandle(hClientRequestSharedMemoryThread);

	CloseHandle(hClientRequestSemaphoreItems);
	CloseHandle(hClientRequestSemaphoreEmpty);
	CloseHandle(hServerResponseSemaphoreItems);
	CloseHandle(hServerResponseSemaphoreEmpty);
}