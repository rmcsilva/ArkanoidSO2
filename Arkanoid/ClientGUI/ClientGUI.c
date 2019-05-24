// ClientGUI.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "ClientGUI.h"
#include <windows.h>
#include <tchar.h>
#include <windowsx.h>
#include <winuser.h>
#include "resource.h"
#include <winuser.h>
#include "DLL.h"
#include "util.h"
#include "Constants.h"
#include "setup.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
BOOL shutdown = FALSE;							// used to signal errors that cause the server to shutdown

HANDLE hGameThread;
DWORD dwGameThreadId;

DWORD mainThreadId;								

HANDLE hRegistryKey;

TCHAR rightMovementKey;
TCHAR leftMovementKey;

HBITMAP hGameBackgroundBitmap;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	loginEventsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK	settingsEventsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
DWORD WINAPI		GameUpdate(LPVOID lpParam);

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

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, DIM_X_FRAME, DIM_Y_FRAME + 55, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

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
	RECT rt;

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
		hGameBackgroundBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ARKANOID_GAME));

		//Game movement keys
		setupMovementKeys(&hRegistryKey, &rightMovementKey, &leftMovementKey);

		//Thread to handle the game updates
		hGameThread = CreateThread(
			NULL,						// default security attributes
			0,				// use default stack size  
			GameUpdate,					// thread function name
			NULL,						// argument to thread function 
			0,				// use default creation flags 
			&dwGameThreadId);			// returns the thread identifier
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
				//TODO: Complete
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
			yPos = GET_Y_LPARAM(lParam);

			//TODO: Verify with barrier position
		}
		break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
			GetClientRect(hWnd, &rt);
			HDC hmemdc = CreateCompatibleDC(hdc);
			SelectObject(hmemdc, hGameBackgroundBitmap);
			BitBlt(hdc, 0,0, DIM_X_FRAME, DIM_Y_FRAME, hmemdc, 0, 0, SRCCOPY);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
			DeleteDC(hmemdc);
        }
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
		DeleteObject(hGameBackgroundBitmap);

		RegCloseKey(hRegistryKey);
		logout();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
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

DWORD WINAPI GameUpdate(LPVOID lpParam)
{
	int status;
	while(shutdown == FALSE)
	{
		while (shutdown == FALSE)
		{
			status = receiveBroadcast();

			if (status == GAME_OVER)
			{
				inGame = FALSE;
				break;
			}
			else if (status == LOGOUT)
			{
				inGame = FALSE;
				shutdown = TRUE;
				PostThreadMessage(mainThreadId, WM_QUIT, 0, 0);
				return -1;
			}
			inGame = TRUE;
			//TODO: Update GUI
			//Export the game memory pointer and update?
		}

		inGame = FALSE;

		//Receives the top10
		sendMessage(TOP10);
		receiveMessage(TOP10);
	}
	
	return 1;
}