#include "utils.h"


const wchar_t windowSavedTemplate[] = L"%s - Notepad in night mode";
const wchar_t windowUnsavedTemplate[] = L"*%s - Notepad in night mode";


void _Error(HWND hWndOwner, DWORD errorCode)
{
	wchar_t text[25];

	wsprintf(text, L"%lu", errorCode);

	MessageBox(hWndOwner, text, L"An error occured", MB_OK | MB_ICONERROR);
}

void Error(HWND hWndOwner, DWORD errorCode)
{
	_Error(hWndOwner, errorCode);
}

BOOL CaretSelectionPos(const HWND hWnd, const CHARRANGE cr, const CHARRANGE oldCr)
{
	BOOL pos = 0;
	LONGLONG textLength = 0;
	LONGLONG lineCount = SendMessageW(hWnd, EM_GETLINECOUNT, NULL, NULL) - 1;

	textLength = SendMessageW(hWnd, WM_GETTEXTLENGTH, NULL, NULL) - lineCount;

	if (cr.cpMin == cr.cpMax)
	{
		pos = SELECTION_END;
	}

	if (cr.cpMin == oldCr.cpMin && cr.cpMax != oldCr.cpMax)
	{
		pos = SELECTION_END;
	}
	else
	{
		pos = SELECTION_START;
	}

	return pos;
}

CARET_POS CaretPos(const HWND hWnd, SELCHANGE* sc)
{
	wchar_t text[2];

	static CHARRANGE crPrev = { 0 };
	CHARRANGE cr = { 0 };

	LONG cursorChar = 0;
	LONG column = 0;
	LONGLONG line = 0;

	LONG selSize = 0;
	LONGLONG lineIndex = 0;

	LONGLONG length = 0;

	TEXTRANGEW tr = { 0 };
	CHARRANGE chrg = { 0 };

	CARET_POS pos = { 0 };

	cr = sc->chrg;

	if (cr.cpMax != 0)
	{
		chrg.cpMin = cr.cpMax - 1;
		chrg.cpMax = cr.cpMax;
	}
	else
	{
		chrg.cpMin = 0;
		chrg.cpMax = 1;
	}

	tr.chrg = chrg;
	tr.lpstrText = text;

	SendMessageW(hWnd, EM_GETTEXTRANGE, NULL, (LPARAM)&tr);

	length = SendMessageW(hWnd, WM_GETTEXTLENGTH, NULL, NULL);
	if (tr.lpstrText[0] == L'\0')
	{
		cr.cpMax = (length != 0) ? (cr.cpMax - 1) : 0;
		/*
		if (length != 0)
		{
		cr.cpMax--;
		}
		else
		{
		cr.cpMax = 0;
		}
		*/
	}

	if (CaretSelectionPos(hWnd, cr, crPrev) == SELECTION_START)
	{
		cursorChar = cr.cpMin;
	}
	else
	{
		cursorChar = cr.cpMax;
	}

	line = SendMessageW(hWnd, EM_LINEFROMCHAR, cursorChar, NULL);
	lineIndex = SendMessageW(hWnd, EM_LINEINDEX, line, NULL);

	pos.line = line + 1;
	pos.column = cursorChar - lineIndex + 1;

	crPrev = cr;

	return pos;
}

CARET_POS SelectAll(HWND hWnd)
{
	CARET_POS cp;
	CHARRANGE cr;
	cr.cpMin = 0;
	cr.cpMax = -1;
	LONGLONG sel = SendMessageW(hWnd, EM_EXSETSEL, NULL, (LPARAM)&cr);
	LONGLONG lineCount = SendMessageW(hWnd, EM_GETLINECOUNT, NULL, NULL);
	LONGLONG lineIndex = SendMessageW(hWnd, EM_LINEINDEX, lineCount - 1, NULL);
	cp.line = lineCount;
	cp.column = sel - lineIndex;
	return cp;
}

void WriteTimeDate(HWND hRichEdit)
{
	SYSTEMTIME st;
	GetLocalTime(&st);

	size_t dateBufSize = (size_t)GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_AUTOLAYOUT, &st, NULL, NULL, 0, NULL);
	size_t timeBufSize = (size_t)GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, NULL, &st, NULL, NULL, 0);

	wchar_t* dateStr = (wchar_t*)malloc((size_t)dateBufSize * 2);
	wchar_t* timeStr = (wchar_t*)malloc((size_t)timeBufSize * 2);
	wchar_t* timeDateStr = (wchar_t*)malloc((dateBufSize + timeBufSize) * 2);

	if (dateStr == NULL || timeStr == NULL || timeDateStr == NULL)
	{
		MessageBeep(MB_ICONERROR);
		return;
	}

	GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_AUTOLAYOUT, &st, NULL, dateStr, (int)dateBufSize, NULL);
	GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, NULL, &st, NULL, timeStr, (int)timeBufSize);

	timeStr[timeBufSize - 1] = L' ';

	memcpy(timeDateStr, timeStr, timeBufSize * 2);
	memcpy(timeDateStr + timeBufSize, dateStr, dateBufSize * 2);

	SendMessageW(hRichEdit, EM_REPLACESEL, TRUE, (LPARAM)timeDateStr);

	free(timeStr);
	free(dateStr);
	free(timeDateStr);
}

