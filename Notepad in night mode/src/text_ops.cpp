#include "text_ops.h"

#include "resource.h"
#include "colors.h"


COLORREF textColor = defaultTextColorref;

FINDREPLACEW fr = { 0 };
wchar_t findText[MAX_FIND_REPLACE_LENGTH + 1];
wchar_t replaceText[MAX_FIND_REPLACE_LENGTH + 1];

extern HWND hMainWindow;
extern UINT FIND_STRING;

UINT_PTR FindHookProc(HWND, UINT, WPARAM, LPARAM);

void OnCreateTextOps()
{
	SecureZeroMemory(findText, (MAX_FIND_REPLACE_LENGTH + 1) * 2);
	fr.lStructSize = sizeof(FINDREPLACEW);
	fr.lpstrFindWhat = findText;
	fr.lpstrReplaceWith = replaceText;
	fr.wFindWhatLen = MAX_FIND_REPLACE_LENGTH * sizeof(wchar_t);
	fr.wReplaceWithLen = MAX_FIND_REPLACE_LENGTH * sizeof(wchar_t);

	return;
}

void OnDestroyTextOps()
{
	return;
}

void ChangeFont(HWND hWndOwner, HWND hRichEdit, HINSTANCE hInstance)
{
	CHOOSEFONTW cf;
	LOGFONTW lf;
	CHARFORMAT2W cfw;
	SecureZeroMemory(&cf, sizeof(CHOOSEFONTW));
	SecureZeroMemory(&lf, sizeof(LOGFONTW));
	SecureZeroMemory(&cfw, sizeof(CHARFORMAT2W));

	cfw.cbSize = sizeof(CHARFORMAT2W);
	cfw.dwMask = CFM_ALL | CFM_BACKCOLOR;

	SendMessageW(hRichEdit, EM_GETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cfw);

	cf2lf(cfw, lf);

	cf.lStructSize = sizeof(CHOOSEFONTW);
	cf.hwndOwner = hWndOwner;
	cf.hInstance = hInstance;
	cf.lpLogFont = &lf;
	cf.Flags = CF_EFFECTS | CF_ENABLETEMPLATE | CF_FORCEFONTEXIST | CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
	cf.lpTemplateName = MAKEINTRESOURCEW(FONT_DLG);

	if (ChooseFontW(&cf))
	{
		DWORD error = CommDlgExtendedError();

		if (!error)
		{
			cfw.crTextColor = textColor;
			cfw.dwMask = CFM_COLOR;

			lf2cf(lf, cfw);
			SendMessageW(hRichEdit, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cfw);
		}

		else
		{
			Error(hWndOwner, error);
		}
	}
	return;
}

void ChangeTextColor(HWND hWndOwner, HWND hRichEdit)
{
	static COLORREF customColors[16] = {
	WHITE, WHITE, WHITE, WHITE, 
	WHITE, WHITE, WHITE, WHITE, 
	WHITE, WHITE, WHITE, WHITE, 
	WHITE, WHITE, WHITE, WHITE
	};

	CHOOSECOLORW cc;
	SecureZeroMemory(&cc, sizeof(CHOOSECOLORW));

	cc.lStructSize = sizeof(CHOOSECOLORW);
	cc.rgbResult = textColor;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;
	cc.lpCustColors = customColors;

	if (ChooseColor(&cc))
	{
		DWORD error = CommDlgExtendedError();

		if (!error)
		{
			textColor = cc.rgbResult;

			CHARFORMATW cf;
			SecureZeroMemory(&cf, sizeof(CHARFORMATW));
			cf.cbSize = sizeof(CHARFORMATW);
			cf.dwMask = CFM_COLOR;
			cf.crTextColor = textColor;
			SendMessageW(hRichEdit, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);
		}

		else
		{
			Error(hWndOwner, error);
		}
	}
}

