#include "../../platform/platform.h"
#include "../../common/server.h"

#include "wnewserv.h"
#include "res.h"
#include "wrunserv.h"

#include <tchar.h>
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <commctrl.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

static DWORD WINAPI serverTickThread(LPVOID lpParam) {
    serverTick();
    ExitThread(0);
    return 0;
}

static void noOp(void *foo) {}

#pragma clang diagnostic pop

static char *startServer(HWND hwnd) {
    sa_family_t family;
    HWND ports, path;
    char str[MAX_PATH], *err;
    DWORD word;

    err = platformIpStackInit();
    if (err)
        return err;

    path = GetDlgItem(hwnd, EDT_ROOT_PATH);
    GetWindowText(path, str, MAX_PATH);
    word = GetFileAttributes(str);

    if (word == INVALID_FILE_ATTRIBUTES || !(word & FILE_ATTRIBUTE_DIRECTORY))
        return "Root path specified is not a directory";

    globalRootPath = malloc(strlen(str) + 1);
    if (!globalRootPath)
        return "Couldn't allocate memory";

    strcpy(globalRootPath, str);

    ports = GetDlgItem(hwnd, RB_PORT_EPHEMERAL);
    if (SendMessage(ports, BM_GETSTATE, 0, 0) == BST_CHECKED)
        strcpy(str, "0");
    else {
        ports = GetDlgItem(hwnd, RB_PORT_HTTP);
        if (SendMessage(ports, BM_GETSTATE, 0, 0) == BST_CHECKED)
            strcpy(str, "80,8080");
        else {
            ports = GetDlgItem(hwnd, EDT_PORTS);
            GetWindowText(ports, str, MAX_PATH);
        }
    }

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

    /* Set new server memory state */
    serverMaxSocket = serverListenSocket;
    FD_ZERO(&serverCurrentSockets);
    FD_SET(serverListenSocket, &serverCurrentSockets);

    globalFileRoutineArray.size = globalDirRoutineArray.size = 0;
    globalFileRoutineArray.array = globalDirRoutineArray.array = NULL;

    return NULL;
}

