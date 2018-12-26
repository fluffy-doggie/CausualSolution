#include <Windows.h>

#include "bmfont.h"
#include "gui.h"

extern TCHAR g_szClassName[];
extern TCHAR g_szWndName[];

extern HWND g_hWndMain;

// 窗口过程函数
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lPawam) {
	return DefWindowProc(hWnd, uMessage, wParam, lPawam);
}

// 注册窗口类
ATOM RegisterWndClass(HINSTANCE hInstance) {
	WNDCLASSEX wndc;
	//ZeroMemory(&wndc, sizeof(wndc));

	wndc.cbSize = sizeof(wndc);
	wndc.style = CS_HREDRAW | CS_VREDRAW;
	wndc.lpfnWndProc = (WNDPROC)MainWndProc;
	wndc.cbClsExtra = 0;
	wndc.cbWndExtra = 0;
	wndc.hInstance = hInstance;
	wndc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndc.hIconSm = 0;
	wndc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndc.lpszMenuName = NULL;
	wndc.lpszClassName = g_szClassName;
	//wndc.lpszClassName = TEXT("BMFont");

	return RegisterClassEx(&wndc);
}

BOOL CreateMainWindow(HINSTANCE hInstance, int nCmdShow) {
	g_hWndMain = CreateWindowEx(
		WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
		g_szClassName,
		g_szWndName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL,
		hInstance, NULL);

	if (!g_hWndMain) {
		return FALSE;
	}

	ShowWindow(g_hWndMain, nCmdShow);
	UpdateWindow(g_hWndMain);

	return TRUE;
}

int WndMainLoop() {
	HINSTANCE hInstance = GetModuleHandle(NULL);

	//HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow
	
	//DWORD ret = RegisterWndClass(hInstance);

	if (!RegisterWndClass(hInstance)) {
		TCHAR buffer[512];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), LANG_USER_DEFAULT, buffer, 512, NULL);
		xprintf(buffer);

		return -1;
	}
	
	if (!CreateMainWindow(hInstance, SW_SHOW))
		return -1;

	MSG msg;
	while (GetMessage(&msg, NULL, 0x00, 0x00)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}
