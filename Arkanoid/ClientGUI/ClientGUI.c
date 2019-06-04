// ClientGUI.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "ClientGUI.h"
#include <tchar.h>
#include <windowsx.h>
#include <winuser.h>
#include "resource.h"
#include <winuser.h>
#include "DLL.h"
#include "util.h"
#include "constants.h"
#include "setup.h"
#include <Mmsystem.h>
#include <windows.h>


#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
BOOL shutdown = FALSE;							// used to signal errors that cause the server to shutdown
GameData gameData;
HWND gHwnd;

HANDLE hGameThread;
DWORD dwGameThreadId;
int playerIndex = UNDEFINED_ID;

DWORD mainThreadId;								

HANDLE hRegistryKey;

TCHAR rightMovementKey;
TCHAR leftMovementKey;

HFONT hScoreFont;

BOOL isMusicPlaying = TRUE;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	loginEventsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK	settingsEventsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
DWORD WINAPI		GameUpdate(LPVOID lpParam);
void				drawGame(GameData gameData);
void				drawGameBackground();
void				showTop10();
HBRUSH				convertIndexToPlayerBarrierBrush(int index);
HBRUSH				convertIndexToBonusIndicatorBrush(int index);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CLIENTGUI, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CLIENTGUI));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LOGO));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_CLIENTGUI);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_LOGO));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

	//Setup client area
   RECT rect = {0};
   rect.right = DIM_X_FRAME;
   rect.bottom = DIM_Y_FRAME;

   AdjustWindowRect(
	   &rect,
	   WS_CAPTION |
	   WS_BORDER | 
	   WS_SYSMENU |
	   WS_THICKFRAME | 
	   WS_MINIMIZEBOX | 
	   WS_MAXIMIZEBOX,
	   TRUE
   );

   rect.right += abs(rect.left);
   rect.bottom += abs(rect.top);
	
   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, rect.right, rect.bottom, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   gHwnd = hWnd;

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//Global variables used to draw on screen
HDC hMemDC = NULL;
HBITMAP hMemBitmap = NULL;
HBRUSH hBrush = NULL;
HDC hTempDC = NULL;

HBITMAP hGameBackgroundBitmap;
BITMAP gameBackgroundBitmap;

