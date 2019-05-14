#include "stdafx.h"
#include "namedPipeLogic.h"
#include "messages.h"

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
	//DisconnectAndReconnectNamedPipes(i);
}
