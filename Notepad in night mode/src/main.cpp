#include <windows.h>
#include <richedit.h>
#include <richole.h>
#include <textserv.h>

#include "resource.h"
#include "colors.h"
#include "menu_defs.h"
#include "utils.h"
#include "file_ops.h"
#include "text_ops.h"


#pragma comment(lib, "Comctl32.lib")

//? https://docs.microsoft.com/en-us/windows/win32/controls/cookbook-overview#using-manifests-or-directives-to-ensure-that-visual-styles-can-be-applied-to-applications
#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"") // Enable visual styles

#define MAX_ZOOM 40960
#define MIN_ZOOM 64


HBRUSH defaultBackgroundBrush = CreateSolidBrush(defaultBackgroundColorref);

HWND hMainWindow = 0;
HWND hEditPlane = 0;
HWND hStatus = 0;
HWND hWndFindReplace = 0;

HINSTANCE hInst = 0;

HICON icon = 0;

HMENU hEditMenu = 0;

HMODULE hmodRichEdit = 0;

const wchar_t mainWindowClassName[] = L"MainWindowClass";
const wchar_t statusTemplate[] = L"Line %I64u  Column %I64u  |  %%%d";

const int statusHeight = 18;

BOOL textChanged = FALSE;

BOOL textPresent = FALSE;
BOOL selPresent = FALSE;
BOOL canUndo = FALSE;

ULONGLONG numerator = 640;
LONGLONG denominator = 640;
int zoomPercent = 100;
CARET_POS caretPos = { 1, 1 };

DWORD findFlags = DEFAULT_FIND_FLAGS;

UINT FIND_STRING = RegisterWindowMessageW(FINDMSGSTRINGW);

void OnCreate(HWND hWnd);
void OnDestroy();

void AddControls(HWND hWnd);
void AddMenus(HWND hWnd);

void SetupEdit();
void SetupStatusBar();

void ResizeControls(WORD width, WORD heigth);
void ResizeControls(HWND hWnd);

void UpdateStatus();
void UpdateMenus();


LRESULT CALLBACK MainWidnowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK RichEditWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDPROC OldRichEditProc = 0;


int wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR args, _In_ int cmdShow)
{
	hmodRichEdit = LoadLibraryW(L"Msftedit.dll");
	if (hmodRichEdit == NULL)
	{
		MessageBox(GetDesktopWindow(), L"Msftedit.dll cannot be loaded", L"Error", MB_OK);
		return 1;
	}

	WNDCLASSEXW mainWndClassEx = { 0 };

	icon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(ICON_MAIN), IMAGE_ICON, NULL, NULL, LR_DEFAULTCOLOR | LR_DEFAULTSIZE);

	mainWndClassEx.cbSize = sizeof(WNDCLASSEXW);
	mainWndClassEx.style = CS_HREDRAW | CS_VREDRAW;
	mainWndClassEx.hbrBackground = defaultBackgroundBrush;
	mainWndClassEx.hCursor = LoadCursorW(NULL, IDC_ARROW);
	mainWndClassEx.hInstance = hInstance;
	mainWndClassEx.lpszClassName = mainWindowClassName;
	mainWndClassEx.lpfnWndProc = MainWidnowProc;
	mainWndClassEx.hIcon = icon;

	RegisterClassExW(&mainWndClassEx);

	int argc = 0;

	wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	hMainWindow = CreateWindowW(mainWindowClassName, L"Untitled - Notepad in night mode", WS_VISIBLE | WS_OVERLAPPEDWINDOW, 100, 100, 1450, 770, NULL, NULL, NULL, NULL);

	ShowWindow(hMainWindow, cmdShow);

	MSG msg;
	HACCEL hAccel = 0;
	hInst = hInstance;

	hAccel = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(SHORT_CUTS));
	AddClipboardFormatListener(hMainWindow);

	if (argc > 1)
	{
		Open(hMainWindow, hEditPlane, argv[1], &textChanged);
		ChangeTitle(hMainWindow, fileName, textChanged);
	}

	LocalFree(argv);

	while (GetMessageW(&msg, NULL, NULL, NULL))
	{
		if (IsDialogMessageW(hWndFindReplace, &msg))
		{
			continue;
		}
		else if (!TranslateAcceleratorW(hMainWindow, hAccel, &msg))
		{
			TranslateMessage(&msg); 
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK MainWidnowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == FIND_STRING)
	{
		FINDREPLACEW* fr = (FINDREPLACEW*)lParam;
		BOOL wrapAround = FALSE;

		if (fr->Flags & FR_DIALOGTERM)
		{
			findFlags = fr->Flags & REMOVE_OTHER_FLAGS;
			return 0;
		}

		if (fr->Flags & FR_WRAPAROUND)
		{
			wrapAround = TRUE;
		}

		if (fr->Flags & FR_FINDNEXT)
		{
			findFlags = fr->Flags & REMOVE_OTHER_FLAGS;
			Find(hEditPlane, fr, NULL, wrapAround);
		}

		else if (fr->Flags & FR_REPLACE)
		{
			findFlags = fr->Flags & REMOVE_OTHER_FLAGS;
			Replace(hEditPlane, fr, NULL, wrapAround, FALSE);
		}

		else if (fr->Flags & FR_REPLACEALL)
		{
			findFlags = fr->Flags & REMOVE_OTHER_FLAGS;
			Replace(hEditPlane, fr, NULL, wrapAround, TRUE);
		}

		return 0;
	}

	switch (msg)
	{
		case WM_CREATE:
		{
			OnCreate(hWnd);
			return 0;
		}

		case WM_CLOSE:
		{
			if (textChanged)
			{
				if (SaveChanges(hWnd, hEditPlane, &textChanged))
				{
					DestroyWindow(hWnd);
				}
			}

			else
			{
				DestroyWindow(hWnd);
			}

			return 0;
		}

		case WM_DESTROY:
		{
			OnDestroy();
			return 0;
		}

		case WM_SIZE:
		{
			WORD width = LOWORD(lParam);
			WORD height = HIWORD(lParam);
			if (hWnd == hMainWindow)
			{
				ResizeControls(width, height);
			}
			return DefWindowProcW(hWnd, msg, wParam, lParam);
		}

		case WM_CLIPBOARDUPDATE:
		{
			UpdateMenus();
		}

		case WM_CTLCOLORSTATIC:
		{
			HDC hdc = (HDC)wParam;
			SetTextColor(hdc, defaultTextColorref);
			SetBkColor(hdc, defaultBackgroundColorref);
			return (LRESULT)defaultBackgroundBrush;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case SELECT_ALL:
				{
					caretPos = SelectAll(hEditPlane);
					UpdateStatus();
					break;
				}

				case NEW_FILE:
				{
					NewFile(hWnd, hEditPlane, &textChanged);

					ChangeTitle(hWnd, fileName, textChanged);
					UpdateMenus();
					break;
				}

				case EXIT_APP:
				{
					PostMessageW(hWnd, WM_CLOSE, NULL, NULL);
					break;
				}

				case OPEN_FILE:
				{
					GetFile(hWnd, hEditPlane, &textChanged);

					ChangeTitle(hWnd, fileName, textChanged);
					UpdateMenus();
					break;
				}

				case SAVE:
				{
					Save(hWnd, hEditPlane, filePath, &textChanged);

					ChangeTitle(hWnd, fileName, textChanged);
					UpdateMenus();
					break;
				}

				case SAVE_AS:
				{
					SaveAs(hWnd, hEditPlane, &textChanged);

					ChangeTitle(hWnd, fileName, textChanged);
					UpdateMenus();
					break;
				}

				case ZOOM_IN:
				{
					if (numerator == MAX_ZOOM)
					{
						return 0;
					}

					else if (numerator > MAX_ZOOM)
					{
						numerator = MAX_ZOOM;
					}
					numerator += 64;
					zoomPercent += 10;

					SetZoom(hEditPlane, numerator, denominator);
					UpdateStatus();
					break;
				}

				case ZOOM_OUT:
				{
					if (numerator == MIN_ZOOM)
					{
						return 0;
					}

					else if (numerator < MIN_ZOOM)
					{
						numerator = MIN_ZOOM;
					}

					numerator -= 64;
					zoomPercent -= 10;

					SetZoom(hEditPlane, numerator, denominator);
					UpdateStatus();
					break;
				}

				case RESET_ZOOM:
				{
					numerator = 640;
					zoomPercent = 100;
					SetZoom(hEditPlane, numerator, denominator);
					UpdateStatus();
					break;
				}

				case REMOVE_WORD:
				{
					RemoveWord(hEditPlane);
					UpdateMenus();
					break;
				}

				case CHANGE_FONT:
				{
					ChangeFont(hWnd, hEditPlane, hInst);
					break;
				}

				case CHANGE_COLOR:
				{
					ChangeTextColor(hWnd, hEditPlane);
					break;
				}

				case FIND_TEXT:
				{
					hWndFindReplace = FindTextDlgBox(hWnd, hEditPlane, findFlags, TRUE);
					UpdateMenus();
					break;
				}

				case REPLACE_TEXT:
				{
					hWndFindReplace = ReplaceTextDlgBox(hWnd, hEditPlane, findFlags, TRUE);
					UpdateMenus();
					break;
				}

				case UNDO:
				{
					SendMessageW(hEditPlane, EM_UNDO, NULL, NULL);
					break;
				}

				case CUT:
				{
					SendMessageW(hEditPlane, WM_CUT, NULL, NULL);
					break;
				}

				case PASTE:
				{
					SendMessageW(hEditPlane, WM_PASTE, NULL, NULL);
					break;
				}

				case COPY:
				{
					SendMessageW(hEditPlane, WM_COPY, NULL, NULL);
					break;
				}

				case DATE_TIME:
				{
					WriteTimeDate(hEditPlane);
					break;
				}

				case FIND_NEXT:
				{
					CHARRANGE cr = { 0 };
					//! Selection size in BYTES
					LONG selSize = GetSelectionSize(hEditPlane);

					wchar_t* text = (wchar_t*)malloc(((size_t)selSize + 1) * 2);

					if (text == NULL)
					{
						MessageBeep(MB_ICONERROR);
						break;
					}

					SendMessageW(hEditPlane, EM_GETSELTEXT, NULL, (LPARAM)text);

					text[selSize] = L'\0';

					BOOL A = Search(hEditPlane, text, FR_DOWN, &cr, TRUE);

					free(text);

					SendMessageW(hEditPlane, EM_EXSETSEL, NULL, (LPARAM)&cr);

					break;
				}

				case FIND_PREV:
				{
					CHARRANGE cr = { 0 };
					//! Selection size in BYTES
					LONG selSize = GetSelectionSize(hEditPlane);

					wchar_t* text = (wchar_t*)malloc(((size_t)selSize + 1) * 2);

					if (text == NULL)
					{
						MessageBeep(MB_ICONERROR);
						break;
					}

					SendMessageW(hEditPlane, EM_GETSELTEXT, NULL, (LPARAM)text);

					text[selSize] = L'\0';

					Search(hEditPlane, text, NULL, &cr, TRUE);

					free(text);

					SendMessageW(hEditPlane, EM_EXSETSEL, NULL, (LPARAM)&cr);

					break;
				}

				#ifdef _DEBUG
				case TEST: // For easy testing not compiled if not in debug mode
				{
					MessageBeep(MB_ICONERROR);
					break;
				}
				#endif
				default:
				{
					break;
				}
			}
			switch (HIWORD(wParam))
			{
				case EN_CHANGE:
				{
					if (!textChanged)
					{
						textChanged = TRUE;
					}

					if (SendMessageW(hEditPlane, WM_GETTEXTLENGTH, NULL, NULL))
					{
						textPresent = TRUE;
					}
					else
					{
						textPresent = FALSE;
					}

					ChangeTitle(hWnd, fileName, textChanged);
					UpdateMenus();
					break;
				}

				default:
				{
					break;
				}

			}
			break;
		}

		case WM_NOTIFY:
		{
			NMHDR* nm = (NMHDR*)lParam;
			if (nm->hwndFrom == hEditPlane)
			{
				switch (nm->code)
				{
					case EN_SELCHANGE:
					{
						SELCHANGE* sc = (SELCHANGE*)lParam;
						caretPos = CaretPos(hEditPlane, sc);

						if (GetSelectionSize(hEditPlane) == 0)
						{
							selPresent = FALSE;
						}
						else
						{
							selPresent = TRUE;
						}

						UpdateStatus();
						UpdateMenus();
						break;
					}

					case EN_REQUESTRESIZE:
					{
						RECT mainRect;
						LONG width = 0;
						LONG height = 0;

						GetClientRect(hWnd, &mainRect);

						width = mainRect.right;
						height = mainRect.bottom;
						GetClientRect(hWnd, &mainRect);

						SetWindowPos(hStatus, HWND_TOP, 0,  height - statusHeight, width, statusHeight, SWP_SHOWWINDOW);
						break;
					}
				}
			}
		}

		default:
		{
			return DefWindowProcW(hWnd, msg, wParam, lParam);
		}
	}
	return 0;
}


