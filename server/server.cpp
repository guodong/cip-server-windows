// server.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "env.h"
#include <string>
#include <fstream>
#include <stdlib.h>
#include <string>
#include "server.h"
#include "WebsocketServer.h"
#include <boost/thread/thread.hpp>
#include "uwm.h"
#include "cip.h"
#include "cip_protocol.h"
#include "cip_window.h"
#include "x264.h"


using namespace std;

cip_context_t cip_context;
WebsocketServer wsServer;

mutex windows_lock;


typedef BOOL(CALLBACK *INSTALLHOOK)(HWND, DWORD);
typedef BOOL(CALLBACK *UNINSTALLHOOK)();

void WsServerThread()
{
	wsServer.run(0);
}

#define MAX_LOADSTRING 100

// 全局变量: 
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
HWND hWnd;

// 此代码模块中包含的函数的前向声明: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

map<int, cip_window_t*> windows;
DWORD pid = 0;

BOOL isTopWindow(HWND hwnd)
{
	//return (hwnd == GetAncestor(hwnd, GA_ROOT));
	//DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
	////BOOL istop = !(dwStyle & WS_CHILD);
	////return istop;
	//if (dwStyle & WS_POPUP) {
	//	return true;
	//}
	if (hwnd == GetAncestor(hwnd, GA_ROOT)) {
		return true;
	}
	HWND parent = GetAncestor(hwnd, GA_PARENT);
	return (parent == GetDesktopWindow());
}

BOOL CALLBACK proc(HWND hwnd, LPARAM lParam)
{
	TCHAR title[200];
	GetWindowText(hwnd, title, sizeof(title));
	if (0 == _tcscmp(title, L"Program Manager") || 0 == _tcscmp(title, L"")) {
		return 1;
	}
	TCHAR klass[200];
	GetClassName(hwnd, klass, sizeof(klass));
	if (0 == _tcscmp(klass, L"Internet Explorer_Hidden")
		|| 0 == _tcscmp(klass, L"TaskSwitcherOverlayWnd")
		|| 0 == _tcscmp(klass, L"EdgeUiInputWndClass")) {
		return 1;
	}
	DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
	cip_window_t *win = (cip_window_t*)malloc(sizeof(cip_window_t));
	if (dwStyle & WS_MINIMIZE) {
		return 1;
	}
	if (pid != 0) {
		DWORD _pid = GetWindowThreadProcessId(hwnd, NULL);
		if (_pid != pid) {
			return 1;
		}
	}
	if (!isTopWindow(hwnd)) {
		return 1;
	}

	RECT rc;
	GetWindowRect(hwnd, &rc);

	RECT winRc;
	GetWindowRect(hwnd, &winRc);

	win->wid = (u32)hwnd;
	win->x = winRc.left;
	win->y = winRc.top;
	win->width = rc.right - rc.left;
	win->height = rc.bottom - rc.top;
	if (dwStyle & WS_VISIBLE) {
		win->visible = 1;
	} else {
		win->visible = 0;
	}
	win->bare = 1;
	win->stream_ready = 0;
	windows[win->wid] = win;
	cip_window_stream_init(win);
	return 1;
}
int stop = 0;

void windowFrameLoop()
{
	while (1) {
		if (stop) {
			break;
		}
		windows_lock.lock();
		map<int, cip_window_t*>::iterator it;
		for (it = windows.begin(); it != windows.end(); it++) {
			if (it->second->visible)
				cip_window_frame_send(it->second, 0);
			Sleep(10);
		}
		windows_lock.unlock();
		Sleep(10);
	}
}

HANDLE hPipe;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: 在此放置代码。

