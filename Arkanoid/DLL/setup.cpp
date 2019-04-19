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