void OnDestroy()
{
	OnDestroyFileOps();
	OnDestroyTextOps();
	DestroyIcon(icon);
	DeleteObject(defaultBackgroundBrush);
	PostQuitMessage(0);
}

void OnCreate(HWND hWnd)
{
	OnCreateFileOps();
	OnCreateTextOps();
	AddControls(hWnd);
	AddMenus(hWnd);
	UpdateStatus();
	UpdateMenus();
	SetupEdit();
	SetupStatusBar();
	ResizeControls(hWnd);
}

void SetupEdit()
{
	IID* IID_ITS = (IID*)GetProcAddress(hmodRichEdit, "IID_ITextServices");

	IRichEditOle* richEditOle = NULL;

	SendMessageW(hEditPlane, EM_GETOLEINTERFACE, 0, (LPARAM)&richEditOle);

	ITextServices* pTxtSrv = NULL;

	richEditOle->QueryInterface(*IID_ITS, (void**)&pTxtSrv);

	richEditOle->Release();

	pTxtSrv->OnTxPropertyBitsChange(TXTBIT_ALLOWBEEP, 0);
	pTxtSrv->Release();

	SendMessage(hEditPlane, EM_SHOWSCROLLBAR, SB_VERT, TRUE);
	SendMessage(hEditPlane, EM_SHOWSCROLLBAR, SB_HORZ, TRUE);

	SendMessageW(hEditPlane, EM_SETBKGNDCOLOR, FALSE, defaultBackgroundColorref);

	CHARFORMATW cf;
	SecureZeroMemory(&cf, sizeof(CHARFORMATW));

	cf.cbSize = sizeof(CHARFORMATW);
	cf.crTextColor = defaultTextColorref;
	cf.dwMask = CFM_COLOR;

	SendMessageW(hEditPlane, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);

	LONGLONG langOps = SendMessageW(hEditPlane, EM_GETLANGOPTIONS, NULL, NULL);
	langOps &= ~IMF_AUTOFONT;
	SendMessageW(hEditPlane, EM_SETLANGOPTIONS, NULL, (LPARAM)langOps);

	OldRichEditProc = (WNDPROC)SetWindowLongPtrW(hEditPlane, GWLP_WNDPROC, (LONG_PTR)RichEditWindowProc);
}

