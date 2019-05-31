#define _CRT_RAND_S

#define _10MILLISECONDS -100000LL
#define _100MILLISECONDS -100000LL
#include "stdafx.h"
#include "gameLogic.h"
#include "gameStructs.h"
#include "messages.h"
#include <stdlib.h>

TCHAR debugText[MAX];
unsigned int random;

HANDLE hBonusEvent;

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

	while (pGameData->gameStatus != GAME_OVER)
	{
		// Set a timer to wait.
		if (!SetWaitableTimer(hGameWait, &timeToWait, 0, NULL, NULL, 0))
		{
			_tprintf(TEXT("SetWaitableTimer failed (%d)\n"), GetLastError());
			return -1;
		}
		
		if(pGameData->lives == 0)
		{
			_tprintf(TEXT("\nGame Over!! Out of lives! Bad luck, try again!\n\n"));
			break;
		} else if (pGameVariables->gameConfigs.levels == pGameData->level)
		{
			_tprintf(TEXT("\nGame Over!! All levels cleared! Niceee\n\n"));
			break;
		}

		if(pGameData->numBricks == 0)
		{
			//TODO: Advance level, reset balls, reset bricks
		}

		ballMovement(pGameVariables);

		sendGameUpdate(*pGameVariables);

		if (WaitForSingleObject(hGameWait, INFINITE) != WAIT_OBJECT_0) {
			_tprintf(TEXT("WaitForSingleObject failed (%d)\n"), GetLastError());
			return -1;
		}
	}

	pGameData->gameStatus = GAME_OVER;
	sendGameUpdate(*pGameVariables);

	if (pGameData->numBonus == 0)
	{
		SetEvent(hBonusEvent);
	}

	WaitForSingleObject(hBonusThread, INFINITE);

	return 1;
}

DWORD WINAPI BonusLogic(LPVOID lpParam)
{
	GameVariables* pGameVariables = (GameVariables*)lpParam;
	GameData* pGameData = pGameVariables->pGameData;

	HANDLE hBonusWait;
	LARGE_INTEGER timeToWait;

	timeToWait.QuadPart = _100MILLISECONDS;

	// Create an unnamed waitable timer.
	hBonusWait = CreateWaitableTimer(NULL, TRUE, NULL);
	if (hBonusWait == NULL)
	{
		_tprintf(TEXT("CreateWaitableTimer failed (%d)\n"), GetLastError());
		return -1;
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

		for (int i = 0, j = 0; i < pGameVariables->gameConfigs.maxBonus; i++)
		{
			if (pGameData->bonus[i].isActive == TRUE)
			{
				pGameData->bonus[i].position.y += pGameVariables->gameConfigs.movementSpeed;

				int yStart = pGameData->bonus[i].position.y;
				int yEnd = pGameData->bonus[i].position.y + GAME_BONUS_HEIGHT;
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
							pGameData->bonus[i].isActive = FALSE;
							pGameData->numBonus--;
							switch (pGameData->bonus->type)
							{
								case BONUS_SPEED_UP:
									break;
								case BONUS_SLOW_DOWN:
									break;
								case BONUS_EXTRA_LIFE:
									pGameData->lives++;
									break;
								case BONUS_TRIPLE_BALL:
									break;
							}
							OutputDebugString(TEXT("\nPlayer catched a bonus!!\n"));
							break;
						}
					}
				} else if(yStart >= DIM_Y_FRAME)
				{
					//Bonus is out of the screen
					pGameData->bonus[i].isActive = FALSE;
					pGameData->numBonus--;
				}
			}
		}

		if (WaitForSingleObject(hBonusWait, INFINITE) != WAIT_OBJECT_0) {
			_tprintf(TEXT("WaitForSingleObject failed (%d)\n"), GetLastError());
			return -1;
		}
	}

	return 1;
}