HWND FindTextDlgBox(HWND hWndOwner, HWND hRichEdit, DWORD flags, BOOL showSelected)
{
	fr.Flags = flags;
	HWND _return = 0;

	size_t textLength = 0;
	wchar_t* selText = NULL;

	if (showSelected)
	{
		textLength = GetSelectionSize(hRichEdit);
		if (textLength > MAX_FIND_REPLACE_LENGTH)
		{
			CHARRANGE cr;
			SendMessageW(hRichEdit, EM_EXGETSEL, NULL, (LPARAM)&cr);
			cr.cpMax = MAX_FIND_REPLACE_LENGTH;
			SendMessageW(hRichEdit, EM_EXSETSEL, NULL, (LPARAM)&cr);
		}
		selText = (wchar_t*)malloc((MAX_FIND_REPLACE_LENGTH + 1) * 2);

		SendMessageW(hRichEdit, EM_GETSELTEXT, NULL, (LPARAM)selText);
	}

	_return = CreateFindDialogBox(hWndOwner, selText, textLength);

	if (showSelected)
	{
		free(selText);
	}

	return _return;
}

HWND ReplaceTextDlgBox(HWND hWndOwner, HWND hRichEdit, DWORD flags, BOOL showSelected)
{
	fr.Flags = flags;
	HWND _return = 0;

	size_t textLength = 0;
	wchar_t* selText = NULL;

	if (showSelected)
	{
		textLength = GetSelectionSize(hRichEdit);
		if (textLength > MAX_FIND_REPLACE_LENGTH)
		{
			CHARRANGE cr;
			SendMessageW(hRichEdit, EM_EXGETSEL, NULL, (LPARAM)&cr);
			cr.cpMax = MAX_FIND_REPLACE_LENGTH;
			SendMessageW(hRichEdit, EM_EXSETSEL, NULL, (LPARAM)&cr);
		}
		selText = (wchar_t*)malloc((MAX_FIND_REPLACE_LENGTH + 1) * 2);

		SendMessageW(hRichEdit, EM_GETSELTEXT, NULL, (LPARAM)selText);
	}

	_return = CreateReplaceDialogBox(hWndOwner, selText, textLength);

	if (showSelected)
	{
		free(selText);
	}

	return _return;
}

HWND CreateFindDialogBox(HWND hWndOwner, const wchar_t* defaultText, size_t defaultTextSize)
{
	fr.hwndOwner = hWndOwner;

	if (defaultText != NULL)
	{
		size_t textSize = 0;
		if (defaultTextSize == 0)
		{
			textSize = wcslen(defaultText);
		}
		else
		{
			textSize = defaultTextSize;
		}

		memcpy(fr.lpstrFindWhat, defaultText, min(MAX_FIND_REPLACE_LENGTH * 2, (textSize + 1) * 2));
	}
	
	HWND hWndDlg = FindTextW(&fr);

	HWND findEdit = GetDlgItem(hWndDlg, edt1);
	SendMessageW(findEdit, EM_SETLIMITTEXT, MAX_FIND_REPLACE_LENGTH, NULL);

	return hWndDlg;
}

HWND CreateReplaceDialogBox(HWND hWndOwner, const wchar_t* defaultText, size_t defaultTextSize)
{
	fr.hwndOwner = hWndOwner;

	if (defaultText != NULL)
	{
		size_t textSize = 0;
		if (defaultTextSize == 0)
		{
			textSize = wcslen(defaultText);
		}
		else
		{
			textSize = defaultTextSize;
		}

		memcpy(fr.lpstrFindWhat, defaultText, min(MAX_FIND_REPLACE_LENGTH * 2, (textSize + 1) * 2));
	}

	HWND hWndDlg = ReplaceTextW(&fr);
	DWORD error = CommDlgExtendedError();

	HWND findEdit = GetDlgItem(hWndDlg, edt1);
	HWND replaceEdit = GetDlgItem(hWndDlg, edt2);

	SendMessageW(findEdit, EM_SETLIMITTEXT, MAX_FIND_REPLACE_LENGTH, NULL);
	SendMessageW(replaceEdit, EM_SETLIMITTEXT, MAX_FIND_REPLACE_LENGTH, NULL);

	return hWndDlg;
}