void SetupStatusBar()
{
	HFONT font = (HFONT)SendMessageW(hEditPlane, WM_GETFONT, NULL, NULL);

	SendMessageW(hStatus, WM_SETFONT, (WPARAM)font, TRUE);
}

void AddControls(HWND hWnd)
{
	hEditPlane = CreateWindowW(MSFTEDIT_CLASS, NULL, ES_MULTILINE | WS_VISIBLE | WS_CHILD | ES_NOHIDESEL | ES_AUTOVSCROLL | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, NULL, NULL, NULL);
	hStatus = CreateWindowW(L"static", L"Line 1  Column 1  |  %100", WS_VISIBLE | WS_CHILD | WS_BORDER, 0, 0, 0, 0, hWnd, NULL, NULL, NULL);

	SendMessageW(hEditPlane, EM_SETEVENTMASK, NULL, ENM_SELCHANGE | ENM_CHANGE | ENM_REQUESTRESIZE);
}

void AddMenus(HWND hWnd)
{
	HMENU hMenu = CreateMenu();
	HMENU hFileMenu = CreateMenu();
	HMENU hZoomMenu = CreateMenu();
	HMENU hViewMenu = CreateMenu();
	hEditMenu = CreateMenu();


	#pragma region FILE_MENU
	AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"File");

	AppendMenuW(hFileMenu, MF_STRING, NEW_FILE, L"New");
	AppendMenuW(hFileMenu, MF_STRING, OPEN_FILE, L"Open");
	AppendMenuW(hFileMenu, MF_STRING, SAVE, L"Save");
	AppendMenuW(hFileMenu, MF_STRING, SAVE_AS, L"Save as");

	AppendMenuW(hFileMenu, MF_SEPARATOR, NULL, NULL);

	AppendMenuW(hFileMenu, MF_STRING, EXIT_APP, L"Exit");
	#pragma endregion

	#pragma region EDIT_MENU
	AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hEditMenu, L"Edit");

	AppendMenuW(hEditMenu, MF_STRING | MF_GRAYED, UNDO, L"Undo");
	AppendMenuW(hEditMenu, MF_STRING | MF_GRAYED, CUT, L"Cut");
	AppendMenuW(hEditMenu, MF_STRING | MF_GRAYED, COPY, L"Copy");
	AppendMenuW(hEditMenu, MF_STRING | MF_GRAYED, PASTE, L"Paste");

	AppendMenuW(hEditMenu, MF_SEPARATOR, NULL, NULL);

	AppendMenuW(hEditMenu, MF_STRING | MF_GRAYED, FIND_TEXT, L"Find");
	AppendMenuW(hEditMenu, MF_STRING | MF_GRAYED, FIND_NEXT, L"Find Next");
	AppendMenuW(hEditMenu, MF_STRING | MF_GRAYED, FIND_PREV, L"Find Prev");
	AppendMenuW(hEditMenu, MF_STRING, REPLACE_TEXT, L"Replace");

	AppendMenuW(hEditMenu, MF_SEPARATOR, NULL, NULL);

	AppendMenuW(hEditMenu, MF_STRING, SELECT_ALL, L"Select All");
	AppendMenuW(hEditMenu, MF_STRING, DATE_TIME, L"Time / Date");
	#pragma endregion

	#pragma region VIEW_MENU
	AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hViewMenu, L"View");

	AppendMenuW(hViewMenu, MF_STRING, CHANGE_FONT, L"Change Font");
	AppendMenuW(hViewMenu, MF_STRING, CHANGE_COLOR, L"Change Text Color");

	AppendMenuW(hViewMenu, MF_SEPARATOR, NULL, NULL);

	#pragma region ZOOM_MENU__VIEW_MENU
	AppendMenuW(hViewMenu, MF_POPUP, (UINT_PTR)hZoomMenu, L"Zoom");	

	AppendMenuW(hZoomMenu, MF_STRING, ZOOM_IN, L"Zoom In");
	AppendMenuW(hZoomMenu, MF_STRING, ZOOM_OUT, L"Zoom Out");
	AppendMenuW(hZoomMenu, MF_STRING, RESET_ZOOM, L"Zoom Reset");
	#pragma endregion
	#pragma endregion

	#ifdef _DEBUG
	AppendMenuW(hMenu, MF_STRING, TEST, L"Test"); // For easy testing not compiled if not in debug mode
	#endif

	SetMenu(hWnd, hMenu);
}

