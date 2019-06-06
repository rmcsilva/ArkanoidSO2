#define _CRT_RAND_S

#define _10MILLISECONDS -100000LL
#define _100MILLISECONDS -100000LL
#define _1SECOND -10000000LL

#include "stdafx.h"
#include "gameLogic.h"
#include "gameStructs.h"
#include "messages.h"
#include <stdlib.h>
#include "setup.h"

TCHAR debugText[MAX];
unsigned int random;

HANDLE hBallControlMutex;
HANDLE hBonusMutex;
HANDLE hBrickMovementMutex;

HANDLE hBonusEvent;

HANDLE hBonusTimeoutEvent[MAX_BONUS];

int bonusCounter = 0;

DWORD WINAPI GameLogic(LPVOID lpParam)
{
	GameVariables* pGameVariables = (GameVariables*)lpParam;
	GameData* pGameData = pGameVariables->pGameData;

	HANDLE hGameWait;
	LARGE_INTEGER timeToWait;

	HANDLE hBonusThread;
	DWORD dwBonusThreadID;

	timeToWait.QuadPart = _10MILLISECONDS;

	// Create an unnamed waitable timer.
	hGameWait = CreateWaitableTimer(NULL, TRUE, NULL);
	if (hGameWait == NULL)
	{
		_tprintf(TEXT("CreateWaitableTimer failed (%d)\n"), GetLastError());
		return -1;
	}

	//Initialize Game Variables
	initializeGame(pGameVariables);

	//Thread to handle the bonus logic
	hBonusThread = CreateThread(
		NULL,						// default security attributes
		0,							// use default stack size  
		BonusLogic,					// thread function name
		(LPVOID)pGameVariables,		// argument to thread function 
		0,							// use default creation flags 
		&dwBonusThreadID);			// returns the thread identifier 

	if (hBonusThread == NULL)
	{
		_tprintf(TEXT("\nError creating bonus thread!\n\n"));
		ExitProcess(3);
	}

	while (pGameData->gameStatus != GAME_OVER)
	{
		// Set a timer to wait.
		if (!SetWaitableTimer(hGameWait, &timeToWait, 0, NULL, NULL, 0))
		{
			_tprintf(TEXT("SetWaitableTimer failed (%d)\n"), GetLastError());
			return -1;
		}

		if (getInGamePlayers(pGameData) == 0)
		{
			_tprintf(TEXT("\n\nGame Over!! No players in the game!\n\n"));
			break;
		}

		if (pGameData->lives == 0)
		{
			_tprintf(TEXT("\n\nGame Over!! Out of lives! Bad luck, try again!\n\n"));
			break;
		}
		else if (pGameVariables->gameConfigs.levels < pGameData->level)
		{
			_tprintf(TEXT("\n\nGame Over!! All levels cleared! Niceee\n\n"));
			break;
		}

		if (pGameData->numBricks == 0)
		{
			advanceLevel(pGameVariables);
			continue;
		}

		ballMovement(pGameVariables);

		sendGameUpdate(*pGameVariables);

		if (WaitForSingleObject(hGameWait, INFINITE) != WAIT_OBJECT_0) {
			_tprintf(TEXT("WaitForSingleObject failed (%d)\n"), GetLastError());
			return -1;
		}
	}
	//Game is over
	gameOver(pGameData);

	saveGameScoresAndOrderTop10(pGameVariables);

	pGameData->gameStatus = GAME_OVER;
	sendGameUpdate(*pGameVariables);

	if (pGameData->numBonus == 0)
	{
		SetEvent(hBonusEvent);
	}

	WaitForSingleObject(hBonusThread, INFINITE);
	
	CloseHandle(hGameWait);

	return 1;
}