void initializeGame(GameVariables* pGameVariables)
{
	GameData* pGameData = pGameVariables->pGameData;

	pGameData->gameStatus = GAME_ACTIVE;
	pGameData->numBalls = 1;
	pGameData->lives = pGameVariables->gameConfigs.initialLives;
	pGameData->numBonus = 0;
	pGameData->numBricks = 0;

	for (int i = 0; i < MAX_BALLS; i++)
	{
		resetBall(&pGameData->ball[i]);
	}

	pGameData->ball[0].inPlay = TRUE;

	initializeBricks(pGameVariables);

	//Initialize bonus
	hBonusEvent = CreateEvent(
		NULL,						// default security attributes
		FALSE,						// auto-reset event
		FALSE,						// initial state is nonsignaled
		NULL						// unnamed event
	);

	pGameData->numBonus = 0;
	for(int i = 0; i < pGameVariables->gameConfigs.maxBonus; i++)
	{
		pGameData->bonus[i].isActive = FALSE;
	}
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

	for(int i = 0; i < MAX_BRICK_LINES; i++)
	{
		for (int j = 0; j < MAX_BRICKS_IN_LINE; j++)
		{
			if(pGameData->numBricks == pGameVariables->gameConfigs.numBricks)
			{
				break;
			}
			pGameData->numBricks++;
			index = j + i * MAX_BRICKS_IN_LINE;
			//Setup brick position
			pGameData->brick[index].position.x = (BRICKS_WIDTH + BRICKS_MARGIN) * j + GAME_BORDER_LEFT;
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
				pGameData->brick[index].resistance = 1;
			}
		}
	}
}

void sendGameUpdate(GameVariables gameVariables)
{
	if (!SetEvent(gameVariables.hGameUpdateEvent))
	{
		_tprintf(TEXT("SetEvent GameUpdate failed (%d)\n"), GetLastError());
		return;
	}
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

	for(int i=0, j=0; j < pGameData->numBalls; i++)
	{
		if(pGameData->ball[i].inPlay == TRUE)
		{
			//Advance ball according to speed
			int velocity = pGameVariables->gameConfigs.movementSpeed * pGameData->ball[i].velocityRatio;
			pGameData->ball[i].position.x += velocity * pGameData->ball[i].directionX;
			pGameData->ball[i].position.y += velocity * pGameData->ball[i].directionY;
			detectBallCollision(pGameVariables, i);
			_stprintf_s(debugText, MAX, TEXT("\nBall %d position x: %d y: %d\n"), i, pGameData->ball[i].position.x, pGameData->ball[i].position.y);
			OutputDebugString(debugText);
			j++;
		}
	}
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
	} else if (ball->position.x <= GAME_BORDER_LEFT)
	{
		//Left border hit -> Move right instead
		ball->directionX = DIRECTION_X_RIGHT;
	} 
	
	//Game Border Top or Bottom Collisions
	if (ball->position.y <= GAME_BORDER_TOP)
	{
		//Top border hit -> Move down instead
		ball->directionY = DIRECTION_Y_DOWN;
	} else if (ball->position.y + GAME_BALL_HEIGHT >= DIM_Y_FRAME)
	{
		//Bottom border hit -> Move up instead
		//Resets the ball
		resetBall(ball);

		//If it was the last ball removes a life and puts the ball back in play
		if (pGameData->numBalls == 1)
		{
			pGameData->lives--;
			ball->inPlay = TRUE;
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
	if(ball->directionY == DIRECTION_Y_UP)
	{
		if(ball->directionX == DIRECTION_X_RIGHT)
		{
			//Ball is going up and right
			y += GAME_BALL_HITBOX;
			x += GAME_BALL_WIDTH - GAME_BALL_HITBOX;
			direction = DIRECTION_LEFT_TO_RIGHT;
			ballAndBrickCollision(pGameVariables, index, x, y, direction);
		} else if(ball->directionX == DIRECTION_X_LEFT)
		{
			//Ball is going up and left
			y += GAME_BALL_HITBOX;
			x += GAME_BALL_HITBOX;
			direction = DIRECTION_RIGHT_TO_LEFT;
			ballAndBrickCollision(pGameVariables, index, x, y, direction);
		}
	} else if(ball->directionY == DIRECTION_Y_DOWN)
	{	
		if (ball->directionX == DIRECTION_X_RIGHT)
		{
			//Ball is going down and right
			x += GAME_BALL_WIDTH - GAME_BALL_HITBOX;
			y += GAME_BALL_HEIGHT - GAME_BALL_HITBOX;
			direction = DIRECTION_LEFT_TO_RIGHT;
			ballAndBrickCollision(pGameVariables, index, x, y, DIRECTION_LEFT_TO_RIGHT);
		} else if (ball->directionX == DIRECTION_X_LEFT)
		{
			//Ball is going down and left
			y += GAME_BALL_HEIGHT - GAME_BALL_HITBOX;
			direction = DIRECTION_RIGHT_TO_LEFT;
			ballAndBrickCollision(pGameVariables, index, x, y, DIRECTION_RIGHT_TO_LEFT);
		}
	}

	if(y >= GAME_BARRIER_Y)
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
		int brickIndexX = (x - GAME_BORDER_LEFT) / (BRICKS_WIDTH + BRICKS_MARGIN);
		int brickIndexY = (y - GAME_BORDER_TOP) / BRICKS_HEIGHT;

		int brickIndex = brickIndexX + (brickIndexY - 1) * MAX_BRICKS_IN_LINE;

		Brick* brick = &pGameData->brick[brickIndex];

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
				if(directionX == DIRECTION_LEFT_TO_RIGHT)
				{
					ball->directionX = DIRECTION_X_LEFT;
				}
			}

			//Hit was on right
			if (x >= brick->position.x + BRICKS_WIDTH / 2 + GAME_BRICK_HITBOX)
			{
				//Switch direction if the hit was from the right too
				if(directionX == DIRECTION_RIGHT_TO_LEFT)
				{
					ball->directionX = DIRECTION_X_RIGHT;
				}
			}

			brick->resistance--;

			//If a valid player hit the ball increase scoreboard
			if(ball->playerIndex != UNDEFINED_ID)
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

					int bonuxIndex;
					for (int i = 0; i < pGameVariables->gameConfigs.maxBonus; i++)
					{
						if (pGameData->bonus[i].isActive == FALSE)
						{
							bonuxIndex = i;
							pGameData->bonus[i].isActive = TRUE;
							pGameData->bonus[i].position.x = brick->position.x;
							pGameData->bonus[i].position.y = brick->position.y;
							pGameData->bonus[i].type = brick->bonusType;
							pGameData->numBonus++;
							return;
						}	
					}
				}
			}
		}
	} 
}