void ResizeControls(HWND hWnd)
{
	RECT mainRect;
	LONG width = 0;
	LONG height = 0;

	GetClientRect(hWnd, &mainRect);

	width = mainRect.right;
	height = mainRect.bottom;

	SendMessageW(hEditPlane, EM_REQUESTRESIZE, NULL, NULL);
	SetWindowPos(hStatus, HWND_TOP, 0,  height - statusHeight, width, statusHeight, SWP_SHOWWINDOW);
	SetWindowPos(hEditPlane, hStatus, 0, 0, width, height - statusHeight, SWP_SHOWWINDOW);
}

void ResizeControls(WORD width, WORD height)
{
	SendMessageW(hEditPlane, EM_REQUESTRESIZE, NULL, NULL);
	SetWindowPos(hStatus, HWND_TOP, 0,  height - statusHeight, width, statusHeight, SWP_SHOWWINDOW);
	SetWindowPos(hEditPlane, hStatus, 0, 0, width, height - statusHeight, SWP_SHOWWINDOW);
}

void UpdateStatus()
{
	wchar_t text[50];
	wsprintf(text, statusTemplate, caretPos.line, caretPos.column, zoomPercent);
	SetWindowTextW(hStatus, text);
}

void UpdateMenus()
{
	if (SendMessageW(hEditPlane, EM_CANPASTE, CF_UNICODETEXT | CF_TEXT, NULL))
	{
		EnableMenuItem(hEditMenu, PASTE, MF_ENABLED);
	}
	else
	{
		EnableMenuItem(hEditMenu, PASTE, MF_GRAYED);
	}

	if (GetSelectionSize(hEditPlane) != 0)
	{
		EnableMenuItem(hEditMenu, CUT, MF_ENABLED);
		EnableMenuItem(hEditMenu, COPY, MF_ENABLED);

		EnableMenuItem(hEditMenu, FIND_NEXT, MF_ENABLED);
		EnableMenuItem(hEditMenu, FIND_PREV, MF_ENABLED);
	}
	else
	{
		EnableMenuItem(hEditMenu, CUT, MF_GRAYED);
		EnableMenuItem(hEditMenu, COPY, MF_GRAYED);

		EnableMenuItem(hEditMenu, FIND_NEXT, MF_GRAYED);
		EnableMenuItem(hEditMenu, FIND_PREV, MF_GRAYED);
	}

	if (SendMessageW(hEditPlane, EM_CANUNDO, NULL, NULL))
	{
		EnableMenuItem(hEditMenu, UNDO, MF_ENABLED);
	}
	else
	{
		EnableMenuItem(hEditMenu, UNDO, MF_GRAYED);
	}

	if (SendMessageW(hEditPlane, WM_GETTEXTLENGTH, NULL, NULL))
	{
		EnableMenuItem(hEditMenu, FIND_TEXT, MF_ENABLED);
	}
	else
	{
		EnableMenuItem(hEditMenu, FIND_TEXT, MF_GRAYED);
	}
}