DWORD WINAPI BonusLogic(LPVOID lpParam)
{
	GameVariables* pGameVariables = (GameVariables*)lpParam;
	GameData* pGameData = pGameVariables->pGameData;

	HANDLE hBonusWait;
	LARGE_INTEGER timeToWait;

	HANDLE hBonusDurationThread;
	DWORD hBonusDurationThreadID;

	HANDLE hBrickMovementThread;
	DWORD hBrickMovementThreadID;

	timeToWait.QuadPart = _100MILLISECONDS;

	// Create an unnamed waitable timer for the bonus movement.
	hBonusWait = CreateWaitableTimer(NULL, TRUE, NULL);
	if (hBonusWait == NULL)
	{
		_tprintf(TEXT("CreateWaitableTimer failed (%d)\n"), GetLastError());
		return -1;
	}

	//Thread to handle the bonus logic
	hBonusDurationThread = CreateThread(
		NULL,						// default security attributes
		0,							// use default stack size  
		BonusDuration,				// thread function name
		(LPVOID)pGameVariables,		// argument to thread function 
		0,							// use default creation flags 
		&hBonusDurationThreadID);	// returns the thread identifier


	if (hBonusDurationThread == NULL)
	{
		_tprintf(TEXT("\nError creating bonus duration thread!\n\n"));
		ExitProcess(3);
	}

	//Thread to handle the bricks movement logic
	hBrickMovementThread = CreateThread(
		NULL,						// default security attributes
		0,							// use default stack size  
		BricksMovementLogic,		// thread function name
		(LPVOID)pGameVariables,		// argument to thread function 
		0,							// use default creation flags 
		&hBrickMovementThreadID);	// returns the thread identifier 

	if (hBrickMovementThread == NULL)
	{
		_tprintf(TEXT("\nError creating brick movement thread!\n\n"));
		ExitProcess(3);
	}

	while (pGameData->gameStatus != GAME_OVER)
	{
		if (pGameData->numBonus == 0)
		{
			WaitForSingleObject(hBonusEvent, INFINITE);
		}

		if (!SetWaitableTimer(hBonusWait, &timeToWait, 0, NULL, NULL, 0))
		{
			_tprintf(TEXT("SetWaitableTimer failed (%d)\n"), GetLastError());
			return -1;
		}

		for (int i = 0; i < pGameVariables->gameConfigs.maxBonus; i++)
		{
			if (pGameData->bonus[i].status == BONUS_IN_PLAY)
			{
				pGameData->bonus[i].position.y += pGameVariables->gameConfigs.movementSpeed;

				int yStart = pGameData->bonus[i].position.y;
				int yEnd = pGameData->bonus[i].position.y + GAME_BONUS_HEIGHT;
				int status;
				//Check if bonus is in the player barrier area
				if ((yEnd >= GAME_BARRIER_Y && yEnd <= DIM_Y_FRAME) || (yStart >= GAME_BARRIER_Y && yStart <= DIM_Y_FRAME))
				{
					int x = pGameData->bonus[i].position.x + GAME_BONUS_WIDTH / 2;
					//Check if bonus got caught
					for (int j = 0; j < pGameData->numPlayers; j++)
					{
						//Checks if bonus hit a barrier
						int barrierDimension = pGameData->barrierDimensions * pGameData->barrier[j].sizeRatio;
						if (x >= pGameData->barrier[j].position.x && x <= pGameData->barrier[j].position.x + barrierDimension)
						{
							switch (pGameData->bonus[i].type)
							{
								case BONUS_SPEED_UP:
									if (ballBonusIncrease(pGameData) == TRUE)
									{
										status = BONUS_CAUGHT;
										SetEvent(hBonusTimeoutEvent[i]);
									} else
									{
										status = BONUS_INACTIVE;
										pGameData->numBonus--;
									}
									break;
								case BONUS_SLOW_DOWN:
									if (ballBonusDecrease(pGameData) == TRUE)
									{
										status = BONUS_CAUGHT;
										SetEvent(hBonusTimeoutEvent[i]);
									} else
									{
										status = BONUS_INACTIVE;
										pGameData->numBonus--;
									}
									break;
								case BONUS_EXTRA_LIFE:
									status = BONUS_INACTIVE;
									pGameData->numBonus--;
									if(pGameData->lives < pGameVariables->gameConfigs.initialLives)
									{
										pGameData->lives++;
									}
									break;
								case BONUS_TRIPLE_BALL:
									tripleBallBonus(pGameData);
									status = BONUS_INACTIVE;
									pGameData->numBonus--;
									break;
							}
							//Change bonus status
							WaitForSingleObject(hBonusMutex, INFINITE);
							pGameData->bonus[i].status = status;
							ReleaseMutex(hBonusMutex);
							OutputDebugString(TEXT("\nPlayer catched a bonus!!\n"));
							break;
						}
					}
				}
				else if (yStart >= DIM_Y_FRAME)
				{
					WaitForSingleObject(hBonusMutex, INFINITE);
					//Bonus is out of the screen
					pGameData->bonus[i].status = BONUS_INACTIVE;
					pGameData->numBonus--;
					ReleaseMutex(hBonusMutex);
				}
			}
		}

		if (WaitForSingleObject(hBonusWait, INFINITE) != WAIT_OBJECT_0) {
			_tprintf(TEXT("WaitForSingleObject failed (%d)\n"), GetLastError());
			return -1;
		}
	}

	SetEvent(hBonusTimeoutEvent[0]);
	WaitForSingleObject(hBonusDurationThread, INFINITE);

	WaitForSingleObject(hBrickMovementThread, INFINITE);

	CloseHandle(hBonusWait);

	return 1;
}

