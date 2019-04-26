#pragma once
#include "stdafx.h"
#include "gameStructs.h"

typedef struct GameVariables {
	GameData* pGameData;
	HANDLE hGameUpdateEvent;
}GameVariables;

DWORD WINAPI GameLogic(LPVOID lpParam);
void ballMovement(GameData* pGameData);