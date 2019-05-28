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
DWORD WINAPI GameUpdateNamedPipe(LPVOID lpParam);

//Shared Memory
DWORD WINAPI ClientRequestsSharedMemory(LPVOID lpParam);
void sendResponseSharedMemory(ServerMessage serverMessage);

//Server logic
ServerMessage userLogin(ClientMessage* clientMessage);
ServerMessage sendTop10(ClientMessage* clientMessage);
void movePlayerBarrier(int playerId, int direction);
int checkIfUserIsInGame(int playerId);
void userLogout(int userID);

void serverShutdownClients();
void closeHandles();

//Shared Memory
HANDLE hClientRequestSharedMemoryThread;
DWORD dwClientRequestSharedMemoryThreadId;

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
HANDLE* hPipeRequestsEvents;
HANDLE* hPipeGameUpdateEvents;

HANDLE hClientRequestsNamedPipeThread;
DWORD dwClientRequestsNamedPipeThreadId;

HANDLE hGameUpdateNamedPipeThread;
DWORD hGameUpdateNamedPipeThreadId;

//Game Logic
GameConfigs gameConfigs;
HANDLE hGameLogicMutex;

int _tmain(int argc, TCHAR* argv[])
{
	ClientMessageControl clientRequests;
	ServerMessageControl serverResponses;
	GameData gameData;

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

	hGameLogicMutex = CreateMutex(
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

	hPipeRequestsEvents = malloc(sizeof(HANDLE) * maxPlayers);
	hPipeGameUpdateEvents = malloc(sizeof(HANDLE) * maxPlayers);
	namedPipesData = malloc(sizeof(PipeData) * maxPlayers);

	setupNamedPipes(namedPipesData, hPipeRequestsEvents, hPipeGameUpdateEvents, maxPlayers);

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

	//Thread to handle game updates through named pipe
	hGameUpdateNamedPipeThread = CreateThread(
		NULL,										// default security attributes
		0,											// use default stack size  
		GameUpdateNamedPipe,						// thread function name
		NULL,										// argument to thread function 
		0,											// use default creation flags 
		&hGameUpdateNamedPipeThreadId);				// returns the thread identifier


	if(hClientRequestSharedMemoryThread == NULL)
	{
		_tprintf(TEXT("Error creating client request thread.\n"));
		return -1;
	}

	_tprintf(TEXT("%hs"), WELCOME_MESSAGE);

	int option;

	do
	{
		//TODO: Send the game status to change menu accordingly
		option = initialMenu(pGameDataMemory->gameStatus);

		switch (option)
		{
			case START_GAME:
				if(pGameDataMemory->gameStatus != GAME_ACTIVE)
				{
					if(currentUsers > 0)
					{
						assignUsersToGame(pGameDataMemory, users, currentUsers);

						//Setup Game Variables
						gameVariables.namedPipesData = namedPipesData;
						gameVariables.gameConfigs = gameConfigs;
						gameVariables.hGameUpdateEvent = hGameUpdateEvent;
						gameVariables.hGameLogicMutex = hGameLogicMutex;
						gameVariables.pGameData = pGameDataMemory;

						//Thread to handle the game logic
						hGameThread = CreateThread(
							NULL,						// default security attributes
							0,							// use default stack size  
							GameLogic,					// thread function name
							(LPVOID)&gameVariables,		// argument to thread function 
							0,							// use default creation flags 
							&dwGameThreadId);			// returns the thread identifier 
					} else
					{
						_tprintf(TEXT("\nCannot start a game without players!\n\n"));
					}
					
				} else
				{
					pGameDataMemory->gameStatus = GAME_OVER;
					//TODO: Update user status
					WaitForSingleObject(hGameThread, INFINITE);
					sendGameUpdate(gameVariables);
				}
				break;
			case SHOW_TOP10:
				showTopPlayers(topPlayers, top10PlayerCount);
				break;
			case LIST_USERS:
				showConnectedUsers(users, currentUsers);
				break;
			case SHUTDOWN:
				if(pGameDataMemory->gameStatus == GAME_ACTIVE)
				{
					_tprintf(TEXT("Cannot shutdown server! Game is still going!\n"));
					option = -1;
					break;
				}
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
	SetEvent(hPipeRequestsEvents[maxPlayers-1]);

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
	free(hPipeRequestsEvents);
	free(hPipeGameUpdateEvents);
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
			maxPlayers,				// number of event objects 
			hPipeRequestsEvents,	// array of event objects 
			FALSE,					// does not wait for all 
			INFINITE);				// waits indefinitely

		// dwWait shows which pipe completed the operation. 
		i = dwWait - WAIT_OBJECT_0;  // determines which pipe 

		if (i < 0 || i > (maxPlayers - 1))
		{
			_tprintf(TEXT("Index out of range (client requests).\n"));
			return 0;
		}

		// Get the result if the operation was pending. 
		if(namedPipesData[i].fPendingIORequests)
		{
			fSuccess = GetOverlappedResult(
				namedPipesData[i].hClientRequestsPipe,	// handle to pipe 
				&namedPipesData[i].overlappedRequests,	// OVERLAPPED structure 
				&nBytes,								// bytes transferred 
				FALSE);									// do not wait 

			//Checks what operation was pending
			switch (namedPipesData[i].dwStateRequests)
			{
				// Pending connect operation 
				case CONNECTING_STATE:
					if (!fSuccess)
					{
						_tprintf(TEXT("Pipe Connecting State -> Error %d.\n"), GetLastError());
						return 0;
					}
					namedPipesData[i].dwStateRequests = READING_REQUEST_STATE;
					break;

				// Pending read request operation 
				case READING_REQUEST_STATE:
					//TODO: Same verifications as bellow?
					if (!fSuccess || nBytes == 0)
					{
						disconnectAndReconnectNamedPipes(&namedPipesData[i]);
						continue;
					}
					namedPipesData[i].dwStateRequests = READING_REQUEST_STATE;
					//Handle Client Request 
					receiveRequestNamedPipe(clientRequest, &namedPipesData[i]);
					break;

				// Pending write response operation 
				case WRITING_RESPONSE_STATE:
					if (!fSuccess || nBytes != sizeof(ServerMessage))
					{
						disconnectAndReconnectNamedPipes(&namedPipesData[i]);
						continue;
					}
					namedPipesData[i].dwStateRequests = READING_REQUEST_STATE;
					break;

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
			&namedPipesData[i].overlappedRequests);

		// The read operation completed successfully. 
		if (fSuccess && readBytes != 0)
		{
			namedPipesData[i].fPendingIORequests = FALSE;
			receiveRequestNamedPipe(clientRequest, &namedPipesData[i]);
			continue;
		}

		// The read operation is still pending. 
		dwErr = GetLastError();
		if (!fSuccess && (dwErr == ERROR_IO_PENDING))
		{
			namedPipesData[i].fPendingIORequests = TRUE;
			continue;
		}

		// An error occurred; disconnect from the client. 
		disconnectAndReconnectNamedPipes(&namedPipesData[i]);
	}

	return 1;
}

DWORD WINAPI GameUpdateNamedPipe(LPVOID lpParam)
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
			maxPlayers,				// number of event objects 
			hPipeGameUpdateEvents,	// array of event objects 
			FALSE,					// does not wait for all 
			INFINITE);				// waits indefinitely

		// dwWait shows which pipe completed the operation. 
		i = dwWait - WAIT_OBJECT_0;  // determines which pipe 

		if (i < 0 || i > (maxPlayers - 1))
		{
			_tprintf(TEXT("Index out of range (game update).\n"));
			return 0;
		}

		// Get the result if the operation was pending. 
		if (namedPipesData[i].fPendingIOGame)
		{
			fSuccess = GetOverlappedResult(
				namedPipesData[i].hGamePipe,		// handle to pipe 
				&namedPipesData[i].overlappedGame,	// OVERLAPPED structure 
				&nBytes,							// bytes transferred 
				FALSE);								// do not wait 

			//Checks what operation was pending
			switch (namedPipesData[i].dwStateGame)
			{
				// Pending connect operation 
				case CONNECTING_STATE:
					if (!fSuccess)
					{
						_tprintf(TEXT("Pipe Connecting State -> Error %d.\n"), GetLastError());
						return 0;
					}
					namedPipesData[i].dwStateGame = WRITING_GAME_STATE;
					break;

				// Pending write game update operation 
				case WRITING_GAME_STATE:
					if (!fSuccess || nBytes != sizeof(GameData))
					{
						disconnectAndReconnectNamedPipes(&namedPipesData[i]);
						continue;
					}
					break;
				default:
				{
					_tprintf(TEXT("Invalid pipe state.\n"));
					return 0;
				}
			}
		}

		//Resets event until there is another game update
		ResetEvent(namedPipesData[i].overlappedGame.hEvent);
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
			case MOVE_RIGHT:
				movePlayerBarrier(clientRequest->id, MOVE_RIGHT);
				break;
			case MOVE_LEFT:
				movePlayerBarrier(clientRequest->id, MOVE_LEFT);
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

void movePlayerBarrier(int playerId, int direction)
{
	int index = checkIfUserIsInGame(playerId);

	if(index == -1)
	{
		return;
	}

	//TODO: Adapt to barrier changes in size!

	int moveTo, nextPlayer, barrierDimensions;
	if(direction == MOVE_RIGHT)
	{
		moveTo = pGameDataMemory->barrier[index].position.x + gameConfigs.movementSpeed;
		nextPlayer = getPlayerToTheRight(*pGameDataMemory, index);
		barrierDimensions = pGameDataMemory->barrierDimensions * pGameDataMemory->barrier[index].sizeRatio;

		if(nextPlayer != -1)
		{
			if(pGameDataMemory->barrier[nextPlayer].position.x < moveTo + barrierDimensions)
			{
				_tprintf(TEXT("\nUser %s can't move right(player overlap)\n\n"), pGameDataMemory->player[index].username);
				return;
			}
		}

		if (moveTo + barrierDimensions <= GAME_BORDER_RIGHT)
		{
			WaitForSingleObject(hGameLogicMutex, INFINITE);
			pGameDataMemory->barrier[index].position.x = moveTo;
			ReleaseMutex(hGameLogicMutex);
			_tprintf(TEXT("\nUser %s moved right\n\n"), pGameDataMemory->player[index].username);
			return;
		}
		_tprintf(TEXT("\nUser %s can't move right\n\n"), pGameDataMemory->player[index].username);
		
	} else if(direction == MOVE_LEFT)
	{
		moveTo = pGameDataMemory->barrier[index].position.x - gameConfigs.movementSpeed;
		nextPlayer = getPlayerToTheLeft(*pGameDataMemory, index);
		barrierDimensions = pGameDataMemory->barrierDimensions * pGameDataMemory->barrier[nextPlayer].sizeRatio;

		if(nextPlayer != -1)
		{
			if (pGameDataMemory->barrier[nextPlayer].position.x + barrierDimensions > moveTo)
			{
				_tprintf(TEXT("\nUser %s can't move left (player overlap)\n\n"), pGameDataMemory->player[index].username);
				return;
			}
		}

		if (moveTo >= GAME_BORDER_LEFT)
		{
			WaitForSingleObject(hGameLogicMutex, INFINITE);
			pGameDataMemory->barrier[index].position.x = moveTo;
			ReleaseMutex(hGameLogicMutex);
			_tprintf(TEXT("\nUser %s moved left\n\n"), pGameDataMemory->player[index].username);
			return;
		}
		_tprintf(TEXT("\nUser %s can't move left\n\n"), pGameDataMemory->player[index].username);
	}

}

int checkIfUserIsInGame(int playerId)
{
	for(int i = 0; i < currentUsers; i++)
	{
		if(pGameDataMemory->player[i].id == playerId)
		{
			if(pGameDataMemory->player[i].inGame == FALSE)
			{
				_tprintf(TEXT("\nUser %s is not in game!\n\n"), pGameDataMemory->player[i].username);
				return -1;
			}
			return i;
		}
	}
	_tprintf(TEXT("\nUser %d is not connected to the server!\n\n"), playerId);
	return -1;
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
	pGameDataMemory->gameStatus = LOGOUT;

	SetEvent(hGameUpdateEvent);
	ResetEvent(hGameUpdateEvent);

	for (int i = 0; i < maxPlayers; i++)
	{
		if (namedPipesData[i].userID != UNDEFINED_ID)
		{
			sendGameNamedPipe(*pGameDataMemory, &namedPipesData[i]);
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

	CloseHandle(hServerLogicMutex);
	CloseHandle(hGameLogicMutex);
}