DWORD WINAPI BonusDuration(LPVOID lpParam)
{
	GameVariables* pGameVariables = (GameVariables*)lpParam;
	GameData* pGameData = pGameVariables->pGameData;

	BonusTimerVariables bonusTimerVariables[MAX_BONUS];

	HANDLE hBonusEffectTimer[MAX_BONUS];
	LARGE_INTEGER bonusEffectTime;

	int maxBonus = pGameVariables->gameConfigs.maxBonus;

	bonusEffectTime.QuadPart = _1SECOND * pGameVariables->gameConfigs.bonusDuration;

	for (int i = 0; i < maxBonus; i++)
	{
		// Create unnamed auto-reset waitable timer to control the bonus duration;
		hBonusEffectTimer[i] = CreateWaitableTimer(NULL, FALSE, NULL);
		//Create event
		hBonusTimeoutEvent[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	DWORD dwWait, i;

	while (pGameData->gameStatus != GAME_OVER)
	{
		dwWait = WaitForMultipleObjectsEx(maxBonus, hBonusTimeoutEvent, FALSE, INFINITE, TRUE);

		//The wait was ended by one or more user-mode asynchronous procedure calls (APC) queued to the thread
		if (dwWait == WAIT_IO_COMPLETION)
		{
			continue;
		}

		// determines which timer is to be set
		i = dwWait - WAIT_OBJECT_0;   

		if (i < 0 || i > (maxBonus - 1))
		{
			continue;
		}

		bonusTimerVariables[i].pGameVariables = pGameVariables;
		bonusTimerVariables[i].bonusIndex = i;

		SetWaitableTimer(
			hBonusEffectTimer[i],	// Handle to the timer object
			&bonusEffectTime,		// When timer will become signaled
			0,						// Non-Periodic timer
			BonusEffectAPCProc,		// Completion routine
			&bonusTimerVariables[i],	// Argument to the completion routine
			FALSE);					// Do not restore a suspended system

	}

	for (int i = 0; i < maxBonus; i++)
	{
		CloseHandle(hBonusEffectTimer[i]);
		CloseHandle(hBonusTimeoutEvent[i]);
	}

	return 1;
}

VOID CALLBACK BonusEffectAPCProc(
	LPVOID lpArg,               // Data value
	DWORD dwTimerLowValue,      // Timer low value
	DWORD dwTimerHighValue)		// Timer high value
{
	// Formal parameters not used.
	UNREFERENCED_PARAMETER(dwTimerLowValue);
	UNREFERENCED_PARAMETER(dwTimerHighValue);

	BonusTimerVariables* pBonusTimerVariables = (BonusTimerVariables*)lpArg;

	GameData* pGameData = pBonusTimerVariables->pGameVariables->pGameData;

	if (pGameData->bonus[pBonusTimerVariables->bonusIndex].status == BONUS_CAUGHT)
	{
		switch (pGameData->bonus[pBonusTimerVariables->bonusIndex].type)
		{
		case BONUS_SPEED_UP:
			//Reduce ball speed according to the value it was increased 
			ballBonusDecrease(pGameData);
			break;
		case BONUS_SLOW_DOWN:
			//Raise ball speed according to the value it was decreased 
			ballBonusIncrease(pGameData);
			break;
		default:
			return;
		}

		WaitForSingleObject(hBonusMutex, INFINITE);
		pGameData->bonus[pBonusTimerVariables->bonusIndex].status = BONUS_INACTIVE;
		pGameData->numBonus--;
		ReleaseMutex(hBonusMutex);

		OutputDebugString(TEXT("\nBonus timed out:\n\n"));
		MessageBeep(0);
	} else
	{
		OutputDebugString(TEXT("\nInvalid bonus:\n\n"));
	}
}

DWORD WINAPI BricksMovementLogic(LPVOID lpParam)
{
	GameVariables* pGameVariables = (GameVariables*)lpParam;
	GameData* pGameData = pGameVariables->pGameData;

	HANDLE hBrickMovementWait;
	LARGE_INTEGER timeToWait;

	int movementSpeed = pGameVariables->gameConfigs.movementSpeed;

	timeToWait.QuadPart = _100MILLISECONDS * pGameVariables->gameConfigs.bricksMovementTime;

	// Create an unnamed waitable timer for the bonus movement.
	hBrickMovementWait = CreateWaitableTimer(NULL, TRUE, NULL);

	if (hBrickMovementWait == NULL)
	{
		_tprintf(TEXT("CreateWaitableTimer failed (%d)\n"), GetLastError());
		return -1;
	}

	int index;

	while (pGameData->gameStatus != GAME_OVER)
	{
		if (!SetWaitableTimer(hBrickMovementWait, &timeToWait, 0, NULL, NULL, 0))
		{
			_tprintf(TEXT("SetWaitableTimer failed (%d)\n"), GetLastError());
			return -1;
		}

		//Check bricks direction
		if (pGameData->bricksDirectionX == DIRECTION_X_RIGHT)
		{
			//Bricks are going to the right so check if collision happened
			if (pGameData->brick[MAX_BRICKS_IN_LINE - 1].position.x + BRICKS_WIDTH >= GAME_BORDER_RIGHT)
			{
				pGameData->bricksDirectionX = DIRECTION_X_LEFT;
				movementSpeed *= -1;
			}

		} else if(pGameData->bricksDirectionX == DIRECTION_X_LEFT)
		{
			//Bricks are going to the left so check if collision happened
			if (pGameData->brick[0].position.x <= GAME_BORDER_LEFT)
			{
				pGameData->bricksDirectionX = DIRECTION_X_RIGHT;
				movementSpeed *= -1;
			}
		}

		WaitForSingleObject(hBrickMovementMutex, INFINITE);
		for (int i = 0; i < MAX_BRICK_LINES; i++)
		{
			for (int j = 0; j < MAX_BRICKS_IN_LINE; j++)
			{
				index = j + i * MAX_BRICKS_IN_LINE;
				pGameData->brick[index].position.x += movementSpeed;
			}
		}
		ReleaseMutex(hBrickMovementMutex);

		if (WaitForSingleObject(hBrickMovementWait, INFINITE) != WAIT_OBJECT_0) {
			_tprintf(TEXT("WaitForSingleObject failed (%d)\n"), GetLastError());
			return -1;
		}
	}

	CloseHandle(hBrickMovementWait);

	return 1;
}

BOOL ballBonusIncrease(GameData* pGameData)
{
	BOOL bonusActivated = FALSE;
	WaitForSingleObject(hBallControlMutex, INFINITE);
	for (int i = 0; i < MAX_BALLS; ++i)
	{
		if (pGameData->ball[i].inPlay == TRUE)
		{
			if (pGameData->ball[i].velocityRatio < SPEED_UP_ACTIVATE_VALUE)
			{
				pGameData->ball[i].velocityRatio += SPEED_UP_INCREASE;
				bonusActivated = TRUE;
			}
		}
	}
	ReleaseMutex(hBallControlMutex);
	return bonusActivated;
}

BOOL ballBonusDecrease(GameData* pGameData)
{
	BOOL bonusActivated = FALSE;
	WaitForSingleObject(hBallControlMutex, INFINITE);
	for (int i = 0; i < MAX_BALLS; ++i)
	{
		if (pGameData->ball[i].inPlay == TRUE)
		{
			if (pGameData->ball[i].velocityRatio > SLOW_DOWN_ACTIVATE_VALUE)
			{
				pGameData->ball[i].velocityRatio -= SLOW_DOWN_DECREASE;
				bonusActivated = TRUE;
			}
		}
	}
	ReleaseMutex(hBallControlMutex);
	return bonusActivated;
}

void tripleBallBonus(GameData* pGameData)
{
	WaitForSingleObject(hBallControlMutex, INFINITE);
	for (int i = 0; i < MAX_BALLS; ++i)
	{
		if (pGameData->ball[i].inPlay == FALSE)
		{
			resetBall(&pGameData->ball[i]);
			pGameData->ball[i].inPlay = TRUE;
			pGameData->numBalls++;
			randomizeBallPosition(&pGameData->ball[i]);
		}
	}
	ReleaseMutex(hBallControlMutex);
}

void randomizeBallPosition(Ball* ball)
{
	rand_s(&random);
	ball->position.x = random % GAME_BOARD_WIDTH + GAME_BORDER_LEFT;

	rand_s(&random);
	ball->position.y = random % GAME_BALL_RANDOM_AREA_Y + GAME_BRICKS_BOTTOM;
}

void initializeGame(GameVariables* pGameVariables)
{
	GameData* pGameData = pGameVariables->pGameData;

	pGameData->gameStatus = GAME_ACTIVE;
	pGameData->level = 1;
	pGameData->lives = pGameVariables->gameConfigs.initialLives;

	pGameData->numBalls = 1;
	for (int i = 0; i < MAX_BALLS; i++)
	{
		resetBall(&pGameData->ball[i]);
	}
	pGameData->ball[0].inPlay = TRUE;

	pGameData->bricksDirectionX = DIRECTION_X_RIGHT;
	pGameData->numBricks = 0;
	initializeBricks(pGameVariables);

	//Initialize bonus
	hBonusEvent = CreateEvent(
		NULL,						// default security attributes
		FALSE,						// auto-reset event
		FALSE,						// initial state is nonsignaled
		NULL						// unnamed event
	);

	hBallControlMutex = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initially not owned
		NULL);             // unnamed mutex

	hBonusMutex = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initially not owned
		NULL);             // unnamed mutex

	hBrickMovementMutex = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initially not owned
		NULL);             // unnamed mutex

	pGameData->numBonus = 0;
	for (int i = 0; i < pGameVariables->gameConfigs.maxBonus; i++)
	{
		pGameData->bonus[i].status = BONUS_INACTIVE;
	}
}

