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