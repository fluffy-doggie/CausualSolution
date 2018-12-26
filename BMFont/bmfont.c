#include <Windows.h>
#include <Shlwapi.h>
#include <strsafe.h>
#include <tchar.h>

// c运行时库
#include <crtdbg.h>

// 项目头文件
#include "bmfont.h"
#include "gui.h"

// path 操作函数
#pragma comment(lib, "shlwapi.lib")

// 定义全局变量
OPTIONS g_Options;
HWND g_hWndMain;

TCHAR g_szClassName[] = TEXT("BMFontClass");
TCHAR g_szWndName[] = TEXT("BMFont Maker");

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

	if (g_Options.bUseGUI) {
		return WndMainLoop(GetModuleHandle(NULL), NULL, NULL, SW_SHOW);
	}

	//LOGFONT lf;
	//ZeroMemory(&lf, sizeof(lf));
	//lf.lfCharSet = GB2312_CHARSET;

	//// 枚举所有字体
	//HDC dc = GetDC(NULL);
	//EnumFontFamiliesEx(dc, &lf, (FONTENUMPROC)EnumFontFamExProc, NULL, 0);
	//ReleaseDC(NULL, dc);

	return 0;
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
void Usage_ZhCN() {
	xprintf(TEXT("\n使用说明：\n"));
	xprintf(TEXT("    %s\t[--gui] --[port=<port>] [--help]\n"), g_Options.szAppName);
	xprintf(TEXT("\t--gui\t\t\t通过窗口运行程序\n"));
	xprintf(TEXT("\t--port=[<port>]\t\t接收来自网络的任务\n"));
	xprintf(TEXT("\t--help\t\t\t显示帮助页面\n"));
}

void Usage_EnUS() {
	xprintf(TEXT("\nUsage:\n"));
	xprintf(TEXT("    %s\t[--gui] --[port=<port>] [--help]\n"), g_Options.szAppName);
	xprintf(TEXT("\t--gui\t\t\tRun this program though windows\n"));
	xprintf(TEXT("\t--port=[<port>]\t\tReceive commands from network\n"));
	xprintf(TEXT("\t--help\t\t\tDisplay this page\n"));
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