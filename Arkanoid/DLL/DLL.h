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

#ifdef __cplusplus
extern "C" {
#endif
	//Global DLL variables
	//Indicate if user is local or remote
	extern DLL_IMP_API int isLocalUser;
	//Imported/Exported functions
	DLL_IMP_API int test(void);
	DLL_IMP_API int login(TCHAR* username);
	DLL_IMP_API void sendMessage(int messageType);
	DLL_IMP_API int receiveMessage(int messageType);
	DLL_IMP_API void logout(void);
#ifdef __cplusplus
}
#endif