void ballAndBarrierCollision(GameVariables* pGameVariables, int index,int x, int y, int directionX)
{
	GameData* pGameData = pGameVariables->pGameData;

	for(int i = 0; i < pGameData->numPlayers; i++)
	{
		//Checks if ball hit a barrier
		int barrierDimension = pGameData->barrierDimensions * pGameData->barrier[i].sizeRatio;
		if(x >= pGameData->barrier[i].position.x && x <= pGameData->barrier[i].position.x + barrierDimension)
		{
			int hitbox = pGameData->barrierDimensions / 8;
			//Check which side of the barrier was hit
			WaitForSingleObject(pGameVariables->hGameLogicMutex, INFINITE);
			if(x <= pGameData->barrier[i].position.x + hitbox)
			{
				//Left side was hit
				if (directionX == DIRECTION_LEFT_TO_RIGHT)
				{
					pGameData->ball[index].directionX = DIRECTION_X_LEFT;
				}
			}else if(x >= pGameData->barrier[i].position.x + pGameData->barrierDimensions - hitbox)
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

void assignUsersToGame(GameData* pGameData, Player* users, int currentUsers)
{
	pGameData->numPlayers = currentUsers;

	int maxDimension = BARRIER_AREA / pGameData->numPlayers;
	pGameData->barrierDimensions =  maxDimension - maxDimension * BARRIER_MARGIN_PERCENTAGE;
	int barrierStartPosition = GAME_BORDER_LEFT + ((maxDimension + BARRIER_REMAINDER) / 2) - pGameData->barrierDimensions / 2;

	for(int i = 0; i < currentUsers; i++)
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

int getPlayerToTheRight(GameData gameData, int userPosition)
{
	for(int i = userPosition; i < gameData.numPlayers - 1; i++)
	{
		if(gameData.player[i + 1].inGame == TRUE)
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