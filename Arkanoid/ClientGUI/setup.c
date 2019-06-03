#include "stdafx.h"
#include "setup.h"
#include "resourceConstants.h"

int setupMovementKeys(HANDLE* hRegistryKey,TCHAR* rightMovementKey, TCHAR* leftMovementKey)
{
	DWORD registryStatus, size;
	//Create/open a key in HKEY_CURRENT_USER\Software\ArkanoidGame
	if (RegCreateKeyEx(HKEY_CURRENT_USER, REGISTRY_KEY_MOVEMENT_PATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, hRegistryKey, &registryStatus) != ERROR_SUCCESS) {
		_tprintf(TEXT("Error creating/opening registry key(%d)\n"), GetLastError());
		return -1;
	} else
	{
		//If key was created, initialize its values
		if (registryStatus == REG_CREATED_NEW_KEY) {
			_tprintf(TEXT("Key: HKEY_CURRENT_USER\\Software\\ArkanoidGameClient created\n"));

			//Create value "RIGHT_MOVEMENT_KEY" = "D"
			RegSetValueEx(*hRegistryKey, RIGHT_MOVEMENT_KEY_VALUE, 0, REG_SZ, (LPBYTE)RIGHT_MOVEMENT_DEFAULT_KEY, _tcslen(RIGHT_MOVEMENT_DEFAULT_KEY) * sizeof(TCHAR));
			rightMovementKey[0] = TEXT('D');

			//Create value "LEFT_MOVEMENT_KEY" = "A"
			RegSetValueEx(*hRegistryKey, LEFT_MOVEMENT_KEY_VALUE, 0, REG_SZ, (LPBYTE)LEFT_MOVEMENT_DEFAULT_KEY, _tcslen(LEFT_MOVEMENT_DEFAULT_KEY) * sizeof(TCHAR));
			leftMovementKey[0] = TEXT('A');

			_tprintf(TEXT("Default key movement values saved\n"));
		}
		//If key was opened, read the stored values
		else if (registryStatus == REG_OPENED_EXISTING_KEY) {
			_tprintf(TEXT("Key: HKEY_CURRENT_USER\\Software\\ArkanoidGameClient opened\n"));

			TCHAR tmp[2] = TEXT("");
			size = sizeof(tmp);

			//Read RIGHT_MOVEMENT_KEY value
			RegQueryValueEx(*hRegistryKey, RIGHT_MOVEMENT_KEY_VALUE, NULL, NULL, (LPBYTE)tmp, &size);
			rightMovementKey[0] = tmp[0];

			//Read LEFT_MOVEMENT_KEY value
			RegQueryValueEx(*hRegistryKey, LEFT_MOVEMENT_KEY_VALUE, NULL, NULL, (LPBYTE)tmp, &size);
			leftMovementKey[0] = tmp[0];

			_tprintf(TEXT("Default key values:%c %c\n"), rightMovementKey[0], leftMovementKey[0]);
		}
	}

	return 1;
}

int saveMovementKeys(HANDLE* hRegistryKey, TCHAR* rightMovementKey, TCHAR* leftMovementKey)
{
	//Save right movement key
	RegSetValueEx(*hRegistryKey, RIGHT_MOVEMENT_KEY_VALUE, 0, REG_SZ, (LPBYTE)rightMovementKey, _tcslen(RIGHT_MOVEMENT_DEFAULT_KEY) * sizeof(TCHAR));

	//Save left movement key
	RegSetValueEx(*hRegistryKey, LEFT_MOVEMENT_KEY_VALUE, 0, REG_SZ, (LPBYTE)leftMovementKey, _tcslen(LEFT_MOVEMENT_DEFAULT_KEY) * sizeof(TCHAR));
}

int filter(unsigned int code)
{
	if (code == EXCEPTION_ACCESS_VIOLATION)
	{
		return EXCEPTION_EXECUTE_HANDLER;
	}
	else
	{
		return EXCEPTION_CONTINUE_SEARCH;
	};
}