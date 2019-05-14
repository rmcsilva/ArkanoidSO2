#pragma once
#include "gameStructs.h"

//Named Pipe State
#define CONNECTING_STATE 0 
#define READING_REQUEST_STATE 1 
#define WRITING_RESPONSE_STATE 2 
#define WRITING_GAME_STATE 3

//Named Pipe Handler Structure
typedef struct PipeData {
	HANDLE hClientRequestsPipe;
	HANDLE hServerResponsesPipe;
	HANDLE hGamePipe;
	OVERLAPPED overlapped;
	int userID;
	DWORD dwState;
	BOOL fPendingIO;
}PipeData;

void sendGameNamedPipe(GameData gameData, PipeData* pipeData);