#include "stdafx.h"
#include "namedPipeLogic.h"
#include "setup.h"
#include "ui.h"

ServerMessage userLogin(ClientMessage* clientMessage);
ServerMessage sendTop10(ClientMessage* clientMessage);
void movePlayerBarrier(int playerId, int direction);
void userLogout(int userID);

void sendGameNamedPipe(GameData gameData, PipeData* pipeData)
{
	DWORD nBytes, dwErr;

	BOOL fSuccess = WriteFile(
		pipeData->hGamePipe,
		&gameData,
		sizeof(gameData),
		&nBytes,
		&pipeData->overlappedGame);

	// The write operation completed successfully. 
	if (fSuccess && nBytes == sizeof(gameData))
	{
		pipeData->fPendingIOGame = FALSE;
		return;
	}

	// The write operation is still pending. 
	dwErr = GetLastError();
	if (!fSuccess && (dwErr == ERROR_IO_PENDING))
	{
		pipeData->fPendingIOGame = TRUE;
		return;
	}

	// An error occurred; disconnect from the client.
	disconnectAndReconnectNamedPipes(pipeData);
}


void receiveRequestNamedPipe(ClientMessage clientRequest, PipeData* namedPipeData)
{
	ServerMessage serverResponse;

	switch (clientRequest.type)
	{
	case LOGIN_REQUEST:
		serverResponse = userLogin(&clientRequest);
		sendResponseNamedPipe(serverResponse, namedPipeData);
		if (serverResponse.type == REQUEST_ACCEPTED)
		{
			namedPipeData->userID = serverResponse.id;
		}
		showResponseMessageInformation(serverResponse, clientRequest.type);
		break;
	case TOP10:
		serverResponse = sendTop10(&clientRequest);
		sendResponseNamedPipe(serverResponse, namedPipeData);
		showResponseMessageInformation(serverResponse, clientRequest.type);
		break;
	case MOVE_RIGHT:
		movePlayerBarrier(clientRequest.id, MOVE_RIGHT);
		break;
	case MOVE_LEFT:
		movePlayerBarrier(clientRequest.id, MOVE_LEFT);
		break;
	case LOGOUT:
		userLogout(clientRequest.id);
		disconnectAndReconnectNamedPipes(namedPipeData);
		break;
		//TODO: implement
	default:
		break;
	}
}

void sendResponseNamedPipe(ServerMessage serverResponse, PipeData* namedPipeData)
{
	DWORD nBytes, dwErr;

	BOOL fSuccess = WriteFile(
		namedPipeData->hServerResponsesPipe,
		&serverResponse,
		sizeof(serverResponse),
		&nBytes,
		&namedPipeData->overlappedRequests);

	// The write operation completed successfully. 
	if (fSuccess && nBytes == sizeof(serverResponse))
	{
		namedPipeData->fPendingIORequests = FALSE;
		namedPipeData->dwStateRequests = READING_REQUEST_STATE;
		return;
	}

	// The write operation is still pending. 
	dwErr = GetLastError();
	if (!fSuccess && (dwErr == ERROR_IO_PENDING))
	{
		namedPipeData->fPendingIORequests = TRUE;
		namedPipeData->dwStateRequests = WRITING_RESPONSE_STATE;
		return;
	}

	// An error occurred; disconnect from the client.
	disconnectAndReconnectNamedPipes(namedPipeData);
}

void disconnectAndReconnectNamedPipes(PipeData* namedPipeData)
{
	// Disconnect the pipe instance. 
	if (!DisconnectNamedPipe(namedPipeData->hClientRequestsPipe))
	{
		_tprintf(TEXT("DisconnectNamedPipe hClientRequestsPipe failed with %d.\n"), GetLastError());
	}

	if (!DisconnectNamedPipe(namedPipeData->hServerResponsesPipe))
	{
		_tprintf(TEXT("DisconnectNamedPipe hServerResponsesPipe failed with %d.\n"), GetLastError());
	}

	if (!DisconnectNamedPipe(namedPipeData->hGamePipe))
	{
		_tprintf(TEXT("DisconnectNamedPipe hGamePipe failed with %d.\n"), GetLastError());
	}

	namedPipeData->userID = UNDEFINED_ID;

	// Call a subroutine to connect to the new client. 
	namedPipeData->fPendingIORequests = newPlayerPipeConnection(
		namedPipeData->hClientRequestsPipe,
		&namedPipeData->overlappedRequests);

	namedPipeData->dwStateRequests = namedPipeData->fPendingIORequests
		? CONNECTING_STATE :		// still connecting  
		READING_REQUEST_STATE;		// ready to read clients request

	newPlayerPipeConnection(
		namedPipeData->hServerResponsesPipe,
		&namedPipeData->overlappedRequests);

	namedPipeData->fPendingIOGame = newPlayerPipeConnection(
		namedPipeData->hGamePipe,
		&namedPipeData->overlappedGame);

	namedPipeData->dwStateGame = namedPipeData->fPendingIOGame
		? CONNECTING_STATE :		// still connecting 
		WRITING_GAME_STATE;			// ready to read clients request
}