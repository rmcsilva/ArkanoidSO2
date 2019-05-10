#pragma once
#include "stdafx.h"

#ifdef DLL_EXPORTS
#define DLL_IMP_API __declspec(dllexport)
#else
#define DLL_IMP_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif
	//Imported/Exported functions
	DLL_IMP_API int readInt(void);
	DLL_IMP_API TCHAR* trimWhiteSpace(TCHAR* str);
#ifdef __cplusplus
}
#endif