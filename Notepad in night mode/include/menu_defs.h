#pragma once

#ifdef _DEBUG
#define TEST 0xffff // For easy testing not compiled if not in debug mode
#endif

#define TEXT_MENU 0b11000000
#define GENERAL 0b10000000
#define CHANGE_FONT (TEXT_MENU | 1)
#define CHANGE_COLOR (TEXT_MENU | 2)
#define EXIT_APP (GENERAL | 1)
