#pragma once
#include "stdafx.h"
#include "messages.h"

void createClientsSharedMemory(HANDLE* hClientRequestMemoryMap, DWORD clientRequestsSize);
void createServersSharedMemory(HANDLE* hServerResponseMemoryMap, DWORD serverResponsesSize);