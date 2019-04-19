#pragma once
#include "stdafx.h"
#include "messages.h"
//Shared Memory
void createClientsSharedMemory(HANDLE* hClientRequestMemoryMap, DWORD clientRequestsSize);
void createServersSharedMemory(HANDLE* hServerResponseMemoryMap, DWORD serverResponsesSize);

//Semaphore
void createClientsRequestSemaphores(HANDLE* hClientRequestSemaphoreItems, HANDLE* hClientRequestSemaphoreEmpty);
void createServersResponseSemaphores(HANDLE* hServerResponseSemaphoreItems, HANDLE* hServerResponseSemaphoreEmpty);