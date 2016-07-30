// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include "uwm.h"


#pragma data_seg(".shared")
HINSTANCE hInstance = NULL;
HHOOK hHookCBT = NULL;
HHOOK hHookCWPR = NULL;
HWND hWndServer = NULL;
#pragma data_seg()
#pragma comment(linker, "/section:.shared,rws")


#define __TEST_CALLWNDPROCRET 1
//#definwh_getmessagee __TEST_CBT
//#define __TEST_MSG
//#define __TEST_CALLWNDPROC

LRESULT CALLBACK HookCWPR(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
		return CallNextHookEx(hHookCWPR, nCode, wParam, lParam);
#ifdef __TEST_MSG
	MSG *msg = (MSG*)lParam;
	if(LOWORD(msg->message) == WM_CREATE)
		PostMessage(hWndServer, WM_APP + LOWORD(msg->message), (WPARAM)msg->hwnd, (LPARAM)msg->wParam);
#endif
#ifdef __TEST_CBT
	PostMessage(hWndServer, WM_APP + nCode, wParam, 0);
	/*switch (nCode) {
	case HCBT_CREATEWND: {
		PostMessage(hWndServer, WM_APP + HCBT_CREATEWND, wParam, 0);
		break;
	}
	case HCBT_DESTROYWND: {
		PostMessage(hWndServer, WM_APP + HCBT_DESTROYWND, wParam, 0);
		break;
	}
	case HCBT_ACTIVATE: {
		PostMessage(hWndServer, WM_APP + HCBT_ACTIVATE, wParam, 0);
		break;
	}
	case HCBT_MOVESIZE: {
		PostMessage(hWndServer, WM_APP + HCBT_MOVESIZE, wParam, 0);
		break;
	}
	default:
		break;
	}*/
	//if (nCode == HCBT_CREATEWND) {
	//	PostMessage(hWndServer, WM_APP + HCBT_CREATEWND, wParam, 0);
	//	/*HWND wd = (HWND)wParam;
	//	HWND parent = GetAncestor(wd, GA_PARENT);
	//	DWORD dwStyle = GetWindowLong(wd, GWL_STYLE);
	//	if (parent != GetDesktopWindow()) {
	//		RECT rc;
	//		GetWindowRect(wd, &rc);
	//		FILE *f = fopen("C:\\debug.txt", "a");
	//		fprintf(f, "%d, %d: %d, %d\n", rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
	//		fclose(f);
	//	}*/
	//}
#endif
#ifdef __TEST_CALLWNDPROC
	CWPSTRUCT *msg = (CWPSTRUCT*)lParam;
	PostMessage(hWndServer, WM_APP + msg->message, (WPARAM)msg->hwnd, (LPARAM)msg->wParam);
#endif
#ifdef __TEST_CALLWNDPROCRET
	CWPRETSTRUCT *msg;
	msg = (CWPRETSTRUCT*)lParam;
	if (msg->message == WM_INITMENUPOPUP) {

		PostMessage(hWndServer, WM_APP + 0x4000 + msg->message, (WPARAM)msg->wParam, (LPARAM)msg->wParam);
	} else {
		PostMessage(hWndServer, WM_APP + 0x4000 + msg->message, (WPARAM)msg->hwnd, (LPARAM)msg->wParam);
	}
	//if(msg->message != WM_MOUSEMOVE && msg->message != 1087 && msg->message != 1048)
		//PostMessage(hWndServer, UWM_DEBUG, (WPARAM)msg->message, 0);
	/// check whether root window
	//HWND wd = msg->hwnd;
	//HWND parent = GetAncestor(wd, GA_PARENT);
	//DWORD dwStyle = GetWindowLong(wd, GWL_STYLE);
	//if (parent == GetDesktopWindow())// || msg->message == WM_CONTEXTMENU)
	//{
	//	switch (msg->message) {
	//	case WM_CREATE:
	//		PostMessage(hWndServer, UWM_CREATE_WINDOW, (WPARAM)msg->hwnd, 0);
	//		break;
	//	case WM_SHOWWINDOW:
	//		if (msg->wParam == TRUE) {
	//			PostMessage(hWndServer, UWM_SHOW_WINDOW, (WPARAM)msg->hwnd, 0);
	//		} else {
	//			PostMessage(hWndServer, UWM_HIDE_WINDOW, (WPARAM)msg->hwnd, 0);
	//		}
	//		break;
	//	case WM_INITMENUPOPUP:
	//		PostMessage(hWndServer, UWM_PAINT_WINDOW, (WPARAM)msg->wParam, 0);
	//		break;
	//	case WM_MOVE:
	//	case WM_WINDOWPOSCHANGED:
	//		PostMessage(hWndServer, UWM_MOVE_WINDOW, (WPARAM)msg->hwnd, 0);
	//		break;
	//	case WM_DESTROY:
	//		PostMessage(hWndServer, UWM_DESTROY_WINDOW, (WPARAM)msg->hwnd, 0);
	//		break;
	//	case WM_COMMAND:
	//		PostMessage(hWndServer, UWM_COMMAND_WINDOW, (WPARAM)msg->wParam, (LPARAM)msg->lParam);
	//		break;
	//	case WM_CTLCOLOREDIT:
	//		PostMessage(hWndServer, UWM_PAINT_WINDOW, (WPARAM)msg->hwnd, 0);
	//		break;
	//	case 0x0118:
	//		PostMessage(hWndServer, UWM_BLINK, (WPARAM)msg->hwnd, 0);
	//		break;
	//	default:
	//		break;
	//	}
	//}
	//HWND ans = GetAncestor(wd, GA_ROOT);
	//if (msg->message == WM_PAINT) {
	//	PostMessage(hWndServer, UWM_PAINT_WINDOW, (WPARAM)ans, 0);
	//}

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
	return CallNextHookEx(hHookCWPR, nCode, wParam, lParam);
}

LRESULT CALLBACK HookCBT(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
		return CallNextHookEx(hHookCWPR, nCode, wParam, lParam);
	PostMessage(hWndServer, WM_APP + nCode, wParam, 0);
	return CallNextHookEx(hHookCWPR, nCode, wParam, lParam);
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	switch (ul_reason_for_call) {
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
	hHookCWPR = SetWindowsHookEx(WH_CALLWNDPROCRET, HookCWPR, hInstance, pid);
	hHookCBT = SetWindowsHookEx(WH_CBT, HookCBT, hInstance, pid);

	return true;
}

BOOL UninstallHook()
{
	hWndServer = NULL;
	return UnhookWindowsHookEx(hHookCBT);
}