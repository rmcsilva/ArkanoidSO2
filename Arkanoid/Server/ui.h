#pragma once
//Menu Options
#define WELCOME_MESSAGE "Welcome to Arkanoid Server"

#define START_GAME 1
#define SHOW_TOP10 2
#define LIST_USERS 3
#define SHUTDOWN 4
#include "messages.h"

int initialMenu();
void showResponseMessageInformation(ServerMessage serverMessage, int requestType);
int readInt();