BOOL SetZoom(HWND hRichEdit, ULONGLONG numerator, LONGLONG denominator)
{
	return (BOOL)SendMessageW(hRichEdit, EM_SETZOOM, numerator, denominator);
}

void ChangeTitle(HWND hWnd, wchar_t* fileName, BOOL changed)
{
	size_t size = 0;
	
	#pragma warning (disable : 6054)
	if (changed)
	{
		size = wcslen(fileName) + wcslen(windowUnsavedTemplate) - 1;
	}
	else
	{
		size = wcslen(fileName) + wcslen(windowSavedTemplate) - 1;
	}
	#pragma warning (default : 6054)

	wchar_t* windowText = (wchar_t*)malloc(size * 2);

	if (windowText == NULL)
	{
		SetWindowText(hWnd, L"File - Memory allocation failed for file name");
		return;
	}

	if (changed)
	{
		wsprintf(windowText, windowUnsavedTemplate, fileName);
	}
	else
	{
		wsprintf(windowText, windowSavedTemplate, fileName);
	}
	SetWindowText(hWnd, windowText);
	windowText[size - 1] = L'\0';
	free(windowText);
}

void RemoveWord(HWND hRichEdit)
{
	CHARRANGE word = { 0 };
	LONG pos = 0;

	pos = CaretIndex(hRichEdit);

	#pragma warning (disable : 4244)
	word.cpMin = SendMessageW(hRichEdit, EM_FINDWORDBREAK, WB_MOVEWORDLEFT, pos);
	word.cpMax = pos;
	#pragma warning (default : 4244)

	SendMessageW(hRichEdit, EM_EXSETSEL, NULL, (LPARAM)&word);
	SendMessageW(hRichEdit, EM_REPLACESEL, TRUE, (LPARAM)L"");
}

LONG CaretIndex(HWND hRichEdit)
{
	SELCHANGE sc = { 0 };
	LONG pos = 0;

	SendMessageW(hRichEdit, EM_EXGETSEL, NULL, (LPARAM)&sc.chrg);
	pos = CaretSelectionPos(hRichEdit, sc.chrg, sc.chrg) == SELECTION_START ? sc.chrg.cpMin : sc.chrg.cpMax;

	return pos;
}

LONG GetSelectionSize(HWND hRichEdit)
{
	CHARRANGE cr = { 0 };

	SendMessageW(hRichEdit, EM_EXGETSEL, NULL, (LPARAM)&cr);

	return (cr.cpMax - cr.cpMin);
}

LONG GetSelectionSize(CHARRANGE cr)
{
	return (cr.cpMax - cr.cpMin);
}

void cf2lf(CHARFORMAT2W& cf, LOGFONT& lf)
{
	HDC hdc;
	LONG yPixPerInch;

	SecureZeroMemory(&lf, sizeof(LOGFONTW));

	hdc = GetDC(GetDesktopWindow());
	yPixPerInch = GetDeviceCaps(hdc, LOGPIXELSY);

	lf.lfHeight = abs((cf.yHeight*yPixPerInch)/(72*20));
	ReleaseDC(GetDesktopWindow(), hdc);

	if(cf.dwEffects & CFE_BOLD) lf.lfWeight = FW_BOLD;
	if(cf.dwEffects & CFE_ITALIC) lf.lfItalic = TRUE;
	if(cf.dwEffects & CFE_UNDERLINE) lf.lfUnderline = TRUE;
	if(cf.dwEffects & CFE_STRIKEOUT) lf.lfStrikeOut = TRUE;

	lf.lfCharSet = cf.bCharSet;
	lf.lfPitchAndFamily = cf.bPitchAndFamily;

	memcpy(lf.lfFaceName, cf.szFaceName, 32 * sizeof(wchar_t));
}

void lf2cf(LOGFONT& lf, CHARFORMAT2W& cf)
{
	cf.dwEffects = CFM_EFFECTS | CFE_AUTOBACKCOLOR;
	cf.dwEffects &= ~(CFE_PROTECTED | CFE_LINK| CFM_COLOR);

	HDC hdc;
	LONG yPixPerInch;

	cf.cbSize = sizeof(CHARFORMAT2W);
	hdc = GetDC(GetDesktopWindow());
	yPixPerInch = GetDeviceCaps(hdc, LOGPIXELSY);
	cf.yHeight = abs(lf.lfHeight*72*20/yPixPerInch);
	ReleaseDC(GetDesktopWindow(), hdc);

	cf.yOffset = 0;

	if(lf.lfWeight < FW_BOLD) cf.dwEffects &= ~CFE_BOLD;
	if(!lf.lfItalic) cf.dwEffects &= ~CFE_ITALIC;
	if(!lf.lfUnderline) cf.dwEffects &= ~CFE_UNDERLINE;
	if(!lf.lfStrikeOut) cf.dwEffects &= ~CFE_STRIKEOUT;

	cf.dwMask = CFM_ALL | CFM_BACKCOLOR;
	cf.bCharSet = lf.lfCharSet;
	cf.bPitchAndFamily = lf.lfPitchAndFamily;

	memcpy(cf.szFaceName, lf.lfFaceName, 32 * sizeof(wchar_t));
}
