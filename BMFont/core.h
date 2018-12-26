#ifndef __CORE_H__
#define __CORE_H__

// 材质保存格式
#define FMT_Tga		(1)
#define FMT_Png		(2)
#define FMT_Dds		(3)

// 文档保存格式
#define FMT_Binary	(4)
#define FMT_Text	(5)
#define FMT_Xml		(6)

// font descriptor
typedef struct _FONTDESC {
	HFONT hFont;			// 字体句柄
	TCHAR *lpszFontFamily;	// 字符家族

	BOOL bBold;				// 粗体
	BOOL bSmooth;			// 平滑
	BOOL bItalic;			// 斜体
	BOOL bClearType;		// use clear type
	BOOL bHinting;			// ttf-hinting
	BOOL bFixedHeight;		// 固定高度
	BOOL bForceToZero;		// force offset to zero

	//BOOL b
	DWORD dwSize;			// 字体大小
	DWORD dwOutline;		// 描边
	DWORD dwCharSet;		// 字符集
	DWORD dwHeight;			// 字符高度

	DWORD dwPaddingLeft;	// 左间距
	DWORD dwPaddingRight;	// 右间距
	DWORD dwPaddingUp;		// 上间距
	DWORD dwPaddingDown;	// 下间距

	DWORD dwHorizontalSpace;	// 水平间隔
	DWORD dwVerticalSpace;		// 垂直间隔

} FONTDESC, *LPFONTDESC;

typedef struct _FILEDESC {
	DWORD dwWidth;
	DWORD dwHeight;
	DWORD dwDepth;

	DWORD dwTextureFmt;
	DWORD dwDataFmt;

	DWORD dwChannelA;
	DWORD dwChannelR;
	DWORD dwChannelG;
	DWORD dwChannelB;

	BOOL bInvertA;
	BOOL bInvertR;
	BOOL bInvertG;
	BOOL bInvertB;

	TCHAR *lpszSelected;	// 选择的字符集
	TCHAR *lpszFileName;	// 保存的文件名
} FILEDESC, *LPFILEDESC;

// 初始化字体描述
void InitFontDesc(LPFONTDESC lpFontDesc) {
	//FONTDESC fontDesc;
	ZeroMemory(lpFontDesc, sizeof(FONTDESC));

	lpFontDesc->lpszFontFamily = "Arial";
	lpFontDesc->dwCharSet = ANSI_CHARSET;
	lpFontDesc->dwSize = 32;
	lpFontDesc->bSmooth = TRUE;
	lpFontDesc->bClearType = TRUE;
	lpFontDesc->bHinting = TRUE;
	lpFontDesc->bBold = FALSE;
	lpFontDesc->bItalic = FALSE;
	lpFontDesc->dwHorizontalSpace = 1;
	lpFontDesc->dwVerticalSpace = 1;
	lpFontDesc->dwHeight = 100;
	lpFontDesc->bFixedHeight = FALSE;
	lpFontDesc->bForceToZero = FALSE;

}

// 按照指定的结构创建字体
BOOL CreateSpecifiedFont(LPFONTDESC lpFontDesc) {
	// 创建字体
	lpFontDesc->hFont = CreateFont(
		lpFontDesc->dwSize,
		0, 0, 0,
		lpFontDesc->bBold ? FW_BOLD : FW_NORMAL,
		lpFontDesc->bItalic,
		0, 0,
		lpFontDesc->dwCharSet,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		lpFontDesc->bSmooth ? ANTIALIASED_QUALITY : NONANTIALIASED_QUALITY,
		DEFAULT_PITCH,
		lpFontDesc->lpszFontFamily);

	return NULL != lpFontDesc->hFont;
}

typedef struct _IMAGEDESC {
	DWORD dwWidth;		// 宽度
	DWORD dwHeight;		// 高度
	DWORD *lpdwData;	// 像素数据
	BOOL bReverseY;		// 翻转Y轴（高度）

} IMAGEDESC, *LPIMAGEDESC;

// 创建图像缓存
LPIMAGEDESC CreateImageDesc(DWORD width, DWORD height) {
	LPIMAGEDESC lpDesc = xmalloc(sizeof(IMAGEDESC));
	if (!lpDesc) return NULL;

	lpDesc->dwWidth  = width;
	lpDesc->dwHeight = height;

	lpDesc->lpdwData = xmalloc(width * height * sizeof(DWORD));
	if (!lpDesc->lpdwData) {
		xfree(lpDesc);
		return NULL;
	}

	return lpDesc;
}

// 清空图像数据
#define ClearImageDesc(lpDesc) {ZeroMemory(lpDesc->lpdwData, lpDesc->dwWidth*lpDesc->dwHeight * sizeof(DWORD));}

// 释放图像缓存
void FreeImageDesc(LPIMAGEDESC *ppDesc) {
	LPIMAGEDESC lpDesc = *ppDesc;
	if (lpDesc->lpdwData) {
		xfree(lpDesc->lpdwData);
	}

	xfree(lpDesc);
}

void ToBitmapInfoHeader(LPIMAGEDESC lpDesc, BITMAPINFOHEADER *lpInfoHeader) {
	lpInfoHeader->biSize = sizeof(BITMAPINFOHEADER);
	lpInfoHeader->biBitCount = 32;
	lpInfoHeader->biWidth = lpDesc->dwWidth;
	lpInfoHeader->biHeight = lpDesc->bReverseY ? -(LONG)lpDesc->dwHeight : lpDesc->dwHeight;
	lpInfoHeader->biCompression = BI_RGB;
	lpInfoHeader->biPlanes = 1;
	lpInfoHeader->biSizeImage = 0;
	lpInfoHeader->biClrImportant = 0;
	lpInfoHeader->biClrUsed = 0;
	lpInfoHeader->biXPelsPerMeter = 0;
	lpInfoHeader->biYPelsPerMeter = 0;
}

#endif