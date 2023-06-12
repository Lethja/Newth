#include "../../platform/platform.h"

#include "wrunserv.h"
#include "res.h"
#include "iwindows.h"

#include <commctrl.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>

static WNDCLASSEX sConnectionListClass;
static HWND sAdapterTv, sConnectionList;

void setupTreeAdapterInformation(char *protocol, sa_family_t family, unsigned short port) {
    AdapterAddressArray *adapters = platformGetAdapterInformation(family);
    size_t i, j;

    TVINSERTSTRUCT treeInsert = {0};
    HTREEITEM adapterItem, addressItem = adapterItem = NULL;

    treeInsert.item.cchTextMax = MAX_PATH;
    treeInsert.item.mask = TVIF_TEXT | TVIF_SELECTEDIMAGE;
    treeInsert.item.iImage = 0, treeInsert.item.iSelectedImage = 1;

    if (!adapters) {
        char str[MAX_PATH] = "Unknown Adapter";
        treeInsert.hParent = TVI_ROOT, treeInsert.hInsertAfter = TVI_LAST;
        treeInsert.item.mask = TVIF_TEXT;
        treeInsert.item.pszText = str;
        adapterItem = (HTREEITEM) SendMessage(sAdapterTv, TVM_INSERTITEM, 0, (LPARAM) &treeInsert);

        sprintf(str, "Unknown Address:%d", port);
        treeInsert.hParent = adapterItem;
        treeInsert.hInsertAfter = adapterItem;
        treeInsert.item.pszText = str;
        SendMessage(sAdapterTv, TVM_INSERTITEM, 0, (LPARAM) &treeInsert);
        return;
    }

    for (i = 0; i < adapters->size; ++i) {

        treeInsert.hParent = TVI_ROOT, treeInsert.hInsertAfter = TVI_LAST;
        treeInsert.item.mask = TVIF_TEXT;
        treeInsert.item.pszText = adapters->adapter[i].name;
        adapterItem = (HTREEITEM) SendMessage(sAdapterTv, TVM_INSERTITEM, 0, (LPARAM) &treeInsert);

        for (j = 0; j < adapters->adapter[i].addresses.size; ++j) {
            char str[MAX_PATH];
            if (!adapters->adapter[i].addresses.array[j].type)
                sprintf(str, "%s://%s:%u", protocol, adapters->adapter[i].addresses.array[j].address, port);
            else
                sprintf(str, "%s://[%s]:%u", protocol, adapters->adapter[i].addresses.array[j].address, port);

            treeInsert.hParent = adapterItem;
            treeInsert.hInsertAfter = addressItem;
            treeInsert.item.pszText = str;
            addressItem = (HTREEITEM) SendMessage(sAdapterTv, TVM_INSERTITEM, 0, (LPARAM) &treeInsert);
        }

        /* Automatically expand any adapters that have exactly one address */
        if (j == 1)
            SendMessage(sAdapterTv, TVM_EXPAND, TVE_EXPAND, (LPARAM) adapterItem);
    }

    SendMessage(sAdapterTv, TVM_GETNEXTITEM, TVGN_ROOT, (LPARAM) adapterItem);
    SendMessage(sAdapterTv, TVM_ENSUREVISIBLE, 0, (LPARAM) adapterItem);

    platformFreeAdapterInformation(adapters);
}

LRESULT CALLBACK runServerWindowCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case BTN_DETAILS:
                    if (IsWindowVisible(sConnectionList))
                        ShowWindow(sConnectionList, SW_HIDE);
                    else
                        ShowWindow(sConnectionList, SW_SHOW);
                    break;
            }
            break;
        }

        case WM_CLOSE:
            DestroyWindow(sConnectionList);
            /* Fall though */
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
}

LRESULT CALLBACK detailsWindowCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_SYSCOMMAND:
        case WM_CLOSE:
            return 0;

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
    CreateWindow(_T("BUTTON"), _T("&Details..."), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_PUSHLIKE,
                 winRect.right - 87, 37, 77, 23, window, (HMENU) BTN_DETAILS, inst, 0);

    misc = CreateWindow(_T("BUTTON"), _T("Network Adapters:"), NORMAL_GROUPBOX, 5, miscRect.bottom + 5,
                        winRect.right - 10, winRect.bottom - miscRect.bottom - 10, window, 0, inst, 0);

    /* Make the window visible on the screen */
    ShowWindow(window, show);

    GetClientRect(misc, &miscRect);

    sAdapterTv = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, 0,
                                WS_VISIBLE | WS_CHILD | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_DISABLEDRAGDROP, 5, 15,
                                miscRect.right - 10, miscRect.bottom - 20, misc, (HMENU) TV_ADAPTERS, inst, NULL);

    sConnectionListClass = iWindowCreateClass(inst, "ConnectionDetails", detailsWindowCallback);
    sConnectionListClass.style = CS_NOCLOSE;

    sConnectionList = CreateWindow(sConnectionListClass.lpszClassName, _T("Details"), WS_OVERLAPPED, CW_USEDEFAULT,
                                   CW_USEDEFAULT, 320 + GetSystemMetrics(SM_CXBORDER),
                                   240 + GetSystemMetrics(SM_CYCAPTION), HWND_DESKTOP, NULL, inst, NULL);

    /* Setup system font on all children widgets */
    EnumChildWindows(window, iWindowSetSystemFontEnumerator, (LPARAM) &g_hfDefault);

    /* TODO: Initialize IP stack and populate network adapter addresses somewhere more appropriate */
    platformIpStackInit();

    setupTreeAdapterInformation("http", AF_UNSPEC, 8080);

    platformIpStackExit();
}
