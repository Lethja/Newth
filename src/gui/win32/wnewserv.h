#ifndef NEW_TH_WINDOW_NEW_SERVER_H
#define NEW_TH_WINDOW_NEW_SERVER_H

#include "iwindows.h"

void newServerWindowCreate(WNDCLASSEX *class, HINSTANCE inst, int show);

LRESULT CALLBACK newServerWindowCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif /* NEW_TH_WINDOW_NEW_SERVER_H */
