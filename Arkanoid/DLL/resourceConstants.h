#pragma once

//Name Constant
#define TAM 64
//Max Content Constant
#define MAX 256
//TOP10 Buffer
#define TOP10_SIZE (MAX * 10)

#define MAX_IP_SIZE 45

#define BUFFER_SIZE 50

//Shared Memory Name
#define BUFFER_MEMORY_CLIENT_REQUESTS TEXT("ARKANOID_BUFFER_MEMORY_CLIENT_REQUESTS")
#define BUFFER_MEMORY_SERVER_RESPONSES TEXT("ARKANOID_BUFFER_MEMORY_SERVER_RESPONSES")
#define BUFFER_MEMORY_GAME TEXT("ARKANOID_BUFFER_MEMORY_GAME")

//Named Pipes Name
#define NAMED_PIPE_CLIENT_REQUESTS TEXT("\\\\%s\\pipe\\ARKANOID_NAMED_PIPE_CLIENT_REQUESTS")
#define NAMED_PIPE_SERVER_RESPONSES TEXT("\\\\%s\\pipe\\ARKANOID_NAMED_PIPE_SERVER_RESPONSES")
#define NAMED_PIPE_GAME TEXT("\\\\%s\\pipe\\ARKANOID_NAMED_PIPE_GAME")

//Mutex Name
#define MUTEX_CLIENT_REQUEST TEXT("ARKANOID_MUTEX_CLIENT_REQUEST")
#define MUTEX_SERVER_RESPONSES TEXT("ARKANOID_MUTEX_SERVER_RESPONSES")

//Semaphore Name
#define SEMAPHORE_CLIENT_REQUEST_ITEMS TEXT("ARKANOID_SEMAPHORE_CLIENT_REQUEST_ITEMS")
#define SEMAPHORE_CLIENT_REQUEST_EMPTY TEXT("ARKANOID_SEMAPHORE_CLIENT_REQUEST_EMPTY")

#define SEMAPHORE_SERVER_RESPONSE_ITEMS TEXT("ARKANOID_SEMAPHORE_SERVER_RESPONSE_ITEMS")
#define SEMAPHORE_SERVER_RESPONSE_EMPTY TEXT("ARKANOID_SEMAPHORE_SERVER_RESPONSE_EMPTY")

//Event Name
#define EVENT_CLIENT_MESSAGE_CHECK TEXT("ARKANOID_EVENT_CLIENT_MESSAGE_CHECK")
#define EVENT_GAME_UPDATE TEXT("ARKANOID_EVENT_GAME_UPDATE")

//Registry
#define REGISTRY_TOP10_PATH TEXT("Software\\ArkanoidGame")
#define TOP10_REGISTRY_VALUE TEXT("TOP10")
#define TOP10_PLAYER_COUNT TEXT("TOP10PlayerCount")
#define TOP10_DUMMY_VALUE TEXT("SO2;9999;XPTO;8000;Arkanoid;6666;")

#define REGISTRY_KEY_MOVEMENT_PATH TEXT("Software\\ArkanoidGameClient")
#define RIGHT_MOVEMENT_KEY_VALUE TEXT("RIGHT_MOVEMENT_KEY")
#define LEFT_MOVEMENT_KEY_VALUE TEXT("LEFT_MOVEMENT_KEY")

#define RIGHT_MOVEMENT_DEFAULT_KEY TEXT("D")
#define LEFT_MOVEMENT_DEFAULT_KEY TEXT("A")