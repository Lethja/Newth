#include "wrunserv.h"
#include "res.h"
#include "iwindows.h"

#include <tchar.h>
#include <windows.h>

LRESULT CALLBACK runServerWindowCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
}

void runServerWindowCreate(WNDCLASSEX *class, HINSTANCE inst, int show) {
    HWND window, misc;
    RECT winRect, miscRect;

    /* Create the running server desktop window */
    window = CreateWindow(class->lpszClassName,                             /* Class name */
                          _T("Server Statistics"),                          /* Title Text */
                          WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU,      /* Fixed size window */
                          CW_USEDEFAULT,                                    /* Windows decides the position */
                          CW_USEDEFAULT,                                    /* where the window ends up on the screen */
                          320 + GetSystemMetrics(SM_CXBORDER),              /* The programs width */
                          240 + GetSystemMetrics(SM_CYCAPTION),             /* and height in pixels */
                          HWND_DESKTOP,                                     /* The window is a child-window to desktop */
                          NULL,                                             /* No menu */
                          inst,                                             /* Program Instance handler */
                          NULL                                              /* No Window Creation data */
    );

    GetClientRect(window, &winRect);

    /* Create the path entry & button widgets */
    misc = CreateWindow(_T("BUTTON"), _T("Statistics:"), NORMAL_GROUPBOX, 5, 5, winRect.right - 10, 60, window, 0, inst,
                        0);

    GetClientRect(misc, &miscRect);

    CreateWindow(_T("STATIC"), _T("Active Connections: 0"), NORMAL_LABEL, 10, 15, miscRect.right - 97, 23, misc, 0,
                 inst, 0);
    CreateWindow(_T("STATIC"), _T("Active Transfers: 0"), NORMAL_LABEL, 10, 33, miscRect.right - 97, 23, misc, 0, inst,
                 0);
    CreateWindow(_T("BUTTON"), _T("&Details..."), NORMAL_BUTTON, miscRect.right - 87, miscRect.bottom - 33, 77, 23,
                 misc, 0, inst, 0);

    /* Make the window visible on the screen */
    ShowWindow(window, show);

    /* Setup system font on all children widgets */
    EnumChildWindows(window, iWindowSetSystemFontEnumerator, (LPARAM) &g_hfDefault);
}