void resetBall(Ball* ball)
{
	ball->velocityRatio = 100;
	rand_s(&random);
	ball->directionY = random % 2 == 1
		? 1
		: -1;
	rand_s(&random);
	ball->directionX = random % 2 == 1
		? 1
		: -1;
	ball->playerIndex = UNDEFINED_ID;
	ball->position.x = GAME_BOARD_WIDTH / 2 + GAME_BORDER_LEFT;
	ball->position.y = GAME_BOARD_HEIGHT / 2 + GAME_BORDER_TOP;
	ball->inPlay = FALSE;
}

void initializeBricks(GameVariables* pGameVariables)
{
	GameData* pGameData = pGameVariables->pGameData;
	int index;
	int maxResistance = pGameData->level > BRICKS_MAX_RESISTANCE
		? BRICKS_MAX_RESISTANCE
		: pGameData->level;
	int probability;

	for (int i = 0; i < MAX_BRICK_LINES; i++)
	{
		for (int j = 0; j < MAX_BRICKS_IN_LINE; j++)
		{
			if (pGameData->numBricks == pGameVariables->gameConfigs.numBricks)
			{
				break;
			}
			pGameData->numBricks++;
			index = j + i * MAX_BRICKS_IN_LINE;
			//Setup brick position
			pGameData->brick[index].position.x = (BRICKS_WIDTH + GAME_BRICKS_MARGIN) * j + GAME_BORDER_LEFT;
			pGameData->brick[index].position.y = BRICKS_HEIGHT * i + GAME_BORDER_TOP;
			pGameData->brick[index].hasBonus = FALSE;
			rand_s(&random);
			pGameData->brick[index].resistance = (random % maxResistance) + 1;

			//Setup bonus
			rand_s(&random);
			probability = (random % 100) + 1;
			if (probability <= pGameVariables->gameConfigs.bonusProbability)
			{
				pGameData->brick[index].hasBonus = TRUE;
				rand_s(&random);
				pGameData->brick[index].bonusType = random % TOTAL_BONUS;
				pGameData->brick[index].resistance = 1;
			}
		}
	}
}

