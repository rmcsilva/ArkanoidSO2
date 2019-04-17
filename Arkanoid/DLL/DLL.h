#pragma once
//DLL.h

#ifndef DLL_H
#define DLL_H
#include "GameStructs.h"
#include "stdafx.h"
#endif

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
#ifdef __cplusplus
}
#endif