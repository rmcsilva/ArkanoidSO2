#pragma once

#include "resourceConstants.h"

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
	char name[TAM];
	int score;
	int id;
	int inGame;
}Player;

typedef struct GameData {
	int gameStatus;
	int level;
	int numPlayers;
	Player *player;
	Barrier *barrier;
	int numBricks;
	Brick *brick;
	int numGifts;
	Gift *gift;
	Ball ball;
}GameData;