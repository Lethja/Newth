#if defined(UNICODE) && !defined(_UNICODE)
#define _UNICODE
#elif defined(_UNICODE) && !defined(UNICODE)
#define UNICODE
#endif

#include <tchar.h>
#include <windows.h>
#include "res.h"

/* TODO: Add a manifest to enable visual styles */

typedef struct RadioPortButtons {
    HWND ephemeral;
    HWND http;
    HWND custom;
    HWND port;
} PortRadio;

typedef struct NewThInterface {
    HWND winMain;
    HWND btnRun;
    HWND btnBrowse;
    HWND entryPath;
    HWND labelPath;
    HWND listAdapter;
    HWND listConnections;
    PortRadio port;
} NewThInterface;

/*  Make the class name into a global variable  */
TCHAR szClassName[] = _T("NewThInterface");

NewThInterface NewThCreate(HINSTANCE inst, int show) {
    NewThInterface r;

    /* Create the desktop Window */
    r.winMain = CreateWindow (szClassName,                                      /* Class name */
                              _T("Newth Http Server"),                          /* Title Text */
                              WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU,      /* Fixed size window */
                              CW_USEDEFAULT,                                    /* Windows decides the position */
                              CW_USEDEFAULT,                                    /* where the window ends up on the screen */
                              320,                                              /* The programs width */
                              240,                                              /* and height in pixels */
                              HWND_DESKTOP,                                     /* The window is a child-window to desktop */
                              NULL,                                             /* No menu */
                              inst,                                             /* Program Instance handler */
                              NULL                                              /* No Window Creation data */
    );

    /* Create the path entry & button widgets */
    CreateWindow(_T("BUTTON"), _T("Root Path:"), NORMAL_GROUPBOX, 5, 5, 300, 53, r.winMain, 0, inst, 0);
    r.entryPath = CreateWindow(_T("EDIT"), _T(""), NORMAL_ENTRY, 10, 27, 209, 23, r.winMain, 0, inst, 0);
    r.btnBrowse = CreateWindow(_T("BUTTON"), _T("&Browse..."), NORMAL_BUTTON, 222, 27, 77, 23, r.winMain,
                               (HMENU) BTN_BROWSE, inst, 0);

    /* Create listen port radio buttons */
    CreateWindow(_T("BUTTON"), _T("Listen Port:"), NORMAL_GROUPBOX, 5, 58, 300, 94, r.winMain, 0, inst, 0);
    r.port.ephemeral = CreateWindow(_T("BUTTON"), _T("&Ephemeral Port"), NORMAL_RADIO | WS_GROUP, 10, 80, 290, 17,
                                    r.winMain, 0, inst, 0);
    SendMessage(r.port.ephemeral, BM_SETCHECK, BST_CHECKED, 0);

    r.port.http = CreateWindow(_T("BUTTON"), _T("&HTTP Port"), NORMAL_RADIO, 10, 104, 290, 17, r.winMain, 0, inst, 0);
    r.port.custom = CreateWindow(_T("BUTTON"), _T("&Custom:"), NORMAL_RADIO, 10, 128, 85, 17, r.winMain, 0, inst, 0);

    r.port.port = CreateWindow(_T("EDIT"), _T(""), NORMAL_ENTRY, 85, 125, 215, 23, r.winMain, 0, inst, 0);

    /* Create the Start/Stop server button */
    r.labelPath = CreateWindow(_T("STATIC"), _T("Stopped"), NORMAL_LABEL, 5, 185, 235, 23, r.winMain, 0, inst, 0);

    r.btnRun = CreateWindow(_T("BUTTON"), _T("&Start"), NORMAL_BUTTON, 229, 185, 75, 23, r.winMain, (HMENU) BTN_START,
                            inst, 0);

    /* Make the window visible on the screen */
    ShowWindow(r.winMain, show);

    return r;
}

/*  This function is called by the Windows function DispatchMessage()  */
LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        /* TODO: CoInitialize() and SHBrowseForFolder() */
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                /* TODO: Useful code here */
                case BTN_START:
                    MessageBox(hwnd, "Foo", "Foo", 0);
                    break;
                case BTN_BROWSE:
                    MessageBox(hwnd, "Bar", "Bar", 0);
                    break;
            }
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0); /* send a WM_QUIT to the message queue */
            break;
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}

int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nCmdShow) {
    MSG messages;            /* Here messages to the application are saved */
    WNDCLASSEX winClass;        /* Data structure for the window class */
    NewThInterface newTh;

    /* The Window structure */
    winClass.hInstance = hThisInstance;
    winClass.lpszClassName = szClassName;
    winClass.lpfnWndProc = WindowProcedure;      /* This function is called by windows */
    winClass.style = CS_DBLCLKS;                 /* Catch double-clicks */
    winClass.cbSize = sizeof(WNDCLASSEX);

    /* Use default icon and mouse-pointer */
    winClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    winClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    winClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    winClass.lpszMenuName = NULL; /* No menu */
    winClass.cbClsExtra = 0;      /* No extra bytes after the window class */
    winClass.cbWndExtra = 0;      /* structure or the window instance */

    /* Use Windows's default color as the background of the window */
    winClass.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    /* Register the window class, and if it fails quit the program */
    if (!RegisterClassEx(&winClass))
        return 0;

    /* The class is registered, create the program */
    newTh = NewThCreate(hThisInstance, nCmdShow);

    /* Run the message loop. It will run until GetMessage() returns 0 */
    while (GetMessage(&messages, NULL, 0, 0)) {
        /* Translate virtual-key messages into character messages */
        TranslateMessage(&messages);
        /* Send message to WindowProcedure */
        DispatchMessage(&messages);
    }

    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    return messages.wParam;
}
