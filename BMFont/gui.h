#ifndef __GUI_H__
#define __GUI_H__

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

ATOM RegisterWndClass(HINSTANCE hInstance);

BOOL CreateMainWindow(HINSTANCE hInstance, int nCmdShow);

int WndMainLoop(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

#endif // !__GUI_H__