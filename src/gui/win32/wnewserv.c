#include "../../platform/platform.h"
#include "../../common/server.h"

#include "wnewserv.h"
#include "res.h"
#include "wrunserv.h"

#include <tchar.h>
#include <windows.h>
#include <shlobj.h>
#include <commctrl.h>

static char *startServer(HWND hwnd) {
    sa_family_t family;
    HWND ports, path;
    char str[MAX_PATH], *err;
    DWORD word;

    err = platformIpStackInit();
    if (err)
        return err;

    ports = GetDlgItem(hwnd, EDT_PORTS), path = GetDlgItem(hwnd, EDT_ROOTPATH);
    GetWindowText(path, str, MAX_PATH);
    word = GetFileAttributes(str);

    if (word == INVALID_FILE_ATTRIBUTES || !(word & FILE_ATTRIBUTE_DIRECTORY))
        return "Root path specified is not a directory";

    globalRootPath = malloc(strlen(str) + 1);
    if (!globalRootPath)
        return "Couldn't allocate memory";

    strcpy(globalRootPath, str);

    GetWindowText(ports, str, MAX_PATH);

#ifndef NDEBUG
    MessageBox(hwnd, globalRootPath, "Debug", MB_ICONINFORMATION);
    MessageBox(hwnd, str, "Debug", MB_ICONINFORMATION);
#endif

    if (platformOfficiallySupportsIpv6())
        family = AF_UNSPEC;
    else
        family = AF_INET;

    err = platformServerStartup(&serverListenSocket, family, str);

    if (err) {
        free(globalRootPath);
        globalRootPath = NULL;
        return err;
    }

    /* TODO: Connect callbacks */

    return NULL;
}

void forkServerProcess(HWND hwnd) {
    /* TODO: Implement the server tick to run on another thread */

    MessageBox(hwnd, "Thread fork not yet implemented", "Implement Me!", MB_ICONEXCLAMATION);
}

LRESULT CALLBACK newServerWindowCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case BTN_START: {
                    char *err = startServer(hwnd);
                    if (err)
                        MessageBox(hwnd, err, "Newth Server Startup Error", MB_ICONERROR);
                    else {
                        HINSTANCE hThisInstance = (HINSTANCE) GetWindowLong(hwnd, GWL_HINSTANCE);
                        WNDCLASSEX runWindowClass = iWindowCreateClass(hThisInstance, _T("ThRunServer"),
                                                                       runServerWindowCallback);
                        forkServerProcess(hwnd);
                        runServerWindowCreate(&runWindowClass, hThisInstance, SW_SHOW);
                        DestroyWindow(hwnd);
                    }
                    break;
                }
                case BTN_BROWSE: {
                    char browsePath[MAX_PATH];
                    HWND rootPathEntry = GetDlgItem(hwnd, EDT_ROOTPATH);
                    if (rootPathEntry) {
                        GetWindowText(rootPathEntry, browsePath, MAX_PATH);
                        iWindowBrowseFolder(browsePath, browsePath, "Browse for root path...");

                        if (strlen(browsePath)) {
                            SetWindowText(rootPathEntry, browsePath);
                        }
                    }
                    break;
                }
            }
            break;
        }
        case WM_DESTROY:
            /* If global root path is NULL this window is being destroyed without a server running, send WM_QUIT */
            if (!globalRootPath)
                PostQuitMessage(0);
            else
                /* Fall through */
                default:
                    return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}

typedef HRESULT (__stdcall *GetFolderPath)(HWND, int, HANDLE, DWORD, LPSTR);

#ifndef UNICODE
#define FOLDER_FUNCTION "SHGetFolderPathA"
#else
#define FOLDER_FUNCTION "SHGetFolderPathW"
#endif

static void SetDesktopPath(HWND window) {
    LPSTR path = NULL;

#pragma region Modern System Function Call

    HMODULE module = LoadLibrary("shell32.dll");
    if (module) {
        GetFolderPath FolderPathFunc = (GetFolderPath) GetProcAddress(module, FOLDER_FUNCTION);
        if (FolderPathFunc) {
            path = malloc(MAX_PATH);
            FolderPathFunc(NULL, CSIDL_DESKTOP, NULL, 0, path);
            if (path[0])
                SetWindowText(GetDlgItem(window, EDT_ROOTPATH), path);

            free(path);
            FreeLibrary(module);
            return;
        }
        FreeLibrary(module);
    }

#pragma endregion

#pragma region Fallback for DOS based Windows

    path = malloc(MAX_PATH);
    GetWindowsDirectory(path, MAX_PATH);
    /* TODO: Is this path name the same in all languages? */
    strncat(path, "\\Desktop", MAX_PATH);
    SetWindowText(GetDlgItem(window, EDT_ROOTPATH), path);
    free(path);

#pragma endregion
}

void newServerWindowCreate(WNDCLASSEX *class, HINSTANCE inst, int show) {
    HWND window, misc;

    /* Create the 'New Server' desktop window */
    window = CreateWindow(class->lpszClassName,                             /* Class name */
                          _T("New Server"),                                 /* Title Text */
                          WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU,      /* Fixed size window */
                          CW_USEDEFAULT,                                    /* Windows decides the position */
                          CW_USEDEFAULT,                                    /* where the window ends up on the screen */
                          320 + GetSystemMetrics(SM_CXBORDER),              /* The programs width */
                          192 + GetSystemMetrics(SM_CYCAPTION),             /* and height in pixels */
                          HWND_DESKTOP,                                     /* The window is a child-window to desktop */
                          NULL,                                             /* No menu */
                          inst,                                             /* Program Instance handler */
                          NULL                                              /* No Window Creation data */
    );

    /* Create the path entry & button widgets */
    CreateWindow(_T("BUTTON"), _T("Root Path:"), NORMAL_GROUPBOX, 5, 5, 305, 53, window, 0, inst, 0);
    CreateWindowEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""), NORMAL_ENTRY, 10, 27, 210, 23, window, (HMENU) EDT_ROOTPATH,
                   inst, 0);
    CreateWindow(_T("BUTTON"), _T("&Browse..."), NORMAL_BUTTON, 227, 27, 77, 23, window, (HMENU) BTN_BROWSE, inst, 0);
    SetDesktopPath(window);

    /* Create listen port radio buttons */
    CreateWindow(_T("BUTTON"), _T("Listen Port:"), NORMAL_GROUPBOX, 5, 58, 305, 94, window, 0, inst, 0);
    misc = CreateWindow(_T("BUTTON"), _T("&Ephemeral Port"), NORMAL_RADIO | WS_GROUP, 10, 80, 290, 17, window, 0, inst,
                        0);
    SendMessage(misc, BM_SETCHECK, BST_CHECKED, 0);

    CreateWindow(_T("BUTTON"), _T("&HTTP Port"), NORMAL_RADIO, 10, 104, 290, 17, window, 0, inst, 0);
    CreateWindow(_T("BUTTON"), _T("&Custom:"), NORMAL_RADIO, 10, 128, 85, 17, window, 0, inst, 0);

    CreateWindowEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T("80,8080,0"), NORMAL_ENTRY, 85, 125, 220, 23, window,
                   (HMENU) EDT_PORTS, inst, 0);

    /* Create the Start/Stop server button */
    CreateWindow(_T("BUTTON"), _T("&Host"), NORMAL_BUTTON, 235, 158, 75, 23, window, (HMENU) BTN_START, inst, 0);

    /* Make the window visible on the screen */
    ShowWindow(window, show);

    /* Setup system font on all children widgets */
    EnumChildWindows(window, iWindowSetSystemFontEnumerator, (LPARAM) &g_hfDefault);
}
