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
	int playerID;
}Ball;

typedef struct Barrier {
	Position position;
	int sizeRatio;
	int playerID;
}Barrier;

typedef struct Brick {
	Position position;
	int resistance;
	int hasGift;
}Brick;

typedef struct Gift {
	Position position;
	int type;
}Gift;

typedef struct Player {
	TCHAR username[TAM];
	int score;
	int id;
	int lives;
	int inGame;
}Player;

typedef struct GameData {
	int gameStatus;
	int level;
	int barrierDimensions;
	int numPlayers;
	Player player[MAX_PLAYERS];
	Barrier barrier[MAX_PLAYERS];
	int numBricks;
	Brick brick[MAX_BRICKS];
	int numGifts;
	Gift gift[MAX_GIFTS];
	int numBalls;
	Ball ball[MAX_BALLS];
}GameData;

typedef struct TopPlayer {
	TCHAR username[TAM];
	int topScore;
}TopPlayer;