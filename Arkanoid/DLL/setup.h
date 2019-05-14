#pragma once
#include "stdafx.h"
#include "messages.h"
#include "GameStructs.h"

#ifdef DLL_EXPORTS
#define DLL_IMP_API __declspec(dllexport)
#else
#define DLL_IMP_API __declspec(dllimport)
#endif

//Shared Memory
void openClientsSharedMemory(HANDLE* hClientRequestMemoryMap);
void openServersSharedMemory(HANDLE* hServerResponseMemoryMap);
void openGameSharedMemory(HANDLE* hGameDataMemoryMap);

//Mutex
void createClientsRequestMutex(HANDLE* hClientRequestMutex);
void createServersResponseMutex(HANDLE* hServerResponseMutex);

//Semaphore
void openClientsRequestSemaphores(HANDLE* hClientRequestSemaphoreItems, HANDLE* hClientRequestSemaphoreEmpty);
void openServersResponseSemaphores(HANDLE* hServerResponseSemaphoreItems, HANDLE* hServerResponseSemaphoreEmpty);

//Event
void createClientMessageCheckEvent(HANDLE* hClientMessageCheckEvent);
void openGameUpdateEvent(HANDLE* hGameUpdateEvent);

//Shared Memory
GameData* mapReadOnlyGameSharedMemory(HANDLE* hGameDataMemoryMap, DWORD gameDataSize);

//Named Pipe
void openNamedPipe(HANDLE* hPipe, TCHAR* pipeName, DWORD accessMode);
BOOL changePipeToMessageMode(HANDLE hPipe);

#ifdef __cplusplus
extern "C" {
#endif
	//Imported/Exported functions
	DLL_IMP_API ClientMessageControl* mapClientsSharedMemory(HANDLE* hClientRequestMemoryMap, DWORD clientRequestSize);
	DLL_IMP_API ServerMessageControl* mapServersSharedMemory(HANDLE* hServerResponseMemoryMap, DWORD serverResponseSize);
#ifdef __cplusplus
}
#endif