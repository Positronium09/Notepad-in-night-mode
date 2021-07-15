#include "file_ops.h"


#define _CheckSetFalse(change) if (change != NULL) (*change) = FALSE


#define _ErrorCheck(hWndOwner, errorCode) if (errorCode != 0 && errorCode != 183) Error(hWndOwner, errorCode)

const wchar_t defaultName[] = L"Untitled";

wchar_t* filePath = NULL;
wchar_t* fileName = NULL;


BOOL fileOpen = FALSE;

OPENFILENAMEW ofn;

BOOL _SaveChanges(HWND hWndOwner, HWND hRichEdit, BOOL* changed)
{
	int response = MessageBoxW(hWndOwner, L"Do you want to save the changes", L"File Unsaved", MB_YESNOCANCEL | MB_ICONQUESTION);
	if (response == IDYES)
	{
		if (fileOpen)
		{
			if (!Save(hWndOwner, hRichEdit, filePath, changed))
			{
				return FALSE;
			}
		}
		else
		{
			if (!SaveAs(hWndOwner, hRichEdit, changed))
			{
				return FALSE;
			}
		}
		return TRUE;
	}
	else if (response == IDNO)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void OnCreateFileOps()
{
	filePath = (wchar_t*)malloc((FILE_PATH_MAX + 1) * 2);
	fileName = (wchar_t*)malloc((FILE_TITLE_MAX + 1) * 2);

	if (filePath != NULL && fileName != NULL)
	{
		SecureZeroMemory(&ofn, sizeof(OPENFILENAMEW));
		SecureZeroMemory(filePath, (FILE_PATH_MAX + 1) * 2);
		SecureZeroMemory(fileName, (FILE_TITLE_MAX + 1) * 2);
		memcpy(fileName, defaultName, 9 * 2);
	}

	ofn.lStructSize = sizeof(OPENFILENAMEW);
	ofn.lpstrFile = filePath;
	ofn.nMaxFile = (FILE_PATH_MAX + 1);
	ofn.lpstrFileTitle = fileName;
	ofn.nMaxFileTitle = (FILE_TITLE_MAX + 1);
	ofn.lpstrFilter = L"All Files\0*.*\0Text files\0*.txt";
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
}

void OnDestroyFileOps()
{
	filePath[FILE_PATH_MAX] = L'\0';
	fileName[FILE_TITLE_MAX] = L'\0';
	free(filePath);
	free(fileName);
}

BOOL SaveChanges(HWND hWndOwner, HWND hRichEdit, BOOL* changed)
{
	return _SaveChanges(hWndOwner, hRichEdit, changed);
}

wchar_t* Save(HWND hWndOwner, HWND hRichEdit, wchar_t* file, BOOL* changed)
{
	if (!fileOpen)
	{
		SaveAs(hWndOwner, hRichEdit, changed);
		return fileName;
	}

	memcpy(filePath, file, (wcslen(file) + 1) * 2);
	#pragma warning (disable : 6054)
	HANDLE hFile = CreateFileW(filePath, GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	#pragma warning (default : 6054)

	if (hFile == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
		return NULL;
	}

	EDITSTREAM es;
	es.dwCookie = (DWORD_PTR)hFile;
	es.dwError = 0;
	es.pfnCallback = (EDITSTREAMCALLBACK)SaveFileCallback;
	SendMessageW(hRichEdit, EM_STREAMOUT, SF_TEXT | SF_UNICODE, (LPARAM)&es);

	_ErrorCheck(hWndOwner, es.dwError);

	CloseHandle(hFile);

	_CheckSetFalse(changed);

	wchar_t ext[FILE_TITLE_MAX + 1];

	_wsplitpath_s(filePath, NULL, NULL, NULL, NULL, fileName, ofn.nMaxFileTitle, ext, FILE_TITLE_MAX + 1);

	wcscat_s(fileName, FILE_TITLE_MAX + 1, ext);

	fileOpen = TRUE;

	return fileName;
}

wchar_t* Open(HWND hWndOwner, HWND hRichEdit, wchar_t* file, BOOL* changed)
{
	memcpy(filePath, file, (wcslen(file) + 1) * 2);
	#pragma warning (disable : 6054)
	HANDLE hFile = CreateFileW(filePath, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	#pragma warning (default : 6054)

	if (hFile == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
		return NULL;
	}

	EDITSTREAM es;
	es.dwCookie = (DWORD_PTR)hFile;
	es.dwError = 0;
	es.pfnCallback = (EDITSTREAMCALLBACK)GetFileCallback;
	SendMessageW(hRichEdit, EM_STREAMIN, SF_TEXT | SF_UNICODE, (LPARAM)&es);

	_ErrorCheck(hWndOwner, es.dwError);

	CloseHandle(hFile);

	_CheckSetFalse(changed);

	wchar_t ext[FILE_TITLE_MAX + 1];

	_wsplitpath_s(filePath, NULL, NULL, NULL, NULL, fileName, ofn.nMaxFileTitle, ext, FILE_TITLE_MAX + 1);

	wcscat_s(fileName, FILE_TITLE_MAX + 1, ext);

	fileOpen = TRUE;

	return fileName;
}

wchar_t* SaveAs(HWND hWndOwner, HWND hRichEdit, BOOL* changed)
{
	ofn.hwndOwner = hWndOwner;

	BOOL opened = GetSaveFileNameW(&ofn);

	if (!opened)
	{
		return NULL;
	}


	#pragma warning (disable : 6054)
	HANDLE hFile = CreateFileW(filePath, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	#pragma warning (default : 6054)

	CloseHandle(hFile);

	fileOpen = TRUE;

	return Save(hWndOwner, hRichEdit, filePath, changed);
}

wchar_t* GetFile(HWND hWndOwner, HWND hRichEdit, BOOL* changed)
{
	if ((*changed))
	if (!_SaveChanges(hWndOwner, hRichEdit, changed))
	{
		return NULL;
	}

	ofn.hwndOwner = hWndOwner;

	BOOL opened = GetOpenFileNameW(&ofn);

	if (!opened)
	{
		return NULL;
	}

	return Open(hWndOwner, hRichEdit, filePath, changed);
}

wchar_t* NewFile(HWND hWndOwner, HWND hRichEdit, BOOL* changed)
{
	if ((*changed))
	if (!_SaveChanges(hWndOwner, hRichEdit, changed))
	{
		return NULL;
	}

	SetWindowTextW(hRichEdit, L"");

	_CheckSetFalse(changed);

	fileOpen = FALSE;

	memcpy(fileName, defaultName, 9 * 2);

	return fileName;
}