#ifndef __DEV
	// check whether user is a cloudware user
	TCHAR username[50];
	DWORD usernameSize = 50;
	GetUserName(username, &usernameSize);
	if (_tcscmp(username, _T("Administrator")) == 0) {
		return 0;
	}
	TCHAR pipename[128] = _T("\\\\.\\cloudware\\pipe\\");
	wcscat(pipename, username);
	while (1) {
		hPipe = CreateFile(
			pipename,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
		if (hPipe != INVALID_HANDLE_VALUE) {
			break;
		}
		// wait 10s if pipe is busy
		if (!WaitNamedPipe(pipename, 10000)) {
			return -1;
		}
	}

	DWORD dwMode = PIPE_READMODE_MESSAGE;
	BOOL fSuccess = SetNamedPipeHandleState(
		hPipe,
		&dwMode,
		NULL,
		NULL);
	TCHAR program[1024];
	DWORD cbRead;
	fSuccess = ReadFile(
		hPipe,
		program,
		1024 * sizeof(TCHAR),
		&cbRead,
		NULL);

#endif

	boost::thread thrd(&WsServerThread);
	//Sleep(1000);
	//boost::thread thrd1(&windowFrameLoop);

	// 初始化全局字符串
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_SERVER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 执行应用程序初始化: 
	if (!InitInstance(hInstance, nCmdShow)) {
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SERVER));

	// 开始DLL注入
	HINSTANCE hDLL = LoadLibrary(TEXT("C:\\Users\\Administrator\\Documents\\Visual Studio 2015\\Projects\\cip-server-windows\\x64\\Debug\\hook.dll"));

	INSTALLHOOK InstallHook = (INSTALLHOOK)GetProcAddress(hDLL, "InstallHook");
	UNINSTALLHOOK UninstallHook = (UNINSTALLHOOK)GetProcAddress(hDLL, "UninstallHook");
	HWND notepadhandle = FindWindow(L"Notepad", NULL);
	if (notepadhandle == NULL) {
		printf("Notepad Not Found.\n");
		return TRUE;
	}
	//pid = GetWindowThreadProcessId(notepadhandle, NULL);
	int ret = InstallHook(hWnd, pid);

	if (!ret) {
		DWORD dw = GetLastError();
		printf("err %u\n", dw);
	}
#ifndef __DEV
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	CreateProcess(
		NULL,
		program,
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&si,
		&pi);
