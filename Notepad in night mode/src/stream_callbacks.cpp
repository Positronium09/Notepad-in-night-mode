#include "stream_callbacks.h"


DWORD GetFileCallback(DWORD_PTR dwCookie, BYTE* buffer, LONG sizeToRead, LONG* read)
{
	HANDLE hFile = (HANDLE)dwCookie;

	char* fileData = (char*)malloc((size_t)sizeToRead);

	if (fileData == NULL)
	{
		return -1;
	}

	SecureZeroMemory(fileData, sizeToRead);

	// cast to void to avoid 6031
	(void)ReadFile(hFile, fileData, sizeToRead, (DWORD*)read, NULL);

	if ((*read) == 0)
	{
		free(fileData);
		return 0;
	}

	char* _temp = (char*)realloc(fileData, (size_t)(*read));

	if (_temp == NULL)
	{
		return -1;
	}

	fileData = _temp;

	int mask = IS_TEXT_UNICODE_UNICODE_MASK;

	if (IsTextUnicode(fileData, (*read), &mask))
	{
		memcpy(buffer, fileData, (*read));
	}

	else
	{
		//! Buffer size in WIDE CHARS
		int bufSize = MultiByteToWideChar(CP_UTF8, NULL, fileData, (*read), NULL, 0);

		wchar_t* wText = (wchar_t*)malloc((size_t)bufSize * 2);

		if (wText == NULL)
		{
			return -1;
		}

		MultiByteToWideChar(CP_UTF8, NULL, fileData, (*read), wText, bufSize);
		memcpy(buffer, wText, (size_t)bufSize * 2);

		(*read) = bufSize * 2;

		free(wText);
	}

	free(fileData);

	DWORD error = GetLastError();

	return error;
}

DWORD SaveFileCallback(DWORD_PTR dwCookie, BYTE* buffer, LONG sizeToWrite, LONG* written)
{
	HANDLE hFile = (HANDLE)dwCookie;

	//! Buffer size in BYTES
	int bufSize = WideCharToMultiByte(CP_UTF8, NULL, (wchar_t*)buffer, (sizeToWrite / 2), NULL, 0, NULL, NULL);

	char* multiText = (char*)malloc((size_t)bufSize);

	if (multiText == NULL)
	{
		return -1;
	}

	WideCharToMultiByte(CP_UTF8, NULL, (wchar_t*)buffer, (sizeToWrite / 2), multiText, bufSize, NULL, NULL);
	memcpy(buffer, multiText, (size_t)bufSize * 2);

	free(multiText);

	WriteFile(hFile, buffer, bufSize, (DWORD*)written, NULL);

	DWORD error = GetLastError();

	(*written) = sizeToWrite; // Set written to sizeToWrite to avoid error

	return error;
}