void advanceLevel(GameVariables* pGameVariables)
{
	GameData* pGameData = pGameVariables->pGameData;

	pGameData->level++;

	WaitForSingleObject(hBallControlMutex, INFINITE);
	pGameData->numBalls = 1;
	for (int i = 0; i < MAX_BALLS; i++)
	{
		resetBall(&pGameData->ball[i]);
	}
	pGameData->ball[0].inPlay = TRUE;
	ReleaseMutex(hBallControlMutex);

	pGameData->numBricks = 0;
	initializeBricks(pGameVariables);

	pGameData->numBonus = 0;
	WaitForSingleObject(hBonusMutex, INFINITE);
	for (int i = 0; i < pGameVariables->gameConfigs.maxBonus; i++)
	{
		pGameData->bonus[i].status = BONUS_INACTIVE;
	}
	ReleaseMutex(hBonusMutex);
}

void sendGameUpdate(GameVariables gameVariables)
{
	if (!SetEvent(gameVariables.hGameUpdateEvent))
	{
		_tprintf(TEXT("SetEvent GameUpdate failed (%d)\n"), GetLastError());
		return;
	}
	ResetEvent(gameVariables.hGameUpdateEvent);

	for (int i = 0; i < gameVariables.gameConfigs.maxPlayers; i++)
	{
		if (gameVariables.namedPipesData[i].userID != UNDEFINED_ID)
		{
			sendGameNamedPipe(*gameVariables.pGameData, &gameVariables.namedPipesData[i]);
		}
	}
}

