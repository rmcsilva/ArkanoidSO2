#pragma once
#include "stdafx.h"
#include "gameStructs.h"
#include "namedPipeLogic.h"
#include "gameConstants.h"

#define BARRIER_MARGIN_PERCENTAGE 0.5
#define BARRIER_AREA (GAME_BOARD_WIDTH * 0.8)
#define BARRIER_REMAINDER (GAME_BOARD_WIDTH - BARRIER_AREA)

#define GAME_BALL_HITBOX (GAME_BALL_WIDTH / 5)
#define GAME_BRICK_HITBOX (BRICKS_WIDTH / 10)

//Directions
#define DIRECTION_Y_UP		-1
#define DIRECTION_Y_DOWN	1
#define DIRECTION_X_RIGHT	1
#define DIRECTION_X_LEFT	-1
#define DIRECTION_RIGHT_TO_LEFT 0
#define DIRECTION_LEFT_TO_RIGHT 1

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
void detectBallCollision(GameVariables* pGameVariables, int index);
void ballAndBrickCollision(GameVariables* pGameVariables, int index, int x, int y, int directionX);
void ballAndBarrierCollision(GameVariables* pGameVariables, int index, int x, int y, int directionX);
void sendGameUpdate(GameVariables gameVariables);
void assignUsersToGame(GameData* pGameData, Player* users, int currentUsers);
int getPlayerToTheRight(GameData gameData, int userPosition);
int getPlayerToTheLeft(GameData gameData, int userPosition);
