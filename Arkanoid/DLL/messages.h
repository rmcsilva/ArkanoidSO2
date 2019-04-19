#pragma once

#include "resourceConstants.h"
#include "stdafx.h"

//Login Messages
#define LOGIN_REQUEST 0
#define LOGIN_ACCEPTED 1
#define LOGIN_DENIED 2

//Game Messages
#define MOVE_RIGHT 3
#define MOVE_LEFT 4
#define TOP10 5
#define LEAVE_GAME 6

typedef struct ClientMessage {
	int type;
	TCHAR username[TAM];
	int id;
}ClientMessage;

typedef struct ServerMessage {
	int type;
	TCHAR username[TAM];
	TCHAR content[MAX];
}ServerMessage;

typedef struct ClientMessageControl {
	int clientInput;
	int serverOutput;
	ClientMessage clientMessageBuffer[BUFFER_SIZE];
}ClientMessageControl;

typedef struct ServerMessageControl {
	int serverInput;
	int clientOutput;
	int numUsers;
	int counter;
	ServerMessage serverMessageBuffer[BUFFER_SIZE];
}ServerMessageControl;