void ballMovement(GameVariables* pGameVariables)
{
	GameData* pGameData = pGameVariables->pGameData;
	WaitForSingleObject(hBallControlMutex, INFINITE);
	for (int i = 0, j = 0; j < pGameData->numBalls; i++)
	{
		if (pGameData->ball[i].inPlay == TRUE)
		{
			//Advance ball according to speed
			int velocity = pGameVariables->gameConfigs.movementSpeed * pGameData->ball[i].velocityRatio / 100;
			pGameData->ball[i].position.x += velocity * pGameData->ball[i].directionX;
			pGameData->ball[i].position.y += velocity * pGameData->ball[i].directionY;
			detectBallCollision(pGameVariables, i);
			_stprintf_s(debugText, MAX, TEXT("\nBall %d position x: %d y: %d\n"), i, pGameData->ball[i].position.x, pGameData->ball[i].position.y);
			OutputDebugString(debugText);
			j++;
		}
	}
	ReleaseMutex(hBallControlMutex);
}

void detectBallCollision(GameVariables* pGameVariables, int index)
{
	GameData* pGameData = pGameVariables->pGameData;
	Ball* ball = &pGameData->ball[index];

	//Game Border Right or Left Collisions
	if (ball->position.x + GAME_BALL_WIDTH >= GAME_BORDER_RIGHT)
	{
		//Right border hit -> Move left instead
		ball->directionX = DIRECTION_X_LEFT;
	}
	else if (ball->position.x <= GAME_BORDER_LEFT)
	{
		//Left border hit -> Move right instead
		ball->directionX = DIRECTION_X_RIGHT;
	}

	//Game Border Top or Bottom Collisions
	if (ball->position.y <= GAME_BORDER_TOP)
	{
		//Top border hit -> Move down instead
		ball->directionY = DIRECTION_Y_DOWN;
	}
	else if (ball->position.y + GAME_BALL_HEIGHT >= DIM_Y_FRAME)
	{
		//Bottom border hit -> Move up instead
		//Resets the ball
		resetBall(ball);

		//If it was the last ball removes a life and puts the ball back in play
		if (pGameData->numBalls == 1)
		{
			pGameData->lives--;
			ball->inPlay = TRUE;

			//Invalidate all bonus that were caught
			WaitForSingleObject(hBonusMutex, INFINITE);
			for(int i = 0; i < MAX_BONUS; i++)
			{
				if (pGameData->bonus[i].status == BONUS_CAUGHT)
				{
					pGameData->bonus[i].status = BONUS_INACTIVE;
					pGameData->numBonus--;
				}
			}
			ReleaseMutex(hBonusMutex);
		} else
		{
			pGameData->numBalls--;
		}

		return;
	}

	int direction;

	//Brick collisions
	//Check if ball is going up or down
	int y = ball->position.y, x = ball->position.x;
	if (ball->directionY == DIRECTION_Y_UP)
	{
		if (ball->directionX == DIRECTION_X_RIGHT)
		{
			//Ball is going up and right
			y += GAME_BALL_HITBOX;
			x += GAME_BALL_WIDTH - GAME_BALL_HITBOX;
			direction = DIRECTION_LEFT_TO_RIGHT;
			ballAndBrickCollision(pGameVariables, index, x, y, direction);
		}
		else if (ball->directionX == DIRECTION_X_LEFT)
		{
			//Ball is going up and left
			y += GAME_BALL_HITBOX;
			x += GAME_BALL_HITBOX;
			direction = DIRECTION_RIGHT_TO_LEFT;
			ballAndBrickCollision(pGameVariables, index, x, y, direction);
		}
	}
	else if (ball->directionY == DIRECTION_Y_DOWN)
	{
		if (ball->directionX == DIRECTION_X_RIGHT)
		{
			//Ball is going down and right
			x += GAME_BALL_WIDTH - GAME_BALL_HITBOX;
			y += GAME_BALL_HEIGHT - GAME_BALL_HITBOX;
			direction = DIRECTION_LEFT_TO_RIGHT;
			ballAndBrickCollision(pGameVariables, index, x, y, DIRECTION_LEFT_TO_RIGHT);
		}
		else if (ball->directionX == DIRECTION_X_LEFT)
		{
			//Ball is going down and left
			y += GAME_BALL_HEIGHT - GAME_BALL_HITBOX;
			direction = DIRECTION_RIGHT_TO_LEFT;
			ballAndBrickCollision(pGameVariables, index, x, y, DIRECTION_RIGHT_TO_LEFT);
		}
	}

	if (y >= GAME_BARRIER_Y)
	{
		ballAndBarrierCollision(pGameVariables, index, x, y, direction);
	}

	//TODO: Check bonus collision 
}