LRESULT CALLBACK RichEditWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_MOUSEWHEEL:
		{
			if (wParam & MK_CONTROL)
			{
				short delta = GET_WHEEL_DELTA_WPARAM(wParam);
				if (delta < 0)
				{
					WPARAM wparam = MAKEWPARAM(ZOOM_OUT, NULL);
					SendMessageW(hMainWindow, WM_COMMAND, wparam, NULL);
					return 0;
				}
				else if (delta > 0)
				{
					WPARAM wparam = MAKEWPARAM(ZOOM_IN, NULL);
					SendMessageW(hMainWindow, WM_COMMAND, wparam, NULL);
					return 0;
				}
				return 0;
			}
			else
			{
				short delta = GET_WHEEL_DELTA_WPARAM(wParam);
				if (delta < 0)
				{
					SendMessageW(hEditPlane, WM_VSCROLL, SB_LINEDOWN, NULL);
					return 0;
				}
				else if (delta > 0)
				{
					SendMessageW(hEditPlane, WM_VSCROLL, SB_LINEUP, NULL);
					return 0;
				}
			}
			return 1;
		}

		default:
		{
			return CallWindowProcW(OldRichEditProc, hWnd, msg, wParam, lParam);
		}
	}
	return CallWindowProcW(OldRichEditProc, hWnd, msg, wParam, lParam);
}
