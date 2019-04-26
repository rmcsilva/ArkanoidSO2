#pragma once
#include "stdafx.h"
#include "messages.h"
#include "gameStructs.h"
#include "gameConstants.h"

//Shared Memory
void createClientsSharedMemory(HANDLE* hClientRequestMemoryMap, DWORD clientRequestsSize);
void createServersSharedMemory(HANDLE* hServerResponseMemoryMap, DWORD serverResponsesSize);
void createGameSharedMemory(HANDLE* hGameDataMemoryMap, DWORD gameDataSize);
GameData* mapReadWriteGameSharedMemory(HANDLE* hGameDataMemoryMap, DWORD gameDataSize);

//Semaphore
void createClientsRequestSemaphores(HANDLE* hClientRequestSemaphoreItems, HANDLE* hClientRequestSemaphoreEmpty);
void createServersResponseSemaphores(HANDLE* hServerResponseSemaphoreItems, HANDLE* hServerResponseSemaphoreEmpty);

//Registry
int setupRegistryTopPlayers(HANDLE* hResgistryTop10Key, TCHAR* top10Value, DWORD* playerCount);
void convertStringToTopPlayers(TopPlayer* topPlayers, TCHAR* top10Value, DWORD* playerCount);

//Events
void createGameUpdateEvent(HANDLE* hGameUpdateEvent);