#ifndef __CORE_H__
#define __CORE_H__

// ���ʱ����ʽ
#define FMT_Tga		(1)
#define FMT_Png		(2)
#define FMT_Dds		(3)

// �ĵ������ʽ
#define FMT_Binary	(4)
#define FMT_Text	(5)
#define FMT_Xml		(6)

// font descriptor
typedef struct _FONTDESC {
	HFONT hFont;			// ������
	TCHAR *lpszFontFamily;	// �ַ�����

	BOOL bBold;				// ����
	BOOL bSmooth;			// ƽ��
	BOOL bItalic;			// б��
	BOOL bClearType;		// use clear type
	BOOL bHinting;			// ttf-hinting
	BOOL bFixedHeight;		// �̶��߶�
	BOOL bForceToZero;		// force offset to zero

	//BOOL b
	DWORD dwSize;			// �����С
	DWORD dwOutline;		// ���
	DWORD dwCharSet;		// �ַ���
	DWORD dwHeight;			// �ַ��߶�

	DWORD dwPaddingLeft;	// ����
	DWORD dwPaddingRight;	// �Ҽ��
	DWORD dwPaddingUp;		// �ϼ��
	DWORD dwPaddingDown;	// �¼��

	DWORD dwHorizontalSpace;	// ˮƽ���
	DWORD dwVerticalSpace;		// ��ֱ���

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

	TCHAR *lpszSelected;	// ѡ����ַ���
	TCHAR *lpszFileName;	// ������ļ���
} FILEDESC, *LPFILEDESC;

// ��ʼ����������
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

// ����ָ���Ľṹ��������
BOOL CreateSpecifiedFont(LPFONTDESC lpFontDesc) {
	// ��������
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
	DWORD dwWidth;		// ���
	DWORD dwHeight;		// �߶�
	DWORD *lpdwData;	// ��������
	BOOL bReverseY;		// ��תY�ᣨ�߶ȣ�

} IMAGEDESC, *LPIMAGEDESC;

// ����ͼ�񻺴�
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

// ���ͼ������
#define ClearImageDesc(lpDesc) {ZeroMemory(lpDesc->lpdwData, lpDesc->dwWidth*lpDesc->dwHeight * sizeof(DWORD));}

// �ͷ�ͼ�񻺴�
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