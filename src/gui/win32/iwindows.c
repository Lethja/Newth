#include "iwindows.h"

#include <shlobj.h>
#include <windows.h>

#pragma region Font Management

HFONT g_hfDefault;
NONCLIENTMETRICS g_Ncm;

#pragma endregion

BOOL CALLBACK iWindowSetSystemFontEnumerator(HWND hWnd, LPARAM lParam) {
    HFONT hFont = *(HFONT *) lParam;
    SendMessage(hWnd, WM_SETFONT, (WPARAM) hFont, MAKELPARAM(TRUE, 0));
    return TRUE;
}

static int CALLBACK iWindowBrowseFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) {
    if (uMsg == BFFM_INITIALIZED) {
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
    }

    return 0;
}

void iWindowSetupSystemFontStyle() {

    ZeroMemory(&g_Ncm, sizeof(NONCLIENTMETRICS));
    g_Ncm.cbSize = sizeof(NONCLIENTMETRICS);

    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &g_Ncm, FALSE);
    g_hfDefault = CreateFontIndirect(&g_Ncm.lfMessageFont);
}

void iWindowTearDownSystemFontStyle() {
    DeleteObject(g_hfDefault);
}

void iWindowBrowseFolder(const char *currentPath, char *returnPath, LPCSTR title) {
    BROWSEINFO bi = {0};
    LPITEMIDLIST pItemIdList;

    bi.lpszTitle = title;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON | BIF_EDITBOX;
    bi.lpfn = iWindowBrowseFolderCallback;
    bi.lParam = (LPARAM) currentPath;

    pItemIdList = SHBrowseForFolder(&bi);

    if (pItemIdList != 0) {
        /* Get the name of the folder and put it in returnPath */
        HRESULT hr = SHGetPathFromIDList(pItemIdList, returnPath);

        /* Free co-task memory */
        CoTaskMemFree(pItemIdList);

        if (hr)
            return;

        /* Fallthrough */
    }

    returnPath[0] = '\0';
}

WNDCLASSEX iWindowCreateClass(HINSTANCE hThisInstance, const TCHAR className[], WNDPROC windowProcedure) {
    WNDCLASSEX winClass;     /* Data structure for the window class */

    /* The Window structure */
    winClass.hInstance = hThisInstance;
    winClass.lpszClassName = className;
    winClass.lpfnWndProc = windowProcedure;      /* This function is called by windows */
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
    winClass.hbrBackground = GetSysColorBrush(COLOR_3DFACE);

    /* Register the window class, and if it fails quit the program */
    if (!RegisterClassEx(&winClass)) {
        MessageBox(NULL, "Unable to register class", className, MB_ICONEXCLAMATION);
        exit(1);
    }

    return winClass;
}