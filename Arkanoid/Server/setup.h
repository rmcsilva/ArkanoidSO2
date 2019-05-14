#pragma once
#include "stdafx.h"
#include "messages.h"
#include "gameStructs.h"
#include "gameConstants.h"
#include "gameLogic.h"
#include <stdlib.h>

//Number of total configurations in the configs file
#define TOTAL_CONFIGS 8

//Shared Memory
void createClientsSharedMemory(HANDLE* hClientRequestMemoryMap, DWORD clientRequestsSize);
void createServersSharedMemory(HANDLE* hServerResponseMemoryMap, DWORD serverResponsesSize);
void createGameSharedMemory(HANDLE* hGameDataMemoryMap, DWORD gameDataSize);
GameData* mapReadWriteGameSharedMemory(HANDLE* hGameDataMemoryMap, DWORD gameDataSize);
int setupInitialGameConfigs(TCHAR* filename[_MAX_FNAME], GameConfigs* gameConfigs);

//Semaphore
void createClientsRequestSemaphores(HANDLE* hClientRequestSemaphoreItems, HANDLE* hClientRequestSemaphoreEmpty);
void createServersResponseSemaphores(HANDLE* hServerResponseSemaphoreItems, HANDLE* hServerResponseSemaphoreEmpty);

//Registry
int setupRegistryTopPlayers(HANDLE* hResgistryTop10Key, TCHAR* top10Value, DWORD* playerCount);
void convertStringToTopPlayers(TopPlayer* topPlayers, TCHAR* top10Value, DWORD* playerCount);

//Events
void createGameUpdateEvent(HANDLE* hGameUpdateEvent);

//Named Pipes
int setupNamedPipes(PipeData* namedPipesData, HANDLE* hPipeEvents, int numPipes);
void createMessageNamedPipe(HANDLE* hPipe, TCHAR* pipeName, DWORD openMode, DWORD maxPlayers, DWORD bufferSize);
BOOL newPlayerPipeConnection(HANDLE hPipe, LPOVERLAPPED lpo);