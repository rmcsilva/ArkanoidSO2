#include "stdafx.h"
#include "setup.h"
#include "resourceConstants.h"

void openClientsSharedMemory(HANDLE* hClientRequestMemoryMap)
{
	*hClientRequestMemoryMap = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,				// read/write access
		FALSE,								// do not inherit the name
		BUFFER_MEMORY_CLIENT_REQUESTS);     // name of mapping object
}

void openServersSharedMemory(HANDLE* hServerResponseMemoryMap)
{
	*hServerResponseMemoryMap = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,				// read/write access
		FALSE,								// do not inherit the name
		BUFFER_MEMORY_SERVER_RESPONSES);    // name of mapping object
}

void openGameSharedMemory(HANDLE* hGameDataMemoryMap)
{
	*hGameDataMemoryMap = OpenFileMapping(
		FILE_MAP_READ,						// read access
		FALSE,								// do not inherit the name
		BUFFER_MEMORY_GAME);				// name of mapping object
}

ClientMessageControl* mapClientsSharedMemory(HANDLE* hClientRequestMemoryMap, DWORD clientRequestSize)
{
	ClientMessageControl* pClientRequestMemory = (ClientMessageControl*)MapViewOfFile(
		*hClientRequestMemoryMap,			// handle to map object
		FILE_MAP_ALL_ACCESS,				// read/write permission
		0,
		0,
		clientRequestSize);

	return pClientRequestMemory;
}

ServerMessageControl* mapServersSharedMemory(HANDLE* hServerResponseMemoryMap, DWORD serverResponseSize)
{
	ServerMessageControl* pServerResponseMemory = (ServerMessageControl*)MapViewOfFile(
		*hServerResponseMemoryMap,			// handle to map object
		FILE_MAP_ALL_ACCESS,				// read/write permission
		0,
		0,
		serverResponseSize);

	return pServerResponseMemory;
}

GameData* mapReadOnlyGameSharedMemory(HANDLE* hGameDataMemoryMap, DWORD gameDataSize)
{
	GameData* pGameDataMemory = (GameData*)MapViewOfFile(
		*hGameDataMemoryMap,				// handle to map object
		FILE_MAP_READ,						// read permission
		0,
		0,
		gameDataSize);

	return pGameDataMemory;
}

void createClientsRequestMutex(HANDLE* hClientRequestMutex)
{
	*hClientRequestMutex = CreateMutex(
		NULL,								 // default security attributes
		FALSE,								 // initially not owned
		MUTEX_CLIENT_REQUEST);               // named mutex
}

void createServersResponseMutex(HANDLE* hServerResponseMutex)
{
	*hServerResponseMutex = CreateMutex(
		NULL,								 // default security attributes
		FALSE,								 // initially not owned
		MUTEX_SERVER_RESPONSES);             // named mutex
}

void openClientsRequestSemaphores(HANDLE* hClientRequestSemaphoreItems, HANDLE* hClientRequestSemaphoreEmpty)
{
	*hClientRequestSemaphoreItems = OpenSemaphore(
		SEMAPHORE_ALL_ACCESS,				 // default security attributes
		TRUE,								 // inherit handle
		SEMAPHORE_CLIENT_REQUEST_ITEMS);     // named semaphore

	*hClientRequestSemaphoreEmpty = OpenSemaphore(
		SEMAPHORE_ALL_ACCESS,				 // default security attributes
		TRUE,								 // inherit handle
		SEMAPHORE_CLIENT_REQUEST_EMPTY);     // named semaphore
}

void openServersResponseSemaphores(HANDLE* hServerResponseSemaphoreItems, HANDLE* hServerResponseSemaphoreEmpty)
{
	*hServerResponseSemaphoreItems = OpenSemaphore(
		SEMAPHORE_ALL_ACCESS,				 // default security attributes
		TRUE,								 // inherit handle
		SEMAPHORE_SERVER_RESPONSE_ITEMS);    // named semaphore

	*hServerResponseSemaphoreEmpty = OpenSemaphore(
		SEMAPHORE_ALL_ACCESS,				 // default security attributes
		TRUE,								 // inherit handle
		SEMAPHORE_SERVER_RESPONSE_EMPTY);    // named semaphore
}

void createClientMessageCheckEvent(HANDLE* hClientMessageCheckEvent)
{
	*hClientMessageCheckEvent = CreateEvent(
		NULL,								 // default security attributes
		TRUE,								 // manual-reset event
		FALSE,								 // initial state is nonsignaled
		EVENT_CLIENT_MESSAGE_CHECK			 // object name
	);
}

void openGameUpdateEvent(HANDLE* hGameUpdateEvent)
{
	*hGameUpdateEvent = OpenEvent(
		EVENT_ALL_ACCESS,					 // all access rights
		FALSE,								 // does not inherit handle
		EVENT_GAME_UPDATE					 // object name
	);
}