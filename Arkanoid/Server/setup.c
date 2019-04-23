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
	else {
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
		//Se a chave foi aberta, ler os valores lá guardados
		else if (registryStatus == REG_OPENED_EXISTING_KEY) {
			_tprintf(TEXT("Key: HKEY_CURRENT_USER\\Software\\ArkanoidGame opened\n"));

			//Read TOP10 value
			size = TOP10_SIZE;
			RegQueryValueEx(*hResgistryTop10Key, TOP10_REGISTRY_VALUE, NULL, NULL, (LPBYTE)top10Value, &size);
			top10Value[size / sizeof(TCHAR)] = '\0';

			//Read TOP10PlayerCount value
			size = sizeof(*playerCount);
			RegQueryValueEx(*hResgistryTop10Key, TOP10_PLAYER_COUNT, NULL, NULL, (LPBYTE)playerCount, &size);
			
			//_stprintf_s(str, TAM, TEXT("Autor:%s Versão:%d\n"), autor, versao);
			_tprintf(TEXT("Lido do Registry:%s\n"), top10Value);
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

	while (token != NULL && counter<10)
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