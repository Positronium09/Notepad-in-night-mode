#pragma once

#include <windows.h>

//! sizeToRead in bytes
DWORD GetFileCallback(DWORD_PTR dwCookie, BYTE* buffer, LONG sizeToRead, LONG* read);
//! sizeToWrite in bytes
DWORD SaveFileCallback(DWORD_PTR dwCookie, BYTE* buffer, LONG sizeToRead, LONG* read);
