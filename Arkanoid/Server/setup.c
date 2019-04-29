#include "stdafx.h"
#include "setup.h"
#include "resourceConstants.h"

void createClientsSharedMemory(HANDLE* hClientRequestMemoryMap, DWORD clientRequestsSize)
{
	*hClientRequestMemoryMap = CreateFileMapping(
		INVALID_HANDLE_VALUE,				 // use paging file
		NULL,								 // default security
		PAGE_READWRITE,						 // read/write access
		0,									 // maximum object size (high-order DWORD)
		clientRequestsSize,				     // maximum object size (low-order DWORD)
		BUFFER_MEMORY_CLIENT_REQUESTS);      // name of mapping object
}

void createServersSharedMemory(HANDLE* hServerResponseMemoryMap, DWORD serverResponsesSize)
{
	*hServerResponseMemoryMap = CreateFileMapping(
		INVALID_HANDLE_VALUE,				 // use paging file
		NULL,								 // default security
		PAGE_READWRITE,						 // read/write access
		0,									 // maximum object size (high-order DWORD)
		serverResponsesSize,				 // maximum object size (low-order DWORD)
		BUFFER_MEMORY_SERVER_RESPONSES);     // name of mapping object
}

void createGameSharedMemory(HANDLE* hGameDataMemoryMap, DWORD gameDataSize)
{
	*hGameDataMemoryMap = CreateFileMapping(
		INVALID_HANDLE_VALUE,				 // use paging file
		NULL,								 // default security
		PAGE_READWRITE,						 // read/write access
		0,									 // maximum object size (high-order DWORD)
		gameDataSize,						 // maximum object size (low-order DWORD)
		BUFFER_MEMORY_GAME);				 // name of mapping object
}

GameData* mapReadWriteGameSharedMemory(HANDLE* hGameDataMemoryMap, DWORD gameDataSize)
{
	GameData* pGameDataMemory = (GameData*)MapViewOfFile(
		*hGameDataMemoryMap,				// handle to map object
		FILE_MAP_ALL_ACCESS,				// read/write permission
		0,
		0,
		gameDataSize);

	return pGameDataMemory;
}

int setupInitialGameConfigs(TCHAR* filename[_MAX_FNAME], GameConfigs* gameConfigs)
{
	LARGE_INTEGER tam;
	HANDLE hFile, hFileMemoryMap;

	hFile = CreateFile(filename,     // name of the file to read
		GENERIC_READ,						// open for reading
		0,									// do not share
		NULL,								// default security
		OPEN_EXISTING,						// opens existing file
		FILE_READ_ONLY,						// read only file
		NULL);								// no attr. template

	if (GetLastError() == ERROR_FILE_NOT_FOUND)
	{
		_tprintf(TEXT("File not found!\n"));
		return -1;
	}

	GetFileSizeEx(hFile, &tam);

	hFileMemoryMap = CreateFileMapping(
		hFile,								 // use paging file
		NULL,								 // default security
		PAGE_READONLY,						 // read only access
		tam.HighPart,									 // maximum object size (high-order DWORD)
		tam.LowPart,				     // maximum object size (low-order DWORD)
		NULL);								 // name of mapping object

	char* pGameConfigsMemory = (char*)MapViewOfFile(
		hFileMemoryMap,						// handle to map object
		FILE_MAP_READ,						// read permission
		0,
		0,
		tam.QuadPart);

	if(pGameConfigsMemory == NULL)
	{
		_tprintf(TEXT("Error \n"));
		return -1;
	}

	//TODO: Split by tokens
	char* nextToken = NULL;
	int size = strlen(pGameConfigsMemory) + 1;
	char* string = malloc(sizeof(char)*size);
	strcpy_s(string, size, pGameConfigsMemory);
	char seps[] = " :;";
	char* token = strtok_s(string, seps, &nextToken);

	//Get maxplayers
	token = strtok_s(NULL, seps, &nextToken);

	//TODO: Initialize with default values

	gameConfigs->maxPlayers = atoi(token);

	if (gameConfigs->maxPlayers > MAX_PLAYERS)
	{
		gameConfigs->maxPlayers = MAX_PLAYERS;
	}

	for(int i = 1; i < TOTAL_CONFIGS; i++)
	{
		//Get next value
		token = strtok_s(NULL, seps, &nextToken);
		//Get numLevels
		token = strtok_s(NULL, seps, &nextToken);

		if(token == NULL)
		{
			_tprintf(TEXT("Invalid text file parameters! Read the documentation for the correct format!\n"));
			return -1;
		}

		int value = atoi(token);

		switch(i)
		{
			case 1:
				gameConfigs->levels = value;
				break;
			case 2:
				gameConfigs->maxBonus = value;
				break;
			case 3:
				gameConfigs->bonusDuration = value;
				break;
			case 4:
				gameConfigs->bonusProbability = value;
				break;
			case 5:
				gameConfigs->initialLives = value;
				break;
			case 6:
				gameConfigs->numBricks = value;
				break;
			case 7:
				gameConfigs->movementSpeed = value;
				break;
		}
	}

	free(string);

	UnmapViewOfFile(pGameConfigsMemory);
	CloseHandle(hFileMemoryMap);
	CloseHandle(hFile);

	_tprintf(TEXT("\nGameConfigs:\nMax Players: %d\n"), gameConfigs->maxPlayers);
	_tprintf(TEXT("Levels: %d\nMax Bonus: %d\n"), gameConfigs->levels, gameConfigs->maxBonus);
	_tprintf(TEXT("Bonus Duration: %d\nBonus Probability: %d\n"), gameConfigs->bonusDuration, gameConfigs->bonusProbability);
	_tprintf(TEXT("Initial Lives: %d\nNumber of bricks: %d\n"), gameConfigs->initialLives, gameConfigs->numBricks);
	_tprintf(TEXT("Movement Speed: %d\n\n"), gameConfigs->movementSpeed);

	return 1;
}

