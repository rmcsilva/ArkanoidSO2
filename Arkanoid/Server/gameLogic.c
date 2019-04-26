#include "stdafx.h"
#include "gameLogic.h"
#include "gameStructs.h"

DWORD WINAPI GameLogic(LPVOID lpParam)
{
	GameVariables* pGameVariables = (GameVariables*)lpParam;
	GameData* pGameData = pGameVariables->pGameData;

	int counter = 0;

	//TODO: Initialize Game Variables
	pGameData->numBalls = 1;
	pGameData->ball[0].position.x = 0;
	pGameData->ball[0].position.y = 0;

	while (pGameData->gameStatus != GAME_OVER)
	{
		ballMovement(pGameData);

		_tprintf(TEXT("\nBall position x: %d y: %d\n"), pGameData->ball[0].position.x, pGameData->ball[0].position.y);

		if (counter == 100)
		{
			pGameData->gameStatus = GAME_OVER;
		}
		else
		{
			counter++;
		}

		SetEvent(pGameVariables->hGameUpdateEvent);
		ResetEvent(pGameVariables->hGameUpdateEvent);
		Sleep(1000);
	}

	pGameData->gameStatus = GAME_OVER;

	return 1;
}

void ballMovement(GameData* pGameData)
{
	for(int i=0; i < pGameData->numBalls; i++)
	{
		pGameData->ball[i].position.x += 1;
		pGameData->ball[i].position.y += 1;
	}
}