HBRUSH hPlayerBarrierColors[MAX_PLAYERS];
HBITMAP hBricksBitmap[TOTAL_BRICK_TYPES];
HICON hBallIcon;
HICON hLivesIcon;
RECT scoreBox;
//Bonus icons
HICON hSlowIcon;
HICON hFastIcon;
HICON hExtraIcon;
HICON hTripleIcon;
HBRUSH hBonusIndicatorColors[GAME_BONUS_INIDICATOR_TOTAL_COLORS];

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int xPos, yPos;
	TCHAR letter;
	RECT rect;
	HDC hDC;
	PAINTSTRUCT ps;

    switch (message)
    {
	case WM_CREATE:
		mainThreadId = GetCurrentThreadId();

		DialogBox(hInst, MAKEINTRESOURCE(IDD_LOGIN), hWnd, loginEventsDialog);

		if (shutdown == TRUE) {
			DestroyWindow(hWnd);
			break;
		}

		if (receiveMessage(LOGIN_REQUEST) == REQUEST_ACCEPTED)
		{
			MessageBox(hWnd, TEXT("Sucessufully connected to the server!"), TEXT("Login"), MB_OK);
		}
		else
		{
			MessageBox(hWnd, TEXT("Could not connect to the server...\nTry again!"), TEXT("Login"), MB_OK);
			DestroyWindow(hWnd);
			break;
		}

		//Load assets
		hDC = GetDC(hWnd);
		hMemDC = CreateCompatibleDC(hDC);
		hMemBitmap = CreateCompatibleBitmap(hDC, DIM_X_FRAME, DIM_Y_FRAME);

		SelectObject(hMemDC, hMemBitmap);

		hGameBackgroundBitmap = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_ARKANOID_GAME), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);
		GetObject(hGameBackgroundBitmap, sizeof(gameBackgroundBitmap), &gameBackgroundBitmap);

		drawGameBackground();

		ReleaseDC(hWnd, hDC);

		//Load barrier colors
		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			hPlayerBarrierColors[i] = convertIndexToPlayerBarrierBrush(i);
		}

		//Load bonus indicator colors
		for (int i = 0; i < GAME_BONUS_INIDICATOR_TOTAL_COLORS; ++i)
		{
			hBonusIndicatorColors[i] = convertIndexToBonusIndicatorBrush(i);
		}

		//Load Bricks
		hBricksBitmap[BRICK_RESISTANCE1_INDEX] = (HBITMAP)LoadImage(NULL, BRICK_RESISTANCE1_PATH, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		hBricksBitmap[BRICK_RESISTANCE2_INDEX] = (HBITMAP)LoadImage(NULL, BRICK_RESISTANCE2_PATH, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		hBricksBitmap[BRICK_RESISTANCE3_INDEX] = (HBITMAP)LoadImage(NULL, BRICK_RESISTANCE3_PATH, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		hBricksBitmap[BRICK_RESISTANCE4_INDEX] = (HBITMAP)LoadImage(NULL, BRICK_RESISTANCE4_PATH, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		hBricksBitmap[BRICK_BONUS_INDEX] = (HBITMAP)LoadImage(NULL, BRICK_BONUS_PATH, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

		hBallIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_BALL), IMAGE_ICON, 0, 0, LR_LOADTRANSPARENT);
		hLivesIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_LIVES), IMAGE_ICON, 0, 0, LR_LOADTRANSPARENT);

		scoreBox.left = GAME_SCORE_LEFT;
		scoreBox.top = GAME_SCORE_TOP;
		scoreBox.right = GAME_SCORE_RIGHT;
		scoreBox.bottom = GAME_SCORE_BOTTOM;

		hScoreFont = CreateFont(GAME_SCORE_FONT_SIZE, 0, 0, 0, FW_DONTCARE, FALSE, TRUE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
			CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Impact"));

		hSlowIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SLOW), IMAGE_ICON, 0, 0, LR_LOADTRANSPARENT);
		hFastIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_FAST), IMAGE_ICON, 0, 0, LR_LOADTRANSPARENT);
		hExtraIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_EXTRA), IMAGE_ICON, 0, 0, LR_LOADTRANSPARENT);
		hTripleIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_TRIPLE), IMAGE_ICON, 0, 0, LR_LOADTRANSPARENT);

		//Game movement keys
		setupMovementKeys(&hRegistryKey, &rightMovementKey, &leftMovementKey);

		//Thread to handle the game updates
		hGameThread = CreateThread(
			NULL,					// default security attributes
			0,						// use default stack size  
			GameUpdate,				// thread function name
			NULL,					// argument to thread function 
			0,						// use default creation flags 
			&dwGameThreadId);		// returns the thread identifier

		PlaySound(GAME_MUSIC_PATH, NULL, SND_FILENAME | SND_LOOP | SND_ASYNC | SND_NODEFAULT);
		break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_SETTINGS:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_SETTINGS), hWnd, settingsEventsDialog);
                break;
			case IDM_REQUESTTOP10:
				sendMessage(TOP10);
				receiveMessage(TOP10);
				showTop10();
				break;
			case IDM_MUSIC:
				if (isMusicPlaying == TRUE)
				{
					isMusicPlaying = FALSE;
					PlaySound(NULL, 0, 0);
				} else
				{
					isMusicPlaying = TRUE;
					PlaySound(GAME_MUSIC_PATH, NULL, SND_FILENAME | SND_LOOP | SND_ASYNC | SND_NODEFAULT);
				}
				break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
	case WM_CHAR:
		if(inGame == TRUE)
		{
			//TODO: Check if player is playing or just watching
			letter = (TCHAR)wParam;
			letter = _totupper(letter);

			if(letter == rightMovementKey)
			{
				sendMessage(MOVE_RIGHT);
			} else if(letter == leftMovementKey)
			{
				sendMessage(MOVE_LEFT);
			}
		}
		break;
	case WM_MOUSEMOVE:
		if(inGame == TRUE)
		{
			xPos = GET_X_LPARAM(lParam);

			int barrierX = gameData.barrier[playerIndex].position.x;

			if(xPos > barrierX + gameData.barrierDimensions / 2)
			{
				sendMessage(MOVE_RIGHT);
			} else if(xPos < barrierX + gameData.barrierDimensions / 2)
			{
				sendMessage(MOVE_LEFT);
			}
		}
		break;
	case WM_ERASEBKGND:
		return (LRESULT)1;
    case WM_PAINT:
		if (inGame == TRUE)
		{
			drawGame(gameData);
		} else
		{
			drawGameBackground();
		}

		hDC = BeginPaint(hWnd, &ps);

		BitBlt(hDC, 0, 0, DIM_X_FRAME, DIM_Y_FRAME, hMemDC, 0, 0, SRCCOPY);

		SelectObject(hDC, hMemDC);

		EndPaint(hWnd, &ps);
        break;
	case WM_CLOSE:
		if (shutdown == TRUE) {
			DestroyWindow(hWnd);
			return 1;
		}
		if (MessageBox(hWnd, TEXT("Really quit?"), TEXT("Quit Game"), MB_YESNO) == IDYES)
		{
			DestroyWindow(hWnd);
		}
		break;
    case WM_DESTROY:
		//TODO: Delete the other objects
		DeleteObject(hGameBackgroundBitmap);

		for(int i = 0; i < TOTAL_BRICK_TYPES; i++)
		{
			DeleteObject(hBricksBitmap[i]);
		}

		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			DeleteObject(hPlayerBarrierColors[i]);
		}

		for (int i = 0; i < GAME_BONUS_INIDICATOR_TOTAL_COLORS; ++i)
		{
			DeleteObject(hBonusIndicatorColors[i]);
		}

		DestroyIcon(hBallIcon);
		DestroyIcon(hLivesIcon);

		DeleteObject(hScoreFont);

		DeleteObject(hMemBitmap);
		DeleteDC(hMemDC);

		RegCloseKey(hRegistryKey);
		logout();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

