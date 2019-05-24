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
	HANDLE hGameLogicMutex;
	PipeData* namedPipesData;
}GameVariables;

DWORD WINAPI GameLogic(LPVOID lpParam);
void initializeGame(GameVariables* pGameVariables);
void resetBall(Ball* ball);
void initializeBricks(GameVariables* pGameVariables);
void ballMovement(GameVariables* pGameVariables);
void sendGameUpdate(GameVariables gameVariables);