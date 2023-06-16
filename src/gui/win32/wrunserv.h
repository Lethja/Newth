#ifndef NEW_TH_WINDOW_SERVER_STATUS_H
#define NEW_TH_WINDOW_SERVER_STATUS_H

#include <windows.h>

LRESULT CALLBACK runServerWindowCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

void runServerWindowCreate(WNDCLASSEX *class, HINSTANCE inst, int show);

extern HANDLE serverThread;

#endif /* NEW_TH_WINDOW_SERVER_STATUS_H */
