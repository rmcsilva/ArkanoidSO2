#pragma once
//Game dimensions
#define DIM_X_FRAME 800
#define DIM_Y_FRAME 600

#define GAME_BORDER_TOP 122
#define GAME_BORDER_LEFT 49
#define GAME_BORDER_RIGHT 747
#define GAME_BOARD_WIDTH (GAME_BORDER_RIGHT - GAME_BORDER_LEFT)
#define GAME_BOARD_HEIGHT (DIM_Y_FRAME - GAME_BORDER_TOP)

#define GAME_LIVES_START_X 75
#define GAME_LIVES_START_Y 71
#define GAME_LIVES_MARGIN 10

#define GAME_SCORE_X 620

#define GAME_BARRIER_Y 560

//Game constants 
#define MAX_PLAYERS 20
#define MAX_TOP_PLAYERS 10
#define MAX_BRICKS_IN_LINE 14
#define MAX_BRICK_LINES 7
#define MAX_BRICKS (MAX_BRICKS_IN_LINE * MAX_BRICK_LINES)
#define BRICKS_WIDTH 48
#define BRICKS_HEIGHT 24
#define BRICKS_MARGIN ((GAME_BOARD_WIDTH - (MAX_BRICKS_IN_LINE * BRICKS_WIDTH)) / MAX_BRICKS_IN_LINE)
#define BRICKS_MAX_RESISTANCE 4
#define MAX_BONUS 5
#define MAX_BALLS 3

//Bonus types
#define TOTAL_BONUS 6
#define BONUS_SPEED_UP 0
#define BONUS_SLOW_DOWN 1
#define BONUS_EXTRA_LIFE 2
#define BONUS_TRIPLE_BALL 3
#define BONUS_BIGGER_BARRIER 4
#define BONUS_SMALLER_BARRIER 5

//Game Status
#define IN_LOBBY 0
#define IN_GAME 1
#define GAME_OVER 2