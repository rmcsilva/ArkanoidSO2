#pragma once
//DLL.h

#pragma once

#include "stdafx.h"
#include "gameStructs.h"
#include "gameConstants.h"
#include "messages.h"
#include "resourceConstants.h"
#include "setup.h"

#include <windows.h>

#ifdef DLL_EXPORTS
#define DLL_IMP_API __declspec(dllexport)
#else
#define DLL_IMP_API __declspec(dllimport)
#endif

//Shared Memory
int loginSharedMemory(TCHAR* loginUsername);
void sendMessageSharedMemory(int messageType);
int receiveMessageSharedMemory(int messageType);
void waitForResponseOnEvent();
void logoutSharedMemory();

//Named Pipe
int loginNamedPipe(TCHAR* loginUsername);
void sendMessageNamedPipe(int messageType);
int receiveMessageNamedPipe(int messageType);
void logoutNamedPipe();

#ifdef __cplusplus
extern "C" {
#endif
	//Global DLL variables
	extern DLL_IMP_API BOOL isLocalUser;
	extern DLL_IMP_API int inGame;
	extern DLL_IMP_API TCHAR username[TAM];
	extern DLL_IMP_API TCHAR top10[TOP10_SIZE];
	//Imported/Exported functions
	DLL_IMP_API int login(TCHAR* loginUsername, TCHAR* ip);
	DLL_IMP_API void sendMessage(int messageType);
	DLL_IMP_API int receiveMessage(int messageType);
	DLL_IMP_API int receiveBroadcast();
	DLL_IMP_API void logout(void);
#ifdef __cplusplus
}
#endif