#pragma once
#include "stdafx.h"
#include "messages.h"

#ifdef DLL_EXPORTS
#define DLL_IMP_API __declspec(dllexport)
#else
#define DLL_IMP_API __declspec(dllimport)
#endif

void openClientsSharedMemory(HANDLE* hClientRequestMemoryMap);
void openServersSharedMemory(HANDLE* hServerResponseMemoryMap);
//void setupGameSharedMemory();

#ifdef __cplusplus
extern "C" {
#endif
	//Imported/Exported functions
	DLL_IMP_API ClientMessageControl* mapClientsSharedMemory(HANDLE* hClientRequestMemoryMap, DWORD clientRequestSize);
	DLL_IMP_API ServerMessageControl* mapServersSharedMemory(HANDLE* hServerResponseMemoryMap, DWORD serverResponseSize);
#ifdef __cplusplus
}
#endif