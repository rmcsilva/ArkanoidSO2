#pragma once
#include "stdafx.h"
#include "gameStructs.h"
#include "namedPipeLogic.h"

typedef struct GameConfigs {
	int maxPlayers;
	int levels;
	int maxBonus;
	int bonusDuration;
	int bonusProbability;
	int initialLives;
	int numBricks;
	int movementSpeed;
}GameConfigs;

typedef struct GameVariables {
	GameData* pGameData;
	GameConfigs gameConfigs;
	HANDLE hGameUpdateEvent;
	PipeData* namedPipesData;
}GameVariables;

DWORD WINAPI GameLogic(LPVOID lpParam);
void ballMovement(GameData* pGameData);
void sendGameUpdate(GameVariables gameVariables);