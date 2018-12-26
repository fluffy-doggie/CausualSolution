#include <Windows.h>
#include <Shlwapi.h>
#include <strsafe.h>
#include <tchar.h>

// c运行时库
#include <crtdbg.h>

// 项目头文件
#include "bmfont.h";

// path 操作函数
#pragma comment(lib, "shlwapi.lib")

// 定义全局变量
OPTIONS g_Options;
TCHAR g_szClassName[] = TEXT("BMFontClass");

int CALLBACK EnumFontFamExProc(
	const LOGFONT    *lpelfe,
	const TEXTMETRIC *lpntme,
	DWORD      FontType,
	LPARAM     lParam
) {
	UNREFERENCED_PARAMETER(lpelfe);
	UNREFERENCED_PARAMETER(lpntme);
	UNREFERENCED_PARAMETER(FontType);
	UNREFERENCED_PARAMETER(lParam);

	xprintf(lpelfe->lfFaceName);
	xprintf("\n");

	return 1;
}

int _tmain(int argc, TCHAR *argv[])
{
	// 检测命令行输入是否有误
	if (!ValidOptions(argc, argv)) {
		Usage();
		return -1;
	}

	LOGFONT lf;
	ZeroMemory(&lf, sizeof(lf));
	lf.lfCharSet = GB2312_CHARSET;

	// 枚举所有字体
	HDC dc = GetDC(NULL);
	EnumFontFamiliesEx(dc, &lf, (FONTENUMPROC)EnumFontFamExProc, NULL, 0);
	ReleaseDC(NULL, dc);

	return 0;
}

// 注册窗口类
ATOM RegisterWndClass(HINSTANCE hInstance) {
	WNDCLASSEX wndc;
	ZeroMemory(&wndc, sizeof(wndc));

	wndc.cbSize = sizeof(wndc);
	wndc.style = CS_HREDRAW | CS_VREDRAW;
	wndc.lpfnWndProc = (WNDPROC)MainWndProc;
	wndc.cbClsExtra = 0;
	wndc.cbWndExtra = 0;
	wndc.hInstance = hInstance;
	wndc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndc.hbrBackground = GetStockObject(WHITE_BRUSH);
	wndc.lpszMenuName = NULL;
	wndc.lpszClassName = g_szClassName;

	return RegisterClass(&wndc);
}

// 窗口过程函数
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lPawam) {
	return DefWindowProc(hWnd, uMessage, wParam, lPawam);
}

BOOL ValidOptions(int argc, TCHAR *argv[])
{
	g_Options.bUseGUI = FALSE;
	g_Options.lpszPort = NULL;

	// 
	// Cch: 以字符为单位
	// Cb: 以字节为单位

	// 获取文件名
	size_t cbLength = xstrlen(argv[0]);
	TCHAR *lpszTemp = xmalloc(cbLength + sizeof(TCHAR));
	_ASSERT(SUCCEEDED(StringCbCopy(lpszTemp, cbLength + sizeof(TCHAR), argv[0])));

	PathStripPath(lpszTemp);
	_ASSERT(SUCCEEDED(StringCbCopy(g_Options.szAppName, 64, lpszTemp)));
	xfree(lpszTemp);

	if (1 == argc) {
		return FALSE;
	}

	// 逐个比较有效参数，
	// 如果正确则继续下次循环
	for (int i = 1; i < argc; ++i) {
		if (CSTR_EQUAL == CompareString(
			LOCALE_CUSTOM_DEFAULT, 0,
			CharLower(argv[i]), -1,
			TEXT("--help"), -1)) {

			return FALSE;
		}

		// 启动gui
		if (CSTR_EQUAL == CompareString(
			LOCALE_CUSTOM_DEFAULT, 0,
			CharLower(argv[i]), -1,
			TEXT("--gui"), -1)) {

			g_Options.bUseGUI = TRUE;
			continue;
		}

		// 设置端口
		if (CSTR_EQUAL == CompareString(
			LOCALE_CUSTOM_DEFAULT, 0,
			CharLower(argv[i]), -1,
			TEXT("--port"), -1)) {

			g_Options.lpszPort = argv[++i];
			continue;
		}

		xprintf(TEXT("Invalid argument: \"%s\"\n"), argv[i]);
		return FALSE;
	}

	return TRUE;
}

// 显示如何使用
void Usage() {
	xprintf(TEXT("\nUsage:\n"));
	xprintf(TEXT("    %s\t[--gui] --[port=<port>] [--help]\n"), g_Options.szAppName);
	xprintf(TEXT("\t--gui\t\t\tRun this program trough windows.\n"));
	xprintf(TEXT("\t--port=[<port>]\t\tAccept internet command through this port.\n"));
	xprintf(TEXT("\t--help\t\t\tShow this page.\n"));
}

// 获取字符长度 不包含\0
size_t xstrlen(const TCHAR *lpszString) {
	size_t cchLength;
	_ASSERT(SUCCEEDED(StringCchLength(lpszString, STRSAFE_MAX_CCH * sizeof(TCHAR), &cchLength)));

	return cchLength;
}

void xprintf(const TCHAR *lpFormat, ...)
{
	static TCHAR s_szBuffer[512];
	ZeroMemory(s_szBuffer, sizeof(s_szBuffer));

	va_list arglist;
	va_start(arglist, lpFormat);

	_ASSERT(SUCCEEDED(StringCchVPrintf(s_szBuffer, sizeof(s_szBuffer), lpFormat, arglist)));

	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	_ASSERT(hOut != INVALID_HANDLE_VALUE);

	DWORD dwLength;
	WriteConsole(hOut, s_szBuffer, xstrlen(s_szBuffer), &dwLength, NULL);
}


// Usage:

//  common settings:
//  --gui						start with graphic user interface.
//  --port=<port>				accept internet command request(http).
//	--help						display this page.

//  font settings:
//  --font-family=<Arial>		系统字体
//  --font-file=<*.ttf>			自定义字体文件	
//  --font-size=<px>			字体尺寸
//	--font-height=<100%>		字符高度
//  --charset=<default>			字符编码
//  --font-italic				斜体
//  --font-bold					加粗
//  --match-char-height			匹配字符高度
//	--font-outline=<0px>		外描边

//	additional selections:
//	--output-invalid-char-glyph
//	--do-not-include-kerning-pairs

//	--render-from-truetype-outline
//	--truetype-hinting
//	--font-smoothing
//	--super-sampling=<level>
//	--clear-type

//	export options:
//	--padding=<0,0,0,0>
//	--spacing=<2,2>
//	--equalize-cell-heights
//	--force-offsets-to-zero

//	--texture-size=<256,256>
//	--bit-depth=<32>
//	--pack-chars-in-multiple-channels
//	--texture-alpha=<glyph|outline|encoded-glyph|zero|one|invert>
//	--texture-red=<glyph|outline|encoded-glyph|zero|one|invert>
//	--texture-green=<glyph|outline|encoded-glyph|zero|one|invert>
//	--texture-blue=<glyph|outline|encoded-glyph|zero|one|invert>
//	--texture-mode=<white-text-with-alpha |
//					black-text-with-alpha |
//					white-text-on-black	|
//					black-text-on-white |
//					outlined-text-with-alpha |
//					text-outline-same-channel
//					>
//  --data-format=<text|xml|binary>
//  --sheet-format=<dds|png|tga>
//	--compression=<none|dxt1|dxt3|dxt5|deflate|run-length-encoded>

//	input/output
//  --string=<string>
//  --string-file=<*.txt>
//  --export=<filepath>