void forkServerProcess(HWND hwnd) {
    /* TODO: Connect callbacks to real code updating the list entries */
    eventHttpRespondSetCallback((void (*)(eventHttpRespond *)) &noOp);
    eventHttpFinishSetCallback((void (*)(eventHttpRespond *)) &noOp);
    eventSocketCloseSetCallback((void (*)(SOCKET *)) &noOp);
    eventSocketAcceptSetCallback((void (*)(SOCKET *)) &noOp);

    /* TODO: Handle thread properly and don't create suspended */
    serverThread = CreateThread(NULL, 0, serverTickThread, NULL, 0, NULL);

    if (!serverThread) {
        MessageBox(hwnd, "Unable to create thread for Newth server routine", "Error", MB_ICONEXCLAMATION);
        PostQuitMessage(0);
    }
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
#ifdef _WIN64
                        HINSTANCE hThisInstance = (HINSTANCE) GetWindowLongPtr(hwnd, -6 /* GWL_INSTANCE */);
#else
                        HINSTANCE hThisInstance = (HINSTANCE) GetWindowLong(hwnd, -6 /* GWL_INSTANCE */);
#endif
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
                    HWND rootPathEntry = GetDlgItem(hwnd, EDT_ROOT_PATH);
                    if (rootPathEntry) {
                        GetWindowText(rootPathEntry, browsePath, MAX_PATH);
                        iWindowBrowseFolder(browsePath, browsePath, "Browse for root path...");

                        if (strlen(browsePath)) {
                            SetWindowText(rootPathEntry, browsePath);
                        }
                    }
                    break;
                }
                case EDT_PORTS: {
                    HWND focus = GetFocus(), ports = GetDlgItem(hwnd, EDT_PORTS);
                    if (focus != ports)
                        break;

                    SendMessage(GetDlgItem(hwnd, RB_PORT_EPHEMERAL), BM_SETCHECK, BST_UNCHECKED, 0);
                    SendMessage(GetDlgItem(hwnd, RB_PORT_HTTP), BM_SETCHECK, BST_UNCHECKED, 0);
                    SendMessage(GetDlgItem(hwnd, RB_PORT_CUSTOM), BM_SETCHECK, BST_CHECKED, 0);
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

static char *getDirectoryPathArg(void) {
    char *dirPath = NULL;

#ifdef __argc
    int i;

    for (i = 1; i < __argc; ++i) {
        PlatformFileStat st;
        if (!platformFileStat(__argv[i], &st) && platformFileStatIsDirectory(&st)) {
            dirPath = malloc(strlen(__argv[i]) + 1);
            strcpy(dirPath, __argv[i]);
            break;
        }
    }

#endif

    return dirPath;
}

static void SetRootStartUpPath(HWND window) {
    LPSTR path = NULL;
    HMODULE module;

#pragma region Check For Argv Path Argument

    path = getDirectoryPathArg();

    if (path) {
        SetWindowText(GetDlgItem(window, EDT_ROOT_PATH), path);
        free(path);
        return;
    }

#pragma endregion

#pragma region Modern System Function Call To Get Desktop Path

    module = LoadLibrary("shell32.dll");
    if (module) {
        GetFolderPath FolderPathFunc = (GetFolderPath) GetProcAddress(module, FOLDER_FUNCTION);
        if (FolderPathFunc) {
            path = malloc(MAX_PATH);
            FolderPathFunc(NULL, CSIDL_DESKTOP, NULL, 0, path);
            if (path[0])
                SetWindowText(GetDlgItem(window, EDT_ROOT_PATH), path);

            free(path);
            FreeLibrary(module);
            return;
        }
        FreeLibrary(module);
    }

#pragma endregion

#pragma region Fallback For DOS based Windows To Get Desktop Path

    path = malloc(MAX_PATH);
    GetWindowsDirectory(path, MAX_PATH);
    /* TODO: Is this path name the same in all languages? */
    strncat(path, "\\Desktop", MAX_PATH);
    SetWindowText(GetDlgItem(window, EDT_ROOT_PATH), path);
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
    CreateWindowEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""), NORMAL_ENTRY, 10, 27, 210, 23, window, (HMENU) EDT_ROOT_PATH,
                   inst, 0);

    CreateWindow(_T("BUTTON"), _T("&Browse..."), NORMAL_BUTTON, 227, 27, 77, 23, window, (HMENU) BTN_BROWSE, inst, 0);
    SetRootStartUpPath(window);

    /* Create listen port radio buttons */
    CreateWindow(_T("BUTTON"), _T("Listen Port:"), NORMAL_GROUPBOX, 5, 58, 305, 94, window, 0, inst, 0);
    misc = CreateWindow(_T("BUTTON"), _T("&Ephemeral Port"), NORMAL_RADIO | WS_GROUP, 10, 80, 290, 17, window,
                        (HMENU) RB_PORT_EPHEMERAL, inst, 0);
    SendMessage(misc, BM_SETCHECK, BST_CHECKED, 0);

    CreateWindow(_T("BUTTON"), _T("&HTTP Port"), NORMAL_RADIO, 10, 104, 290, 17, window, (HMENU) RB_PORT_HTTP, inst, 0);
    CreateWindow(_T("BUTTON"), _T("&Custom:"), NORMAL_RADIO, 10, 128, 75, 17, window, (HMENU) RB_PORT_CUSTOM, inst, 0);

    CreateWindowEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T("80,8080,0"), NORMAL_ENTRY, 85, 125, 220, 23, window,
                   (HMENU) EDT_PORTS, inst, 0);

    /* Create the Start/Stop server button */
    CreateWindow(_T("BUTTON"), _T("&Host"), NORMAL_BUTTON, 235, 158, 75, 23, window, (HMENU) BTN_START, inst, 0);

    /* Make the window visible on the screen */
    ShowWindow(window, show);

    /* Setup system font on all children widgets */
    EnumChildWindows(window, iWindowSetSystemFontEnumerator, (LPARAM) &g_hfDefault);
}
