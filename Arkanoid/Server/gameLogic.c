#define _CRT_RAND_S

#include "stdafx.h"
#include "gameLogic.h"
#include "gameStructs.h"
#include "messages.h"
#include <stdlib.h>

TCHAR debugText[TAM];
unsigned int random;

DWORD WINAPI GameLogic(LPVOID lpParam)
{
	GameVariables* pGameVariables = (GameVariables*)lpParam;
	GameData* pGameData = pGameVariables->pGameData;

	int counter = 0;

	//TODO: Initialize Game Variables
	initializeGame(pGameVariables);

	while (pGameData->gameStatus != GAME_OVER)
	{
		if(pGameData->lives == 0)
		{
			break;
		}

		if(pGameData->numBricks == 0)
		{
			//TODO: Advance level, reset balls, reset bricks
		}

		ballMovement(pGameVariables);

		if (counter == 100)
		{
			pGameData->gameStatus = GAME_OVER;
		}
		else
		{
			counter++;
		}

		sendGameUpdate(*pGameVariables);
		Sleep(1000);
	}

	pGameData->gameStatus = GAME_OVER;

	return 1;
}

void initializeGame(GameVariables* pGameVariables)
{
	GameData* pGameData = pGameVariables->pGameData;

	pGameData->numBalls = 1;
	pGameData->level = 1;
	pGameData->lives = pGameVariables->gameConfigs.initialLives;
	pGameData->numBonus = 0;
	pGameData->numBricks = 0;

	for (int i = 0; i < pGameData->numBalls; i++)
	{
		resetBall(&pGameData->ball[i]);
	}

	initializeBricks(pGameVariables);
}

void resetBall(Ball* ball)
{
	ball->velocityRatio = 1;
	rand_s(&random);
	ball->directionY = random % 2 == 1 
						? 1
						: -1;
	rand_s(&random);
	ball->directionX = random % 2 == 1 
						? 1 
						: -1;
	ball->playerID = UNDEFINED_ID;
	ball->position.x = GAME_BOARD_WIDTH / 2;
	ball->position.y = GAME_BOARD_HEIGHT / 2;
}

void initializeBricks(GameVariables* pGameVariables)
{
	GameData* pGameData = pGameVariables->pGameData;
	int index;
	int maxResistance = pGameData->level > BRICKS_MAX_RESISTANCE 
						? BRICKS_MAX_RESISTANCE 
						: pGameData->level;
	int probability;

	for(int i = 0; i < MAX_BRICK_LINES; i++)
	{
		index = i * MAX_BRICKS_IN_LINE;
		pGameData->brick[index].position.x = i % 2 ? BRICKS_MARGIN : 0;

		for (int j = 0; j < MAX_BRICKS_IN_LINE; j++)
		{
			if(pGameData->numBricks == pGameVariables->gameConfigs.numBricks)
			{
				break;
			}
			pGameData->numBricks++;
			index = j + i * MAX_BRICKS_IN_LINE;
			//Setup brick position
			pGameData->brick[index].position.x += (BRICKS_WIDTH + BRICKS_MARGIN) * j + GAME_BORDER_LEFT;
			pGameData->brick[index].position.y = BRICKS_HEIGHT * i + GAME_BORDER_TOP;
			pGameData->brick[index].hasBonus = FALSE;
			rand_s(&random);
			pGameData->brick[index].resistance = (random % maxResistance) + 1;

			//Setup bonus
			rand_s(&random);
			probability = (random % 100) + 1;
			if (probability < pGameVariables->gameConfigs.bonusProbability)
			{
				pGameData->brick[index].hasBonus = TRUE;
				rand_s(&random);
				pGameData->brick[index].bonusType = random % TOTAL_BONUS;
			}
		}
	}
}
void sendGameUpdate(GameVariables gameVariables)
{
	SetEvent(gameVariables.hGameUpdateEvent);
	ResetEvent(gameVariables.hGameUpdateEvent);

	for(int i=0; i < gameVariables.gameConfigs.maxPlayers; i++)
	{
		if(gameVariables.namedPipesData[i].userID != UNDEFINED_ID)
		{
			sendGameNamedPipe(*gameVariables.pGameData, &gameVariables.namedPipesData[i]);
		}
	}
}

void ballMovement(GameVariables* pGameVariables)
{
	GameData* pGameData = pGameVariables->pGameData;

	for(int i=0; i < pGameData->numBalls; i++)
	{
		int velocity = pGameVariables->gameConfigs.movementSpeed * pGameData->ball[i].velocityRatio;
		pGameData->ball[i].position.x += velocity * pGameData->ball[i].directionX;
		pGameData->ball[i].position.y += velocity * pGameData->ball[i].directionY;

		_stprintf_s(debugText, TAM, TEXT("\nBall %d position x: %d y: %d\n"), i, pGameData->ball[0].position.x, pGameData->ball[0].position.y);
		OutputDebugString(debugText);
	}
}