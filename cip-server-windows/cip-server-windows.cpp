// cip-server-windows.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <windows.h>
#include <stdlib.h>
#include <boost/thread/thread.hpp>
#include "WebsocketServer.h"

WebsocketServer wsServer;

typedef BOOL(CALLBACK *INSTALLHOOK)(HWND, DWORD);
typedef BOOL(CALLBACK *UNINSTALLHOOK)();

static TCHAR szWindowClass[] = _T("win32app");

// The string that appears in the application's title bar.
static TCHAR szTitle[] = _T("Win32 Guided Tour Application");

HINSTANCE hInst;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void WsServerThread()
{
	wsServer.run(9002);
}

int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	boost::thread thrd(&WsServerThread);

	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

	if (!RegisterClassEx(&wcex))
	{
		MessageBox(NULL,
			_T("Call to RegisterClassEx failed!"),
			_T("Win32 Guided Tour"),
			NULL);

		return 1;
	}

	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindow(
		szWindowClass,
		szTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		500, 100,
		NULL,
		NULL,
		hInstance,
		NULL
		);

	if (!hWnd)
	{
		MessageBox(NULL,
			_T("Call to CreateWindow failed!"),
			_T("Win32 Guided Tour"),
			NULL);

		return 1;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	HINSTANCE hDLL = LoadLibrary(TEXT("C:\\Users\\Administrator\\Documents\\Visual Studio 2015\\Projects\\cip-server-windows\\x64\\Debug\\hook.dll"));

	INSTALLHOOK InstallHook = (INSTALLHOOK)GetProcAddress(hDLL, "InstallHook");
	UNINSTALLHOOK UninstallHook = (UNINSTALLHOOK)GetProcAddress(hDLL, "UninstallHook");
	HWND notepadhandle = FindWindow(L"Notepad", NULL);
	if (notepadhandle == NULL) {
		printf("Notepad Not Found.\n");
		return TRUE;
	}
	DWORD pid = GetWindowThreadProcessId(notepadhandle, NULL);
	int ret = InstallHook(hWnd, pid);

	if (!ret)
	{
		DWORD dw = GetLastError();
		printf("err %u\n", dw);
	}

	// Main message loop:
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (msg.message == 6666)
		{
			FILE *f = fopen("C:\\debug.txt", "a");
			fprintf(f, "grecv: %d\n", (int)msg.wParam);
			fclose(f);
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	UninstallHook();
	FreeLibrary(hDLL);

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	TCHAR greeting[] = _T("Hello, World!");

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);

		TextOut(hdc,
			5, 5,
			greeting, _tcslen(greeting));
		// End application-specific layout section.

		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case 6666: {
		FILE *f = fopen("C:\\debug.txt", "a");
		fprintf(f, "recv: %d\n", (int)wParam);
		fclose(f);
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;
}