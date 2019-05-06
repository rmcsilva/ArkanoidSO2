#pragma once
#include "messages.h"
#include "gameStructs.h"

//Menu Options
#define WELCOME_MESSAGE "Welcome to Arkanoid Server\n"

#define START_GAME 1
#define SHOW_TOP10 2
#define LIST_USERS 3
#define SHUTDOWN 4

int initialMenu();
void showResponseMessageInformation(ServerMessage serverMessage, int requestType);
void showTopPlayers(TopPlayer* topPlayers, DWORD playerCount);
void showConnectedUsers(Player* users, int connectedUsers);