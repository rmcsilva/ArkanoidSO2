#pragma once

#include "resourceConstants.h"
#include "stdafx.h"

//Initial ID
#define UNDEFINED_ID -1

//Server Response Messages
#define REQUEST_ACCEPTED 0
#define REQUEST_DENIED 1

//Game Messages
#define LOGIN_REQUEST 2
#define MOVE_RIGHT 3
#define MOVE_LEFT 4
#define TOP10 5
#define LEAVE_GAME 6
#define LOGOUT 7

typedef struct ClientMessage {
	int type;
	TCHAR username[TAM];
	int id;
}ClientMessage;

typedef struct ServerMessage {
	int type;
	int id;
	TCHAR username[TAM];
	TCHAR content[TOP10_SIZE];
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