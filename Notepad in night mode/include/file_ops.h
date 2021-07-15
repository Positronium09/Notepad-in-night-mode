#pragma once

#include <windows.h>

#include "stream_callbacks.h"
#include "utils.h"

//! Size in CHARS
#define FILE_PATH_MAX 1000
//! Size in CHARS
#define FILE_TITLE_MAX 500


extern wchar_t* fileName;
extern wchar_t* filePath;
extern BOOL fileOpen;
extern const wchar_t defaultName[9];


BOOL SaveChanges(HWND hWndOwner, HWND hRichEdit, BOOL* changed);

wchar_t* Open(HWND hWndOwner, HWND hRichEdit, wchar_t* file, BOOL* changed);
wchar_t* Save(HWND hWndOwner, HWND hRichEdit, wchar_t* file, BOOL* changed);

wchar_t* GetFile(HWND hWndOwner, HWND hRichEdit, BOOL* changed);
wchar_t* SaveAs(HWND hWndOwner, HWND hRichEdit, BOOL* changed);
wchar_t* NewFile(HWND hWndOwner, HWND hRichEdit, BOOL* changed);

void OnCreateFileOps();
void OnDestroyFileOps();
