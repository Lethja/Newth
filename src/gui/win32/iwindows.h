#ifndef NEW_TH_WINDOWS_H
#define NEW_TH_WINDOWS_H

#include <windows.h>

void iWindowBrowseFolder(const char *currentPath, char *returnPath, LPCSTR title);

WNDCLASSEX iWindowCreateClass(HINSTANCE hThisInstance, const TCHAR className[], WNDPROC windowProcedure);

BOOL CALLBACK iWindowSetSystemFontEnumerator(HWND hWnd, LPARAM lParam);

void iWindowSetupSystemFontStyle();

void iWindowTearDownSystemFontStyle();

#pragma region Font Management

extern HFONT g_hfDefault;
extern NONCLIENTMETRICS g_Ncm;

#pragma endregion

#endif /* NEW_TH_WINDOWS_H */
