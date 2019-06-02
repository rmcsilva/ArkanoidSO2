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
				if(value > MAX_BONUS)
				{
					gameConfigs->maxBonus = MAX_BONUS;
				} else
				{
					gameConfigs->maxBonus = value;
				}
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
				if(value > MAX_BRICKS)
				{
					gameConfigs->numBricks = MAX_BRICKS;
				} else
				{
					gameConfigs->numBricks = value;
				}
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

int setupRegistryTopPlayers(HANDLE* hRegistryTop10Key, TCHAR* top10Value, DWORD* playerCount)
{
	DWORD registryStatus, size;
	//Create/open a key in HKEY_CURRENT_USER\Software\ArkanoidGame
	if (RegCreateKeyEx(HKEY_CURRENT_USER, REGISTRY_TOP10_PATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, hRegistryTop10Key, &registryStatus) != ERROR_SUCCESS) {
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
			RegSetValueEx(*hRegistryTop10Key, TOP10_REGISTRY_VALUE, 0, REG_SZ, (LPBYTE)TOP10_DUMMY_VALUE, _tcslen(TOP10_DUMMY_VALUE) * sizeof(TCHAR));
			_tcscpy_s(top10Value, TOP10_SIZE, TOP10_DUMMY_VALUE);

			//Create value "TOP10PlayerCount" = 3
			*playerCount = 3;
			RegSetValueEx(*hRegistryTop10Key, TOP10_PLAYER_COUNT, 0, REG_DWORD, (LPBYTE)playerCount, sizeof(DWORD));

			_tprintf(TEXT("Values TOP10 and TOP10PlayerCount saved\n"));
		}
		//If key was opened, read the stored values
		else if (registryStatus == REG_OPENED_EXISTING_KEY) {
			_tprintf(TEXT("Key: HKEY_CURRENT_USER\\Software\\ArkanoidGame opened\n"));

			//Read TOP10 value
			size = TOP10_SIZE;
			RegQueryValueEx(*hRegistryTop10Key, TOP10_REGISTRY_VALUE, NULL, NULL, (LPBYTE)top10Value, &size);
			top10Value[size / sizeof(TCHAR)] = '\0';

			//Read TOP10PlayerCount value
			size = sizeof(*playerCount);
			RegQueryValueEx(*hRegistryTop10Key, TOP10_PLAYER_COUNT, NULL, NULL, (LPBYTE)playerCount, &size);

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

void convertTopPlayersToString(TopPlayer* topPlayers, TCHAR* top10Value, DWORD* playerCount)
{
	TCHAR temp[TOP10_SIZE] = TEXT("");
	TCHAR newTop10[TOP10_SIZE] = TEXT("");

	for (int i = 0; i < *playerCount; ++i)
	{
		//Copy username
		_stprintf_s(temp, MAX, TEXT("%s;"), topPlayers[i].username);
		_tcscat_s(newTop10, TOP10_SIZE, temp);

		//Get score
		_stprintf_s(temp, MAX, TEXT("%d;"), topPlayers[i].topScore);
		_tcscat_s(newTop10, TOP10_SIZE, temp);
	}
	//Copy temporary string to actual value
	_tcscpy_s(top10Value, TOP10_SIZE, newTop10);
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

int setupNamedPipes(PipeData* namedPipesData, HANDLE* hPipeRequestsEvents, HANDLE* hPipeGameUpdateEvents, int numPipes)
{
	TCHAR pipeClientRequestsName[MAX];
	TCHAR pipeServerResponsesName[MAX];
	TCHAR pipeGameName[MAX];

	int bufferSize = MAX - 1;

	TCHAR dot[] = TEXT(".");

	_stprintf_s(pipeClientRequestsName, bufferSize, NAMED_PIPE_CLIENT_REQUESTS, dot);
	_stprintf_s(pipeServerResponsesName, bufferSize, NAMED_PIPE_SERVER_RESPONSES, dot);
	_stprintf_s(pipeGameName, bufferSize, NAMED_PIPE_GAME, dot);

	SECURITY_ATTRIBUTES sa;
	setupPipeSecurityAttributes(&sa);

	for (int i = 0; i < numPipes; i++)
	{
		namedPipesData[i].userID = UNDEFINED_ID;
		
		hPipeRequestsEvents[i] = CreateEvent(
			NULL,	 // custom security attribute 
			TRUE,    // manual-reset event 
			TRUE,    // initial state = signaled 
			NULL);   // unnamed event object

		if (hPipeRequestsEvents[i] == NULL)
		{
			_tprintf(TEXT("CreateEvent failed with %d.\n"), GetLastError());
			return -1;
		}

		hPipeGameUpdateEvents[i] = CreateEvent(
			NULL,	 // custom security attribute 
			TRUE,    // manual-reset event 
			TRUE,    // initial state = signaled 
			NULL);   // unnamed event object

		if (hPipeGameUpdateEvents[i] == NULL)
		{
			_tprintf(TEXT("CreateEvent failed with %d.\n"), GetLastError());
			return -1;
		}

		//Setup client requests events
		ZeroMemory(&namedPipesData[i].overlappedRequests, sizeof(namedPipesData[i].overlappedRequests));
		namedPipesData[i].overlappedRequests.hEvent = hPipeRequestsEvents[i];

		//Setup game update events
		ZeroMemory(&namedPipesData[i].overlappedGame, sizeof(namedPipesData[i].overlappedGame));
		namedPipesData[i].overlappedGame.hEvent = hPipeGameUpdateEvents[i];

		createMessageNamedPipe(
			&namedPipesData[i].hClientRequestsPipe,		// pipe handle
			pipeClientRequestsName,						// pipe name 
			PIPE_ACCESS_INBOUND,						// server -> read / client -> write access
			numPipes,									// number of instances 
			sizeof(ClientMessage),						// buffer size
			sa);										// Custom security Attributes

		if (namedPipesData[i].hClientRequestsPipe == INVALID_HANDLE_VALUE)
		{
			_tprintf(TEXT("CreateNamedPipe failed with %d.\n"), GetLastError());
			return -1;
		}

		createMessageNamedPipe(
			&namedPipesData[i].hServerResponsesPipe,	// pipe handle
			pipeServerResponsesName,					// pipe name 
			PIPE_ACCESS_OUTBOUND,						// client -> read / server -> write access 
			numPipes,									// number of instances 
			sizeof(ServerMessage),						// buffer size
			sa);										// Custom security Attributes


		if (namedPipesData[i].hServerResponsesPipe == INVALID_HANDLE_VALUE)
		{
			_tprintf(TEXT("CreateNamedPipe failed with %d.\n"), GetLastError());
			return -1;
		}

		createMessageNamedPipe(
			&namedPipesData[i].hGamePipe,		// pipe handle
			pipeGameName,						// pipe name 
			PIPE_ACCESS_OUTBOUND,				// client -> read / server -> write access 
			numPipes,							// number of instances 
			sizeof(GameData),					// buffer size
			sa);								// Custom security Attributes

		if (namedPipesData[i].hGamePipe == INVALID_HANDLE_VALUE)
		{
			_tprintf(TEXT("CreateNamedPipe failed with %d.\n"), GetLastError());
			return -1;
		}

		namedPipesData[i].fPendingIORequests = newPlayerPipeConnection(
			namedPipesData[i].hClientRequestsPipe,
			&namedPipesData[i].overlappedRequests);

		namedPipesData[i].dwStateRequests = namedPipesData[i].fPendingIORequests
			? CONNECTING_STATE :		// still connecting 
			READING_REQUEST_STATE;		// ready to read clients request

		newPlayerPipeConnection(
			namedPipesData[i].hServerResponsesPipe,
			&namedPipesData[i].overlappedRequests);

		namedPipesData[i].fPendingIOGame = newPlayerPipeConnection(
			namedPipesData[i].hGamePipe,
			&namedPipesData[i].overlappedGame);

		namedPipesData[i].dwStateGame = namedPipesData[i].fPendingIOGame
			? CONNECTING_STATE :		// still connecting 
			WRITING_GAME_STATE;			// ready to read clients request
	}

	return 1;
}

void createMessageNamedPipe(HANDLE* hPipe, TCHAR* pipeName, DWORD openMode, DWORD maxPlayers, DWORD bufferSize, SECURITY_ATTRIBUTES sa)
{
	//TODO: Add security attributes 

	*hPipe = CreateNamedPipe(
		pipeName,						// pipe name 
		openMode |						// openMode access 
		FILE_FLAG_OVERLAPPED,			// overlapped mode 
		PIPE_TYPE_MESSAGE |				// message-type pipe 
		PIPE_READMODE_MESSAGE |			// message-read mode
		PIPE_ACCEPT_REMOTE_CLIENTS |	// accepts remote clients
		PIPE_WAIT,						// blocking mode 
		maxPlayers,						// number of instances 
		bufferSize,						// output buffer size 
		bufferSize,						// input buffer size 
		0,								// client time-out 
		&sa);							// custom security attributes
}

BOOL newPlayerPipeConnection(HANDLE hPipe, LPOVERLAPPED lpo)
{
	BOOL fConnected, fPendingIO = FALSE;

	// Start an overlapped connection for this pipe instance. 
	fConnected = ConnectNamedPipe(hPipe, lpo);

	// Overlapped ConnectNamedPipe should return zero. 
	if (fConnected)
	{
		_tprintf(TEXT("ConnectNamedPipe failed with %d.\n"), GetLastError());
		return 0;
	}

	switch (GetLastError())
	{
		// The overlapped connection in progress. 
		case ERROR_IO_PENDING:
			fPendingIO = TRUE;
			break;

		// Client is already connected, so signal an event. 
		case ERROR_PIPE_CONNECTED:
			if (SetEvent(lpo->hEvent))
				break;

		// If an error occurs during the connect operation... 
		default:
		{
			_tprintf(TEXT("ConnectNamedPipe failed with %d.\n"), GetLastError());
			return 0;
		}
	}

	return fPendingIO;
}

void setupPipeSecurityAttributes(SECURITY_ATTRIBUTES * sa)
{
	PSECURITY_DESCRIPTOR pSD;
	PACL pAcl;
	EXPLICIT_ACCESS ea;
	PSID pEveryoneSID = NULL, pAdminSID = NULL;
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
	TCHAR str[256];

	pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (pSD == NULL) {
		_tprintf(TEXT("Erro LocalAlloc!!!\n"));
		return;
	}
	if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)) {
		_tprintf(TEXT("Erro IniSec!!!\n"));
		return;
	}

	// Create a well-known SID for the Everyone group.
	if (!AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID,
		0, 0, 0, 0, 0, 0, 0, &pEveryoneSID))
	{
		_stprintf_s(str, 256, TEXT("AllocateAndInitializeSid() error %u\n"), GetLastError());
		_tprintf(str);
		if (pEveryoneSID)
			FreeSid(pEveryoneSID);
		if (pAdminSID)
			FreeSid(pAdminSID);
		if (pSD)
			LocalFree(pSD);
	}
	else {
		_tprintf(TEXT("AllocateAndInitializeSid() for the Everyone group is OK\n"));
	}

	ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));

	ea.grfAccessPermissions = GENERIC_READ | GENERIC_WRITE;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea.Trustee.ptstrName = (LPTSTR)pEveryoneSID;

	if (SetEntriesInAcl(1, &ea, NULL, &pAcl) != ERROR_SUCCESS) {
		_tprintf(TEXT("Erro SetAcl!!!"));
		return;
	}

	if (!SetSecurityDescriptorDacl(pSD, TRUE, pAcl, FALSE)) {
		_tprintf(TEXT("Erro IniSec!!!"));
		return;
	}

	sa->nLength = sizeof(*sa);
	sa->lpSecurityDescriptor = pSD;
	sa->bInheritHandle = TRUE;
}