BOOL _Search(HWND hRichEdit, wchar_t* text, DWORD flags, CHARRANGE* cr, BOOL wrapAround)
{
	FINDTEXTEXW ft = { 0 };
	BOOL found = FALSE;
	CHARRANGE currentSel = { 0 };

	SendMessageW(hRichEdit, EM_EXGETSEL, NULL, (LPARAM)&currentSel);

	if (flags & FR_DOWN)
	{
		ft.chrg.cpMax = -1;
		ft.chrg.cpMin = currentSel.cpMax;
	}

	else
	{
		ft.chrg.cpMax = 0;
		ft.chrg.cpMin = currentSel.cpMin;
	}

	ft.lpstrText = text;

	LONGLONG startIndex = SendMessageW(hRichEdit, EM_FINDTEXTEXW, flags, (LPARAM)&ft);

	if (startIndex != -1)
	{
		found = TRUE;
	}
	else if (wrapAround)
	{
		if (flags & FR_DOWN)
		{
			ft.chrg.cpMax = -1;
			ft.chrg.cpMin = 0;
			LONGLONG startIndex = SendMessageW(hRichEdit, EM_FINDTEXTEXW, flags, (LPARAM)&ft);
			if (startIndex != -1)
			{
				found = TRUE;
			}
		}

		else
		{
			ft.chrg.cpMax = 0;
			ft.chrg.cpMin = (LONG)SendMessageW(hRichEdit, WM_GETTEXTLENGTH, NULL, NULL);
			LONGLONG startIndex = SendMessageW(hRichEdit, EM_FINDTEXTEXW, flags, (LPARAM)&ft);
			if (startIndex != -1)
			{
				found = TRUE;
			}
		}
	}

	if (found)
	{
		if (cr != NULL)
		{
			memcpy(cr, &ft.chrgText, sizeof(CHARRANGE));
		}

		return TRUE;
	}

	return FALSE;
}

BOOL Search(HWND hRichEdit, wchar_t* text, DWORD flags, CHARRANGE* cr, BOOL wrapAround)
{
	return _Search(hRichEdit, text, flags, cr, wrapAround);
}

BOOL Find(HWND hRichEdit, FINDREPLACEW* fr, CHARRANGE* cr, BOOL wrapAround)
{
	CHARRANGE _cr = { 0 };
	FINDTEXTEXW ft = { 0 };
	DWORD flags = 0;

	if (fr->Flags & FR_DOWN)
	{
		flags |= FR_DOWN;
	}

	if (fr->Flags & FR_MATCHCASE)
	{
		flags |= FR_MATCHCASE;
	}

	if (fr->Flags & FR_WHOLEWORD)
	{
		flags |= FR_WHOLEWORD;
	}

	if (_Search(hRichEdit, fr->lpstrFindWhat, flags, &_cr, wrapAround))
	{
		SendMessageW(hRichEdit, EM_EXSETSEL, NULL, (LPARAM)&_cr);
		if (cr != NULL)
		{
			memcpy(&_cr, cr, sizeof(CHARRANGE));
		}

		return TRUE;
	}

	return FALSE;
}

BOOL Replace(HWND hRichEdit, FINDREPLACEW* fr, CHARRANGE* cr, BOOL wrapAround, BOOL replaceAll)
{
	CHARRANGE _cr = { 0 };
	FINDTEXTEXW ft = { 0 };
	DWORD flags = FR_DOWN;

	if (fr->Flags & FR_MATCHCASE)
	{
		flags |= FR_MATCHCASE;
	}

	if (fr->Flags & FR_WHOLEWORD)
	{
		flags |= FR_WHOLEWORD;
	}

	if (replaceAll)
	{
		SendMessageW(hRichEdit, EM_EXSETSEL, NULL, (LPARAM)&_cr);
		while (_Search(hRichEdit, fr->lpstrFindWhat, flags, &_cr, FALSE))
		{
			SendMessageW(hRichEdit, EM_EXSETSEL, NULL, (LPARAM)&_cr);
			SendMessageW(hRichEdit, EM_REPLACESEL, TRUE, (LPARAM)fr->lpstrReplaceWith);
		}
		if (cr != NULL)
		{
			memcpy(&_cr, cr, sizeof(CHARRANGE));
		}
		return TRUE;
	}
	else
	{
		if (_Search(hRichEdit, fr->lpstrFindWhat, flags, &_cr, wrapAround))
		{
			SendMessageW(hRichEdit, EM_EXSETSEL, NULL, (LPARAM)&_cr);
			SendMessageW(hRichEdit, EM_REPLACESEL, TRUE, (LPARAM)fr->lpstrReplaceWith);
		}
		if (cr != NULL)
		{
			memcpy(&_cr, cr, sizeof(CHARRANGE));
		}
		return TRUE;
	}

	return FALSE;
}
