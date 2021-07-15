#pragma once

#include <windows.h>

#include "utils.h"

#define MAX_FIND_REPLACE_LENGTH 100
#define DEFAULT_FIND_FLAGS (FR_DOWN | FR_MATCHCASE | FR_SHOWWRAPAROUND | FR_WRAPAROUND)
#define REMOVE_OTHER_FLAGS (~(FR_DIALOGTERM | FR_FINDNEXT | FR_REPLACE | FR_REPLACEALL))


void OnCreateTextOps();
void OnDestroyTextOps();

void ChangeFont(HWND hWndOwner, HWND hRichEdit, HINSTANCE hInstance);
void ChangeTextColor(HWND hWndOwner, HWND hRichEdit);
HWND CreateFindDialogBox(HWND hWndOwner, const wchar_t* defaultText, size_t defaultTextSize);
HWND CreateReplaceDialogBox(HWND hWndOwner, const wchar_t* defaultText, size_t defaultTextSize);
BOOL Search(HWND hRichEdit, wchar_t* text, DWORD flags, CHARRANGE* cr, BOOL wrapAround);
BOOL Find(HWND hRichEdit, FINDREPLACEW* fr, CHARRANGE* cr, BOOL wrapAround);
BOOL Replace(HWND hRichEdit, FINDREPLACEW* fr, CHARRANGE* cr, BOOL wrapAround, BOOL replaceAll);
HWND FindTextDlgBox(HWND hWndOwner, HWND hRichEdit, DWORD flags, BOOL showSelected);
HWND ReplaceTextDlgBox(HWND hWndOwner, HWND hRichEdit, DWORD flags, BOOL showSelected);
