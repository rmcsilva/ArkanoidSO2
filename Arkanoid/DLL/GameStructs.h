#pragma once
#include "stdafx.h"
#include "resourceConstants.h"
#include "gameConstants.h"

#define GAME_LOBBY 0
#define GAME_ACTIVE 1
#define GAME_OVER  2

typedef struct Position {
	int x;
	int y;
}Position;

typedef struct Ball {
	int directionX;
	int directionY;
	int velocityRatio;
	Position position;
	int playerIndex;
	BOOL inPlay;
}Ball;

typedef struct Barrier {
	Position position;
	int sizeRatio;
	int playerID;
}Barrier;

typedef struct Brick {
	Position position;
	int resistance;
	BOOL hasBonus;
	int bonusType;
}Brick;

typedef struct Bonus {
	Position position;
	int type;
}Bonus;

typedef struct Player {
	TCHAR username[TAM];
	int id;
	int score;
	BOOL inGame;
}Player;

typedef struct GameData {
	int gameStatus;
	int level;
	int lives;
	int barrierDimensions;
	int numPlayers;
	Player player[MAX_PLAYERS];
	Barrier barrier[MAX_PLAYERS];
	int numBricks;
	Brick brick[MAX_BRICKS];
	int numBonus;
	Bonus bonus[MAX_BONUS];
	int numBalls;
	Ball ball[MAX_BALLS];
}GameData;

typedef struct TopPlayer {
	TCHAR username[TAM];
	int topScore;
}TopPlayer;