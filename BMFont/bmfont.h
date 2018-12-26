#ifndef __BMFONT_H__
#define __BMFONT_H__

#define xmalloc(s)	HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(s))
#define xfree(p)	if(NULL !=p) { HeapFree(GetProcessHeap(),0,(p)); p=NULL; }

size_t xstrlen(const TCHAR *lpszString);

void xprintf(const TCHAR *lpFormat, ...);

typedef struct _OPTIONS {
	TCHAR szAppName[64];
	TCHAR *lpszPort;
	BOOL bUseGUI;
} OPTIONS, *LPOPTIONS;

BOOL ValidOptions(int argc, TCHAR *argv[]);

void Usage();

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

ATOM RegisterWndClass(HINSTANCE hInstance);

#endif