void ballAndBrickCollision(GameVariables* pGameVariables, int index, int x, int y, int directionX)
{
	GameData* pGameData = pGameVariables->pGameData;
	Ball* ball = &pGameData->ball[index];

	if (ball->position.y >= GAME_BORDER_TOP && ball->position.y <= GAME_BRICKS_BOTTOM)
	{
		//Convert ball position to brick index
		int brickIndexY = (y - GAME_BORDER_TOP) / BRICKS_HEIGHT;

		int brickIndex;
		Brick* brick = NULL;

		//Find brickIndexX
		WaitForSingleObject(hBrickMovementMutex, INFINITE);
		for(int i = 0; i < MAX_BRICKS_IN_LINE; i++)
		{
			brickIndex = i + (brickIndexY - 1) * MAX_BRICKS_IN_LINE;
			brick = &pGameData->brick[brickIndex];

			if(x >= brick->position.x && x <= brick->position.x + BRICKS_WIDTH + GAME_BRICKS_MARGIN)
			{
				break;
			}
		}
		ReleaseMutex(hBrickMovementMutex);

		if(brick == NULL)
		{
			OutputDebugString(TEXT("Could not find brick!!"));
			return;
		}

		if (brick->resistance > 0)
		{
			//Hit was from below the brick
			if (y <= brick->position.y + BRICKS_HEIGHT / 2)
			{
				ball->directionY *= -1;
			}

			//Hit was from above the brick
			if (y >= brick->position.y + BRICKS_HEIGHT / 2)
			{
				ball->directionY *= -1;
			}

			//Hit was on left
			if (x <= brick->position.x + GAME_BRICK_HITBOX)
			{
				//Switch direction if the hit was going from left to the right
				if (directionX == DIRECTION_LEFT_TO_RIGHT)
				{
					ball->directionX = DIRECTION_X_LEFT;
				}
			}

			//Hit was on right
			if (x >= brick->position.x + BRICKS_WIDTH / 2 + GAME_BRICK_HITBOX)
			{
				//Switch direction if the hit was from the right too
				if (directionX == DIRECTION_RIGHT_TO_LEFT)
				{
					ball->directionX = DIRECTION_X_RIGHT;
				}
			}

			brick->resistance--;
			pGameData->numBricks--;

			//If a valid player hit the ball increase scoreboard
			if (ball->playerIndex != UNDEFINED_ID)
			{
				pGameData->player[ball->playerIndex].score += 1;
			}

			//Check if brick has bonus
			if (brick->hasBonus == TRUE)
			{
				if (pGameData->numBonus < pGameVariables->gameConfigs.maxBonus)
				{
					if (pGameData->numBonus == 0)
					{
						SetEvent(hBonusEvent);
					}

					for (int i = 0; i < pGameVariables->gameConfigs.maxBonus; i++)
					{
						WaitForSingleObject(hBonusMutex, INFINITE);
						if (pGameData->bonus[i].status == BONUS_INACTIVE)
						{
							pGameData->bonus[i].status = BONUS_IN_PLAY;
							pGameData->bonus[i].position.x = brick->position.x;
							pGameData->bonus[i].position.y = brick->position.y;
							pGameData->bonus[i].type = brick->bonusType;
							pGameData->numBonus++;
							ReleaseMutex(hBonusMutex);
							return;
						}
						ReleaseMutex(hBonusMutex);
					}
				}
			}
		}
	}
}

void ballAndBarrierCollision(GameVariables* pGameVariables, int index, int x, int y, int directionX)
{
	GameData* pGameData = pGameVariables->pGameData;

	for (int i = 0; i < pGameData->numPlayers; i++)
	{
		if (pGameData->player[i].inGame == TRUE)
		{
			//Checks if ball hit a barrier
			int barrierDimension = pGameData->barrierDimensions * pGameData->barrier[i].sizeRatio;
			if (x >= pGameData->barrier[i].position.x && x <= pGameData->barrier[i].position.x + barrierDimension) {
				int hitbox = pGameData->barrierDimensions / 8;
				//Check which side of the barrier was hit
				WaitForSingleObject(pGameVariables->hGameLogicMutex, INFINITE);
				if (x <= pGameData->barrier[i].position.x + hitbox)
				{
					//Left side was hit
					if (directionX == DIRECTION_LEFT_TO_RIGHT)
					{
						pGameData->ball[index].directionX = DIRECTION_X_LEFT;
					}
				}
				else if (x >= pGameData->barrier[i].position.x + pGameData->barrierDimensions - hitbox)
				{
					//Right side was hit
					if (directionX == DIRECTION_RIGHT_TO_LEFT)
					{
						pGameData->ball[index].directionX = DIRECTION_X_RIGHT;
					}
				}
				ReleaseMutex(pGameVariables->hGameLogicMutex);
				pGameData->ball[index].directionY = DIRECTION_Y_UP;
				pGameData->ball[index].playerIndex = i;
				return;
			}
		}
	}
}