#endif

	//EnumWindows(proc, NULL);
	MSG msg;

	// 主消息循环: 
	while (GetMessage(&msg, nullptr, 0, 0)) {
		/*if (msg.message == WM_APP + HCBT_CREATEWND) {
			std::stringstream ss;
			ss << msg.message << ", " << msg.wParam;
			wsServer.broadcast(ss.str(), 1);
		}
		continue;*/
		/*if (msg.message == WM_APP + WM_CREATE) {
			std::stringstream ss;
			ss << msg.message - WM_APP << ", " << msg.wParam;
			wsServer.broadcast(ss.str(), 1);
		}
		continue;*/
		
		switch (msg.message) {
		case WM_APP + HCBT_CREATEWND: {
			Sleep(60);
			HWND hwnd = (HWND)(msg.wParam);
			if (msg.message == (WM_APP + WM_INITMENUPOPUP)) {
				hwnd = (HWND)(msg.lParam);
			}
			if (isTopWindow(hwnd)) {
				RECT rc;
				GetWindowRect(hwnd, &rc);
				if (rc.left == rc.right || rc.top == rc.bottom) {
					//break;
				}
				DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);

				cip_window_t *win = (cip_window_t*)malloc(sizeof(cip_window_t));
				win->wid = (u32)hwnd;
				win->x = rc.left;
				win->y = rc.top;
				win->width = rc.right - rc.left;
				win->height = rc.bottom - rc.top;
				win->visible = IsWindowVisible(hwnd);
				win->bare = 1;
				win->stream_ready = 0;
				win->stream_on = 0;
				win->stream_end = 0;
				win->stream_reset = 0;
				win->stream_thread = CreateThread(NULL, 0, cip_window_frame_thread, win, 0, NULL);
				
				windows_lock.lock();
				windows[win->wid] = win;
				//cip_window_stream_init(win);
				windows_lock.unlock();

				cip_event_window_create_t *cewc = (cip_event_window_create_t*)malloc(sizeof(cip_event_window_create_t));
				cewc->type = CIP_EVENT_WINDOW_CREATE;
				cewc->wid = (u32)hwnd;
				cewc->x = (u32)rc.left;
				cewc->y = (u32)rc.top;
				cewc->width = (u32)rc.right - (u32)rc.left;
				cewc->height = (u32)rc.bottom - (u32)rc.top;
				cewc->bare = 1;
				wsServer.broadcast(cewc, sizeof(cip_event_window_create_t), 2);
				free(cewc);
				//win->visible = 1;
				if (win->visible) {
					cip_event_window_show_t *cews = (cip_event_window_show_t*)malloc(sizeof(cip_event_window_show_t));
					cews->type = CIP_EVENT_WINDOW_SHOW;

					cews->wid = (u32)hwnd;
					cews->bare = 0;
					wsServer.broadcast(cews, sizeof(cip_event_window_show_t), 2);
					free(cews);
					//windows_lock.lock();
					cip_window_stream_start(win);
					//cip_window_frame_send(win, 0);
					//windows_lock.unlock();
				}
			}
			break;
		}
		case WM_APP + HCBT_DESTROYWND: {
			if (windows.find((int)msg.wParam) == windows.end()) {
				break;
			}
			/*DWORD dwStyle = GetWindowLong((HWND)msg.wParam, GWL_STYLE);
			if (!(dwStyle & WS_VISIBLE)) {
				break;
			}*/
			cip_window_stream_stop(windows[(int)msg.wParam]);
			cip_window_t *win = windows[(int)msg.wParam];
			win->stream_end = 1;
			cip_event_window_destroy_t *cewd = (cip_event_window_destroy_t*)malloc(sizeof(cip_event_window_destroy_t));
			cewd->type = CIP_EVENT_WINDOW_DESTROY;
			cewd->wid = (u32)msg.wParam;
			wsServer.broadcast(cewd, sizeof(cip_event_window_destroy_t), 2);
			free(cewd);
			windows_lock.lock();
			windows.erase((int)msg.wParam);
			windows_lock.unlock();
			Sleep(50);
			free(win);
			break;
		}
		case WM_APP + 0x4000 + WM_SHOWWINDOW: {
			HWND hwnd = (HWND)msg.wParam;
			if (windows.find((int)msg.wParam) == windows.end()) {
				break;
			}
			Sleep(10);
			BOOL visible = IsWindowVisible(hwnd);
			windows[(int)msg.wParam]->visible = visible ? 1 : 0;
			if (visible) {
				cip_event_window_show_t *cews = (cip_event_window_show_t*)malloc(sizeof(cip_event_window_show_t));
				cews->type = CIP_EVENT_WINDOW_SHOW;

				cews->wid = (u32)msg.wParam;
				cews->bare = 1;
				wsServer.broadcast(cews, sizeof(cip_event_window_show_t), 2);
				//cip_window_frame_send(windows[(int)hwnd], 0);
				free(cews);
				cip_window_stream_start(windows[(int)hwnd]);
			} else {
				cip_window_stream_stop(windows[(int)hwnd]);
				cip_event_window_hide_t cewh;
				cewh.type = CIP_EVENT_WINDOW_HIDE;
				cewh.wid = (u32)msg.wParam;
				wsServer.broadcast(&cewh, sizeof(cip_event_window_hide_t), 2);
			}
			
			break;
		}
		case WM_APP + 0x4000 + WM_SIZE: {
			HWND hwnd = (HWND)msg.wParam;
			if (windows.find((int)hwnd) == windows.end()) {
				break;
			}
			RECT rc;
			GetWindowRect(hwnd, &rc);
			cip_event_window_configure_t cewc;
			cewc.type = CIP_EVENT_WINDOW_CONFIGURE;
			cewc.wid = (u32)hwnd;
			cewc.x = rc.left;
			cewc.y = rc.top;
			cewc.width = rc.right - rc.left;
			cewc.height = rc.bottom - rc.top;
			cewc.bare = 1;
			cewc.above = 0;
			wsServer.broadcast(&cewc, sizeof(cip_event_window_configure_t), 2);
			windows_lock.lock();
			cip_window_t *win = windows[(u32)hwnd];
			if (win->width != cewc.width || win->height != cewc.height) {
				win->width = cewc.width;
				win->height = cewc.height;
				if(win->stream_ready)
					cip_window_stream_reset(win);

			}
			windows_lock.unlock();
			break;
		}
		case WM_APP + 0x4000 + WM_WINDOWPOSCHANGED: {
			if (windows.find((int)msg.wParam) == windows.end()) {
				break;
			}
			RECT rc, winRc;
			GetWindowRect((HWND)msg.wParam, &rc);
			GetWindowRect((HWND)msg.wParam, &winRc);
			cip_event_window_configure_t *cewc = (cip_event_window_configure_t*)malloc(sizeof(cip_event_window_configure_t));
			cewc->type = CIP_EVENT_WINDOW_CONFIGURE;
			cewc->wid = (u32)msg.wParam;
			cewc->x = winRc.left;
			cewc->y = winRc.top;
			cewc->width = rc.right - rc.left;
			cewc->height = rc.bottom - rc.top;
			cewc->bare = 1;
			cewc->above = 0;
			wsServer.broadcast(cewc, sizeof(cip_event_window_configure_t), 2);
			free(cewc);
			windows[(int)msg.wParam]->x = rc.left;
			windows[(int)msg.wParam]->y = rc.top;
			break;
		}
									
		default:
			break;
		}

		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	stop = 1;
	Sleep(1000);
	return (int)msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SERVER));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_SERVER);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释: 
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // 将实例句柄存储在全局变量中

	hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 500, 100, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd) {
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// 分析菜单选择: 
		switch (wmId) {
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: 在此处添加使用 hdc 的任何绘图代码...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message) {
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