void createClientsRequestSemaphores(HANDLE* hClientRequestSemaphoreItems, HANDLE* hClientRequestSemaphoreEmpty)
{
	*hClientRequestSemaphoreItems = CreateSemaphore(
		NULL,								 // default security attributes
		0,									 // initial count
		BUFFER_SIZE,						 // maximum count
		SEMAPHORE_CLIENT_REQUEST_ITEMS);     // named semaphore

	*hClientRequestSemaphoreEmpty = CreateSemaphore(
		NULL,								 // default security attributes
		BUFFER_SIZE,						 // initial count
		BUFFER_SIZE,						 // maximum count
		SEMAPHORE_CLIENT_REQUEST_EMPTY);     // named semaphore
}

void createServersResponseSemaphores(HANDLE* hServerResponseSemaphoreItems, HANDLE* hServerResponseSemaphoreEmpty)
{
	*hServerResponseSemaphoreItems = CreateSemaphore(
		NULL,								 // default security attributes
		0,									 // initial count
		BUFFER_SIZE,						 // maximum count
		SEMAPHORE_SERVER_RESPONSE_ITEMS);    // named semaphore

	*hServerResponseSemaphoreEmpty = CreateSemaphore(
		NULL,								 // default security attributes
		BUFFER_SIZE,						 // initial count
		BUFFER_SIZE,						 // maximum count
		SEMAPHORE_SERVER_RESPONSE_EMPTY);    // named semaphore
}

int setupRegistryTopPlayers(HANDLE* hResgistryTop10Key, TCHAR* top10Value, DWORD* playerCount)
{
	DWORD registryStatus, size;
	//Create/open a key in HKEY_CURRENT_USER\Software\ArkanoidGame
	if (RegCreateKeyEx(HKEY_CURRENT_USER, REGISTRY_TOP10_PATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, hResgistryTop10Key, &registryStatus) != ERROR_SUCCESS) {
		_tprintf(TEXT("Error creating/opening registry key(%d)\n"), GetLastError());
		return -1;
	}
	else 
	{
		//If key was created, initialize its values
		if (registryStatus == REG_CREATED_NEW_KEY) {
			_tprintf(TEXT("Key: HKEY_CURRENT_USER\\Software\\ArkanoidGame created\n"));
			//TODO: Remove Dummy Values
			//Create value "TOP10" = "Dummy Value"
			RegSetValueEx(*hResgistryTop10Key, TOP10_REGISTRY_VALUE, 0, REG_SZ, (LPBYTE)TOP10_DUMMY_VALUE, _tcslen(TOP10_DUMMY_VALUE) * sizeof(TCHAR));
			_tcscpy_s(top10Value, TOP10_SIZE, TOP10_DUMMY_VALUE);

			//Create value "TOP10PlayerCount" = 3
			*playerCount = 3;
			RegSetValueEx(*hResgistryTop10Key, TOP10_PLAYER_COUNT, 0, REG_DWORD, (LPBYTE)playerCount, sizeof(DWORD));

			_tprintf(TEXT("Values TOP10 and TOP10PlayerCount saved\n"));
		}
		//If key was opened, read the stored values
		else if (registryStatus == REG_OPENED_EXISTING_KEY) {
			_tprintf(TEXT("Key: HKEY_CURRENT_USER\\Software\\ArkanoidGame opened\n"));

			//Read TOP10 value
			size = TOP10_SIZE;
			RegQueryValueEx(*hResgistryTop10Key, TOP10_REGISTRY_VALUE, NULL, NULL, (LPBYTE)top10Value, &size);
			top10Value[size / sizeof(TCHAR)] = '\0';

			//Read TOP10PlayerCount value
			size = sizeof(*playerCount);
			RegQueryValueEx(*hResgistryTop10Key, TOP10_PLAYER_COUNT, NULL, NULL, (LPBYTE)playerCount, &size);

			_tprintf(TEXT("Read from Registry:%s\n"), top10Value);
		}
	}

	return 1;
}

void convertStringToTopPlayers(TopPlayer* topPlayers, TCHAR* top10Value, DWORD* playerCount)
{
	TCHAR* nextToken = NULL;
	TCHAR value[TOP10_SIZE];
	_tcscpy_s(value, TOP10_SIZE, top10Value);
	TCHAR* token = _tcstok_s(value, TEXT(";"), &nextToken);
	int counter = 0;

	while (token != NULL && counter < MAX_TOP_PLAYERS)
	{
		//Copy username
		_tcscpy_s(topPlayers[counter].username, TAM, token);
		//Get score
		token = _tcstok_s(NULL, TEXT(";"), &nextToken);
		topPlayers[counter].topScore = _tstoi(token);
		//Get next value
		counter++;
		token = _tcstok_s(NULL, TEXT(";"), &nextToken);
	}

	*playerCount = counter;
}

void createGameUpdateEvent(HANDLE* hGameUpdateEvent)
{
	*hGameUpdateEvent = CreateEvent(
		NULL,								 // default security attributes
		TRUE,								 // manual-reset event
		FALSE,								 // initial state is nonsignaled
		EVENT_GAME_UPDATE					 // object name
	);
}