DWORD WINAPI GameUpdate(LPVOID lpParam)
{
	int status;

	while (shutdown == FALSE)
	{
		while (shutdown == FALSE)
		{
			__try
			{
				gameData = receiveBroadcast();
				status = gameData.gameStatus;
			}
			__except (filter(GetExceptionCode()))
			{
				return -1;
			}

			if (status == GAME_OVER)
			{
				break;
			}
			else if (status == LOGOUT)
			{
				inGame = FALSE;
				shutdown = TRUE;
				PostThreadMessage(mainThreadId, WM_QUIT, 0, 0);
				return -1;
			}

			if(playerIndex == UNDEFINED_ID)
			{
				for (int i = 0; i < gameData.numPlayers; i++)
				{
					if (gameData.player[i].id == id)
					{
						playerIndex = i;
						break;
					}
				}
			}
			inGame = TRUE;

			InvalidateRect(gHwnd, NULL, TRUE);
		}

		inGame = FALSE;
		playerIndex = UNDEFINED_ID;

		InvalidateRect(gHwnd, NULL, TRUE);

		//Receives the top10
		sendMessage(TOP10);
		receiveMessage(TOP10);
		showTop10();
	}

	return 1;
}

void drawGame(GameData gameData)
{
	hTempDC = CreateCompatibleDC(hMemDC);

	SelectObject(hTempDC, hGameBackgroundBitmap);
	BitBlt(hMemDC, 0, 0, DIM_X_FRAME, DIM_Y_FRAME, hTempDC, 0, 0, SRCCOPY);

	//Draw bricks
	for (int i = 0; i < MAX_BRICKS; i++)
	{
		if (gameData.brick[i].resistance > 0)
		{
			if (gameData.brick[i].hasBonus == TRUE)
			{
				SelectObject(hTempDC, hBricksBitmap[BRICK_BONUS_INDEX]);
			} else
			{
				switch (gameData.brick[i].resistance)
				{
					case 1:
						SelectObject(hTempDC, hBricksBitmap[BRICK_RESISTANCE1_INDEX]);
						break;
					case 2:
						SelectObject(hTempDC, hBricksBitmap[BRICK_RESISTANCE2_INDEX]);
						break;
					case 3:
						SelectObject(hTempDC, hBricksBitmap[BRICK_RESISTANCE3_INDEX]);
						break;
					case 4:
						SelectObject(hTempDC, hBricksBitmap[BRICK_RESISTANCE4_INDEX]);
						break;
				}
			}

			BitBlt(hMemDC, gameData.brick[i].position.x, gameData.brick[i].position.y, BRICKS_WIDTH, BRICKS_HEIGHT, hTempDC, 0, 0, SRCCOPY);
		}
	}

	//Draw bonus
	for (int i = 0; i < MAX_BONUS; i++)
	{
		if (gameData.bonus[i].status == BONUS_IN_PLAY)
		{
			HICON hBonus = NULL;
			switch (gameData.bonus[i].type)
			{
				case BONUS_SPEED_UP:
					hBonus = hFastIcon;
					break;
				case BONUS_SLOW_DOWN:
					hBonus = hSlowIcon;
					break;
				case BONUS_EXTRA_LIFE:
					hBonus = hExtraIcon;
					break;
				case BONUS_TRIPLE_BALL:
					hBonus = hTripleIcon;
					break;
			}
			DrawIcon(hMemDC, gameData.bonus[i].position.x, gameData.bonus[i].position.y, hBonus);
		}
	}

	SelectObject(hTempDC, hBallIcon);
	//Draw balls
	int x, y, velocityRatio;
	for (int i = 0; i < MAX_BALLS; i++)
	{
		if (gameData.ball[i].inPlay == TRUE)
		{
			x = gameData.ball[i].position.x;
			y = gameData.ball[i].position.y;
			DrawIcon(hMemDC, x, y, hBallIcon);
			
			velocityRatio = gameData.ball[i].velocityRatio;
			if (velocityRatio != 100)
			{
				x += GAME_BONUS_INDICATOR_DIAMETER;
				y += GAME_BONUS_INDICATOR_DIAMETER;
				
				if (velocityRatio >= 200)		SelectObject(hMemDC, hBonusIndicatorColors[GAME_BONUS_SPEED_UP_5]);
				else if (velocityRatio >= 180)	SelectObject(hMemDC, hBonusIndicatorColors[GAME_BONUS_SPEED_UP_4]);
				else if (velocityRatio >= 160)	SelectObject(hMemDC, hBonusIndicatorColors[GAME_BONUS_SPEED_UP_3]);
				else if (velocityRatio >= 140)	SelectObject(hMemDC, hBonusIndicatorColors[GAME_BONUS_SPEED_UP_2]);
				else if (velocityRatio >= 120)	SelectObject(hMemDC, hBonusIndicatorColors[GAME_BONUS_SPEED_UP_1]);
				else if (velocityRatio >= 80)	SelectObject(hMemDC, hBonusIndicatorColors[GAME_BONUS_SLOW_DOWN_1]);
				else if (velocityRatio >= 60)	SelectObject(hMemDC, hBonusIndicatorColors[GAME_BONUS_SLOW_DOWN_2]);

				Ellipse(hMemDC, x, y, x + GAME_BONUS_INDICATOR_DIAMETER, y + GAME_BONUS_INDICATOR_DIAMETER);
			}
		}
	}

	//Draw Barrier Indicator
	RECT barrier;
	barrier.left = GAME_BARRIER_INDICATOR_X;;
	barrier.top = GAME_BARRIER_INDICATOR_Y;
	barrier.right = GAME_BARRIER_INDICATOR_X + GAME_BARRIER_INDICATOR_WIDTH;
	barrier.bottom = GAME_BARRIER_INDICATOR_Y + GAME_BARRIER_INDICATOR_HEIGHT;
	FillRect(hMemDC, &barrier, hPlayerBarrierColors[playerIndex]);

	//Draw Barriers
	for(int i = 0; i < gameData.numPlayers; i++)
	{
		if (gameData.player[i].inGame == TRUE)
		{
			barrier.left = gameData.barrier[i].position.x;
			barrier.top = gameData.barrier[i].position.y;
			barrier.right = gameData.barrier[i].position.x + gameData.barrierDimensions * gameData.barrier[i].sizeRatio;
			barrier.bottom = DIM_Y_FRAME;
			FillRect(hMemDC, &barrier, hPlayerBarrierColors[i]);
		}
	}

	//Draw Lives
	int livesPositionX = GAME_LIVES_START_X;
	for(int i = 0; i < gameData.lives; i++)
	{
		DrawIcon(hMemDC, livesPositionX, GAME_LIVES_START_Y, hLivesIcon);
		livesPositionX += GAME_LIVES_WIDTH + GAME_LIVES_MARGIN;
	}

	//Draw score
	SelectObject(hMemDC, hScoreFont);
	SetTextColor(hMemDC, RGB(255, 255, 255));
	SetBkMode(hMemDC, TRANSPARENT);
	TCHAR score[12];
	_stprintf_s(score, 12, TEXT("%d"), gameData.player[playerIndex].score);
	DrawText(hMemDC, &score, -1, &scoreBox, DT_CENTER);
		
	SelectObject(hMemDC, hTempDC);
	DeleteDC(hTempDC);
}