void assignUsersToGame(GameData* pGameData, Player* users, int currentUsers)
{
	pGameData->numPlayers = currentUsers;

	int maxDimension = BARRIER_AREA / pGameData->numPlayers;
	pGameData->barrierDimensions = maxDimension - maxDimension * BARRIER_MARGIN_PERCENTAGE;
	int barrierStartPosition = GAME_BORDER_LEFT + ((maxDimension + BARRIER_REMAINDER) / 2) - pGameData->barrierDimensions / 2;

	for (int i = 0; i < currentUsers; i++)
	{
		//Setup Player
		_tcscpy_s(pGameData->player[i].username, TAM, users[i].username);
		pGameData->player[i].id = users[i].id;
		pGameData->player[i].inGame = TRUE;
		pGameData->player[i].score = 0;

		//Setup Barrier
		pGameData->barrier[i].playerID = pGameData->player[i].id;
		pGameData->barrier[i].sizeRatio = 1;
		pGameData->barrier[i].position.x = barrierStartPosition;
		barrierStartPosition += maxDimension;
		pGameData->barrier[i].position.y = GAME_BARRIER_Y;
	}
}

void gameOver(GameData* pGameData)
{
	//Update user status from game
	for (int i = 0; i < pGameData->numPlayers; ++i)
	{
		pGameData->player[i].inGame = FALSE;
	}
}

int getPlayerToTheRight(GameData gameData, int userPosition)
{
	for (int i = userPosition; i < gameData.numPlayers - 1; i++)
	{
		if (gameData.player[i + 1].inGame == TRUE)
		{
			return i + 1;
		}
	}
	return -1;
}

int getPlayerToTheLeft(GameData gameData, int userPosition)
{
	for (int i = userPosition; i > 0; i--)
	{
		if (gameData.player[i - 1].inGame == TRUE)
		{
			return i - 1;
		}
	}
	return -1;
}

void saveGameScoresAndOrderTop10(GameVariables* pGameVariables)
{
	GameData* pGameData = pGameVariables->pGameData;
	TopPlayer* pTopPlayers = pGameVariables->topPlayers;
	DWORD* top10PlayerCount = pGameVariables->top10PlayerCount;
	TopPlayer topPlayerTemp;
	int counter = 0;

	for (int i = 0; i < MAX_TOP_PLAYERS; i++)
	{
		for (int j = 0; j < pGameData->numPlayers; j++)
		{
			if (pTopPlayers[i].topScore < pGameData->player[j].score)
			{
				//Copy value to temporary
				topPlayerTemp.topScore = pTopPlayers[i].topScore;
				_tcscpy_s(topPlayerTemp.username, TAM, pTopPlayers[i].username);
				//Replace top10 value with player value
				pTopPlayers[i].topScore = pGameData->player[j].score;
				_tcscpy_s(pTopPlayers[i].username, TAM, pGameData->player[j].username);
				//Replace player value with top player and continue ordering
				pGameData->player[j].score = topPlayerTemp.topScore;
				_tcscpy_s(pGameData->player[j].username, TAM, topPlayerTemp.username);
			}
		}

		if (pTopPlayers[i].topScore != 0)
		{
			counter++;
		}
	}

	 *top10PlayerCount = counter;

	convertTopPlayersToString(pTopPlayers, pGameVariables->top10Value, top10PlayerCount);

	//Create value "TOP10" = "top10Value"
	RegSetValueEx(pGameVariables->hResgistryTop10Key, TOP10_REGISTRY_VALUE, 0, REG_SZ, (LPBYTE)pGameVariables->top10Value, _tcslen(pGameVariables->top10Value) * sizeof(TCHAR));

	//Create value "TOP10PlayerCount" = top10PlayerCount
	RegSetValueEx(pGameVariables->hResgistryTop10Key, TOP10_PLAYER_COUNT, 0, REG_DWORD, (LPBYTE)*top10PlayerCount, sizeof(DWORD));
}

int getInGamePlayers(GameData* pGameData)
{
	int counter = 0;

	for (int i = 0; i < pGameData->numPlayers; ++i)
	{
		if (pGameData->player[i].inGame == TRUE)
		{
			counter++;
		}
	}

	return counter;
}