#pragma once
#include "gameStructs.h"
#include "messages.h"

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
	OVERLAPPED overlappedRequests;
	OVERLAPPED overlappedGame;
	int userID;
	DWORD dwStateRequests;
	DWORD dwStateGame;
	BOOL fPendingIORequests;
	BOOL fPendingIOGame;
}PipeData;

void sendGameNamedPipe(GameData gameData, PipeData* pipeData);
void receiveRequestNamedPipe(ClientMessage clientRequest, PipeData* namedPipeData);
void sendResponseNamedPipe(ServerMessage serverResponse, PipeData* namedPipeData);
void disconnectAndReconnectNamedPipes(PipeData* namedPipeData);
