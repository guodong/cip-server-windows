// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include "uwm.h"


#pragma data_seg(".shared")
HINSTANCE hInstance = NULL;
HHOOK hHook = NULL;
HWND hWndServer = NULL;
#pragma data_seg()
#pragma comment(linker, "/section:.shared,rws")


#define __TEST_CALLWNDPROCRET 1

LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
		return CallNextHookEx(hHook, nCode, wParam, lParam);
#ifdef __TEST_CBT
	if (nCode == HCBT_CREATEWND)
	{
		HWND wd = (HWND)wParam;
		HWND parent = GetAncestor(wd, GA_PARENT);
		DWORD dwStyle = GetWindowLong(wd, GWL_STYLE);
		if (parent != GetDesktopWindow())
		{
			RECT rc;
			GetWindowRect(wd, &rc);
			FILE *f = fopen("C:\\debug.txt", "a");
			fprintf(f, "%d, %d: %d, %d\n", rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
			fclose(f);
		}
	}
#endif
#ifdef __TEST_CALLWNDPROCRET
	CWPRETSTRUCT *msg;
	msg = (CWPRETSTRUCT*)lParam;
	//if(msg->message != WM_MOUSEMOVE && msg->message != 1087 && msg->message != 1048)
		//PostMessage(hWndServer, UWM_DEBUG, (WPARAM)msg->message, 0);
	/// check whether root window
	HWND wd = msg->hwnd;
	HWND parent = GetAncestor(wd, GA_PARENT);
	DWORD dwStyle = GetWindowLong(wd, GWL_STYLE);
	//if (parent == GetDesktopWindow() || msg->message == WM_CONTEXTMENU)
	{
		switch (msg->message)
		{
		case WM_CREATE:
			PostMessage(hWndServer, UWM_CREATE_WINDOW, (WPARAM)msg->hwnd, 0);
			break;
		case WM_SHOWWINDOW:
			if (msg->wParam == TRUE)
			{
				PostMessage(hWndServer, UWM_SHOW_WINDOW, (WPARAM)msg->hwnd, 0);
			}
			else
			{
				PostMessage(hWndServer, UWM_HIDE_WINDOW, (WPARAM)msg->hwnd, 0);
			}
			break;
		case WM_INITMENUPOPUP:
			PostMessage(hWndServer, UWM_PAINT_WINDOW, (WPARAM)msg->wParam, 0);
			break;
		case WM_MOVE:
			PostMessage(hWndServer, UWM_MOVE_WINDOW, (WPARAM)msg->hwnd, 0);
			break;
		case WM_DESTROY:
			PostMessage(hWndServer, UWM_DESTROY_WINDOW, (WPARAM)msg->hwnd, 0);
		default:
			break;
		}
	}
	
	/*if (msg->message == WM_CREATE || msg->message == WM_SHOWWINDOW)
	{
		HWND wd = msg->hwnd;
		HWND parent = GetAncestor(wd, GA_PARENT);
		DWORD dwStyle = GetWindowLong(wd, GWL_STYLE);
		if (parent == GetDesktopWindow())
		{
			PostMessage(hWndServer, 6666, (WPARAM)msg->hwnd, 0);
			RECT rc;
			GetWindowRect(wd, &rc);
			FILE *f = fopen("C:\\debug.txt", "a");
			fprintf(f, "%d: %d %d, %d: %d, %d\n", msg->message, msg->hwnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
			fclose(f);
		}

	}*/
#endif
	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	hInstance = hModule;
	return TRUE;
}




BOOL InstallHook(HWND hWnd, DWORD pid)
{
	hWndServer = hWnd;
	hHook = SetWindowsHookEx(WH_CALLWNDPROCRET, HookProc, hInstance, pid);

	return true;
}

BOOL UninstallHook()
{
	hWndServer = NULL;
	return UnhookWindowsHookEx(hHook);
}