#include "stdafx.h"
#include "namedPipeLogic.h"
#include "setup.h"
#include "ui.h"

ServerMessage userLogin(ClientMessage* clientMessage);
ServerMessage sendTop10(ClientMessage* clientMessage);
void userLogout(int userID);

void sendGameNamedPipe(GameData gameData, PipeData* pipeData)
{
	DWORD nBytes, dwErr;

	BOOL fSuccess = WriteFile(
		pipeData->hGamePipe,
		&gameData,
		sizeof(gameData),
		&nBytes,
		&pipeData->overlapped);

	// The write operation completed successfully. 
	if (fSuccess && nBytes == sizeof(gameData))
	{
		pipeData->fPendingIO = FALSE;
		pipeData->dwState = READING_REQUEST_STATE;
		return;
	}

	// The write operation is still pending. 
	dwErr = GetLastError();
	if (!fSuccess && (dwErr == ERROR_IO_PENDING))
	{
		pipeData->fPendingIO = TRUE;
		pipeData->dwState = WRITING_GAME_STATE;
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
		&namedPipeData->overlapped);

	// The write operation completed successfully. 
	if (fSuccess && nBytes == sizeof(serverResponse))
	{
		namedPipeData->fPendingIO = FALSE;
		namedPipeData->dwState = READING_REQUEST_STATE;
		return;
	}

	// The write operation is still pending. 
	dwErr = GetLastError();
	if (!fSuccess && (dwErr == ERROR_IO_PENDING))
	{
		namedPipeData->fPendingIO = TRUE;
		namedPipeData->dwState = WRITING_RESPONSE_STATE;
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
	namedPipeData->fPendingIO = newPlayerPipeConnection(
		namedPipeData->hClientRequestsPipe,
		&namedPipeData->overlapped);

	namedPipeData->dwState = namedPipeData->fPendingIO
		? CONNECTING_STATE :		// still connecting  
		READING_REQUEST_STATE;		// ready to read clients request

	newPlayerPipeConnection(
		namedPipeData->hServerResponsesPipe,
		&namedPipeData->overlapped);

	newPlayerPipeConnection(
		namedPipeData->hGamePipe,
		&namedPipeData->overlapped);
}