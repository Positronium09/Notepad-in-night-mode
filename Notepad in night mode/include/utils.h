#pragma once

#include <windows.h>
#include <richedit.h>

#define SELECTION_START FALSE
#define SELECTION_END TRUE


typedef struct _caretPos
{
	ULONGLONG line = 0;
	ULONGLONG column = 0;
} CARET_POS;


CARET_POS CaretPos(const HWND hWnd, SELCHANGE* sc);
LONG CaretIndex(HWND hRichEdit);

CARET_POS SelectAll(HWND hWnd);
void WriteTimeDate(HWND hRichEdit);
void ChangeTitle(HWND hWnd, wchar_t* fileName, BOOL changed);

BOOL SetZoom(HWND hRichEdit, ULONGLONG numerator, LONGLONG denominator);
void RemoveWord(HWND hRichEdit);

LONG GetSelectionSize(CHARRANGE cr);
LONG GetSelectionSize(HWND hRichEdit);

void Error(HWND hWndOwner, DWORD errorCode);

//? https://m.blog.naver.com/PostView.nhn?blogId=nawoo&logNo=80109999276&proxyReferer=https:%2F%2Fwww.google.com%2F
void lf2cf(LOGFONT& lf, CHARFORMAT2W& cf);
//? https://m.blog.naver.com/PostView.nhn?blogId=nawoo&logNo=80109999276&proxyReferer=https:%2F%2Fwww.google.com%2F
void cf2lf(CHARFORMAT2W& cf, LOGFONT& lf);