void drawGameBackground()
{
	hTempDC = CreateCompatibleDC(hMemDC);

	SelectObject(hTempDC, hGameBackgroundBitmap);
	BitBlt(hMemDC, 0, 0, DIM_X_FRAME, DIM_Y_FRAME, hTempDC, 0, 0, BLACKNESS);
	BitBlt(hMemDC, 0, 0, DIM_X_FRAME, DIM_Y_FRAME, hTempDC, 0, 0, SRCCOPY);

	DeleteObject(hTempDC);
}

void showTop10()
{
	TCHAR* nextToken = NULL;
	TCHAR value[TOP10_SIZE];
	_tcscpy_s(value, TOP10_SIZE, top10);
	TCHAR* token = _tcstok_s(value, TEXT(";"), &nextToken);
	int counter = 1;
	TCHAR top10Message[TOP10_SIZE] = TEXT("");
	TCHAR temp[MAX] = TEXT("");

	while (token != NULL)
	{
		_stprintf_s(temp, MAX, TEXT("Top %d "), counter);
		_tcscat_s(top10Message, TOP10_SIZE, temp);

		//Copy username
		_stprintf_s(temp, MAX, TEXT("Username: %s "), token);
		_tcscat_s(top10Message, TOP10_SIZE, temp);

		//Get score
		token = _tcstok_s(NULL, TEXT(";"), &nextToken);
		_stprintf_s(temp, MAX, TEXT("Score: %s\n\n"), token);
		_tcscat_s(top10Message, TOP10_SIZE, temp);

		token = _tcstok_s(NULL, TEXT(";"), &nextToken);

		counter++;
	}

	MessageBox(NULL, top10Message, TEXT("Top 10"), MB_OK);
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Message handler for login dialog box.
INT_PTR CALLBACK loginEventsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	TCHAR ip[MAX_IP_SIZE];
	int status;

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_CLOSE:
		if (MessageBox(hDlg, TEXT("Really quit?"), TEXT("Quit Login"), MB_YESNO) == IDYES)
		{
			EndDialog(hDlg, LOWORD(wParam));
			shutdown = TRUE;
		}
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			GetDlgItemText(hDlg, IDC_USERNAME, username, TAM);
			GetDlgItemText(hDlg, IDC_IP, ip, MAX_IP_SIZE);
			
			trimWhiteSpace(username);

			if (_tcslen(username) == 0) 
			{
				break;
			}

			trimWhiteSpace(ip);

			if (_tcslen(ip) == 0)
			{
				status = login(username, NULL);
			} else
			{
				status = login(username, ip);
			}

			if (status == -1) {
				shutdown = TRUE;
			}

			EndDialog(hDlg, LOWORD(wParam));

			return (INT_PTR)TRUE;
			break;
		case IDCANCEL:
			SendMessage(hDlg, WM_CLOSE, 1, 1);
			return (INT_PTR)TRUE;
			break;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK settingsEventsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	TCHAR rightKey[2];
	TCHAR leftKey[2];

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		rightKey[0] = rightMovementKey; rightKey[1] = TEXT('\0');
		leftKey[0] = leftMovementKey; leftKey[1] = TEXT('\0');
		SetDlgItemText(hDlg, IDC_RIGHT, rightKey);
		SetDlgItemText(hDlg, IDC_LEFT, leftKey);
		return (INT_PTR)TRUE;

	case WM_CLOSE:
		if (MessageBox(hDlg, TEXT("Discard settings?"), TEXT("Quit Settings"), MB_YESNO) == IDYES)
		{
			EndDialog(hDlg, LOWORD(wParam));
		}
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			GetDlgItemText(hDlg, IDC_RIGHT, rightKey, 2);
			GetDlgItemText(hDlg, IDC_LEFT, leftKey, 2);

			if (_tcslen(rightKey) == 0 || _tcslen(leftKey) == 0)
			{
				break;
			}

			if(rightKey[0] == leftKey[0])
			{
				break;
			}

			rightMovementKey = _totupper(rightKey[0]);
			leftMovementKey = _totupper(leftKey[0]);

			saveMovementKeys(&hRegistryKey, &rightMovementKey, &leftMovementKey);

			EndDialog(hDlg, LOWORD(wParam));

			return (INT_PTR)TRUE;
			break;
		case IDCANCEL:
			SendMessage(hDlg, WM_CLOSE, 1, 1);
			return (INT_PTR)TRUE;
			break;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

HBRUSH convertIndexToPlayerBarrierBrush(int index)
{
	HBRUSH temp = CreateSolidBrush(RGB(0,0,0));

	switch (index)
	{
		case 0:
			temp = CreateSolidBrush(GAME_PLAYER_BARRIER_COLOR_0);
			break;
		case 1:
			temp = CreateSolidBrush(GAME_PLAYER_BARRIER_COLOR_1);
			break;
		case 2:
			temp = CreateSolidBrush(GAME_PLAYER_BARRIER_COLOR_2);
			break;
		case 3:
			temp = CreateSolidBrush(GAME_PLAYER_BARRIER_COLOR_3);
			break;
		case 4:
			temp = CreateSolidBrush(GAME_PLAYER_BARRIER_COLOR_4);
			break;
		case 5:
			temp = CreateSolidBrush(GAME_PLAYER_BARRIER_COLOR_5);
			break;
		case 6:
			temp = CreateSolidBrush(GAME_PLAYER_BARRIER_COLOR_6);
			break;
		case 7:
			temp = CreateSolidBrush(GAME_PLAYER_BARRIER_COLOR_7);
			break;
		case 8:
			temp = CreateSolidBrush(GAME_PLAYER_BARRIER_COLOR_8);
			break;
		case 9:
			temp = CreateSolidBrush(GAME_PLAYER_BARRIER_COLOR_9);
			break;
		case 10:
			temp = CreateSolidBrush(GAME_PLAYER_BARRIER_COLOR_10);
			break;
		case 11:
			temp = CreateSolidBrush(GAME_PLAYER_BARRIER_COLOR_11);
			break;
		case 12:
			temp = CreateSolidBrush(GAME_PLAYER_BARRIER_COLOR_12);
			break;
		case 13:
			temp = CreateSolidBrush(GAME_PLAYER_BARRIER_COLOR_13);
			break;
		case 14:
			temp = CreateSolidBrush(GAME_PLAYER_BARRIER_COLOR_14);
			break;
		case 15:
			temp = CreateSolidBrush(GAME_PLAYER_BARRIER_COLOR_15);
			break;
		case 16:
			temp = CreateSolidBrush(GAME_PLAYER_BARRIER_COLOR_16);
			break;
		case 17:
			temp = CreateSolidBrush(GAME_PLAYER_BARRIER_COLOR_17);
			break;
		case 18:
			temp = CreateSolidBrush(GAME_PLAYER_BARRIER_COLOR_18);
			break;
		case 19:
			temp = CreateSolidBrush(GAME_PLAYER_BARRIER_COLOR_19);
			break;
	}

	return temp;
}

HBRUSH convertIndexToBonusIndicatorBrush(int index) 
{
	HBRUSH temp = CreateSolidBrush(RGB(0, 0, 0));

	switch (index)
	{
	case 0:
		temp = CreateSolidBrush(GAME_BONUS_SLOW_DOWN_1_COLOR);
		break;
	case 1:
		temp = CreateSolidBrush(GAME_BONUS_SLOW_DOWN_2_COLOR);
		break;
	case 2:
		temp = CreateSolidBrush(GAME_BONUS_SPEED_UP_1_COLOR);
		break;
	case 3:
		temp = CreateSolidBrush(GAME_BONUS_SPEED_UP_2_COLOR);
		break;
	case 4:
		temp = CreateSolidBrush(GAME_BONUS_SPEED_UP_3_COLOR);
		break;
	case 5:
		temp = CreateSolidBrush(GAME_BONUS_SPEED_UP_4_COLOR);
		break;
	case 6:
		temp = CreateSolidBrush(GAME_BONUS_SPEED_UP_5_COLOR);
		break;
	}

	return temp;
}