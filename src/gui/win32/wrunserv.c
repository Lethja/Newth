#include "../../platform/platform.h"
#include "../../server/server.h"

#include "wrunserv.h"
#include "res.h"
#include "iwindows.h"

#include <commctrl.h>
#include <ctype.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>

static WNDCLASSEX sConnectionListClass;
static HWND sAdapterTv, sConnectionList, sConnectionWin, sStats;
static unsigned long nActiveConnections = 0, nActiveTransfers = 0;
/* static HANDLE updateConnectionListMutex; */

HANDLE serverThread;

#pragma region Connection List Window

LRESULT CALLBACK detailsWindowCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) { /* NOLINT(hicpp-multiway-paths-covered) */
        case WM_CLOSE:
            return 0; /* Ignore */

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
}

void updateConnectionsLabel(void) {
    HWND connections = GetDlgItem(sStats, LBL_ACTIVE_CONNECTIONS);
    TCHAR temp[64];
    sprintf(temp, "Active Connections: %lu", nActiveConnections);
    SetWindowText(connections, temp);
}

void updateTransferLabel(void) {
    HWND connections = GetDlgItem(sStats, LBL_ACTIVE_TRANSFERS);
    TCHAR temp[64];
    sprintf(temp, "Active Transfers: %lu", nActiveTransfers);
    SetWindowText(connections, temp);
}

static int GetRowByAddress(HWND list, const char *address) {
    TCHAR addr[MAX_PATH], item[MAX_PATH] = {0};
    LVITEM lvi;
    int i, max;

    ZeroMemory(&lvi, sizeof(LVITEM));
    lvi.mask = LVIF_TEXT, lvi.pszText = item, lvi.cchTextMax = MAX_PATH, lvi.iSubItem = 1; /* Address */
    max = SendMessage(list, LVM_GETITEMCOUNT, 0, 0);

    strncpy(addr, address, MAX_PATH);

    for (i = 0; i < max; ++i) {
        lvi.iItem = i;
        if (SendMessage(list, LVM_GETITEM, 0, (LPARAM) &lvi)) {
            if (lvi.mask & LVIF_TEXT) {
                if (strncmp(addr, item, MAX_PATH) == 0)
                    return i; /* This is the correct item to update or remove */
            }
        }
    }
    return -1;
}

static void updateRow(HWND list, const char *address, const char *state, const char *path) {
    TCHAR temp[MAX_PATH];
    LVITEM item;
    int row = GetRowByAddress(list, address);

    ZeroMemory(&temp, MAX_PATH), ZeroMemory(&item, sizeof(LVITEM));
    item.cchTextMax = MAX_PATH, item.mask = LVIF_TEXT, item.pszText = temp;

    item.iSubItem = 0, strncpy(temp, state, MAX_PATH - 1);
    if (row == -1) {
        item.iItem = SendMessage(list, LVM_INSERTITEM, 0, (LPARAM) &item);
        if (item.iItem == -1)
            MessageBox(list, "Error inserting text", "Debug Error", MB_ICONERROR);
        item.iSubItem = 1, strncpy(temp, address, MAX_PATH - 1);

        if (SendMessage(list, LVM_SETITEM, 0, (LPARAM) &item) == -1)
            MessageBox(list, "Error setting subtext", "Debug Error", MB_ICONERROR);
    } else {
        item.iItem = row;
        if (SendMessage(list, LVM_SETITEM, 0, (LPARAM) &item) == -1)
            MessageBox(list, "Error setting subtext", "Debug Error", MB_ICONERROR);
    }

    item.iSubItem = 2, strncpy(temp, path, MAX_PATH - 1);
    if (SendMessage(list, LVM_SETITEM, 0, (LPARAM) &item) == -1)
        MessageBox(list, "Error setting subtext", "Debug Error", MB_ICONERROR);
}

static void removeRow(HWND list, const char *address) {
    int row = GetRowByAddress(list, address);
    if (row >= 0) {
        TCHAR str[12] = {0};
        LVITEM lvi;

        ZeroMemory(&lvi, sizeof(LVITEM));
        lvi.cchTextMax = 12, lvi.pszText = (char *) &str, lvi.iItem = row, lvi.mask = LVIF_TEXT;
        /* If this rows state is not TSYN then we must also decrement the active transfers counter */
        if (SendMessage(sConnectionList, LVM_GETITEM, 0, (LPARAM) &lvi)) {
            if ((toupper(str[0]) == 'G') && strlen(str) >= 4) {
                switch (str[3]) {
                    case '0':
                    case '6':
                    case '8':
                        --nActiveTransfers;
                        updateTransferLabel();
                }
            }
        }
        SendMessage(list, LVM_DELETEITEM, (WPARAM) row, 0);
    } else
        MessageBox(NULL, "Attempted to remove a address that didn't exist", "Debug", MB_ICONERROR);
}

static void callbackCloseSocket(SOCKET *socket) { /* NOLINT(readability-non-const-parameter) */
    struct sockaddr_storage ss;
    socklen_t sockLen = sizeof(ss);
    sa_family_t family;
    char ip[INET6_ADDRSTRLEN], temp[MAX_PATH];
    unsigned short port;

    getpeername(*socket, (struct sockaddr *) &ss, &sockLen);
    platformGetIpString((struct sockaddr *) &ss, ip, &family);
    port = platformGetPort((struct sockaddr *) &ss);

    sprintf(temp, "%s:%d", ip, port);

    /* WaitForSingleObject(updateConnectionListMutex, INFINITE); */
    removeRow(sConnectionList, temp);

    --nActiveConnections;
    updateConnectionsLabel();

    /* ReleaseMutex(updateConnectionListMutex); */
}

static void callbackNewSocket(SOCKET *socket) { /* NOLINT(readability-non-const-parameter) */
    struct sockaddr_storage ss;
    socklen_t sockLen = sizeof(ss);
    sa_family_t family;
    char ip[INET6_ADDRSTRLEN], temp[MAX_PATH];
    unsigned short port;

    getpeername(*socket, (struct sockaddr *) &ss, &sockLen);
    platformGetIpString((struct sockaddr *) &ss, ip, &family);
    port = platformGetPort((struct sockaddr *) &ss);
    sprintf(temp, "%s:%u", ip, port);

    /* WaitForSingleObject(updateConnectionListMutex, INFINITE); */
    updateRow(sConnectionList, temp, "TSYN", "");

    ++nActiveConnections;
    updateConnectionsLabel();

    /* ReleaseMutex(updateConnectionListMutex); */
}

static void callbackHttpEnd(eventHttpRespond *event) {
    struct sockaddr_storage ss;
    socklen_t sockLen = sizeof(ss);
    sa_family_t family;
    char ip[INET6_ADDRSTRLEN], temp[MAX_PATH];
    unsigned short port;

    getpeername(*event->clientSocket, (struct sockaddr *) &ss, &sockLen);
    platformGetIpString((struct sockaddr *) &ss, ip, &family);
    port = platformGetPort((struct sockaddr *) &ss);
    sprintf(temp, "%s:%u", ip, port);

    /* WaitForSingleObject(updateConnectionListMutex, INFINITE); */
    updateRow(sConnectionList, temp, "TSYN", event->path);

    --nActiveTransfers;
    updateTransferLabel();

    /* ReleaseMutex(updateConnectionListMutex); */
}

static void callbackHttpStart(eventHttpRespond *event) {
    struct sockaddr_storage ss;
    socklen_t sockLen = sizeof(ss);
    sa_family_t family;
    char ip[INET6_ADDRSTRLEN], temp[MAX_PATH], state[12], type;
    unsigned short port;

    getpeername(*event->clientSocket, (struct sockaddr *) &ss, &sockLen);
    platformGetIpString((struct sockaddr *) &ss, ip, &family);
    port = platformGetPort((struct sockaddr *) &ss);
    sprintf(temp, "%s:%u", ip, port);

    switch (*event->type) {
        case httpGet:
            type = 'G';
            break;
        case httpHead:
            type = 'H';
            break;
        case httpPost:
            type = 'P';
            break;
        default:
            type = '?';
            break;
    }

    sprintf(state, "%c%03d", type, *event->response);

    /* WaitForSingleObject(updateConnectionListMutex, INFINITE); */

    switch (*event->response) {
        case 200:
        case 206:
        case 208:
            ++nActiveTransfers;
            updateTransferLabel();
            break;

    }

    updateRow(sConnectionList, temp, state, event->path);

    /* ReleaseMutex(updateConnectionListMutex); */
}

HWND connectionsListCreate(WNDCLASSEX *class, HINSTANCE inst, HWND parent, RECT *parentRect) {
    HWND list;
    TCHAR header[24] = "State\0\0\0Address\0Path";
    LVCOLUMN lvc;
    int i, max = 3;

    list = CreateWindow(WC_LISTVIEW, 0, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_AUTOVSCROLL | LVS_REPORT,
                        parentRect->left, parentRect->top, parentRect->right, parentRect->bottom, parent, 0, inst,
                        NULL);

    SendMessage(list, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

    lvc.fmt = LVCFMT_LEFT, lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    for (i = 0; i < max; ++i) {
        LPSTR ptr = &header[i * (8 * sizeof(TCHAR))];
        switch (i) {
            case 0:
                lvc.cx = 50, lvc.pszText = "State";
                break;
            case 1:
                lvc.cx = 100, lvc.pszText = "Address";
                break;
            case 2:
                lvc.cx = 170, lvc.pszText = "Path";
                break;
            default:
                lvc.pszText = "?";
                break;
        }

        lvc.iSubItem = i;
        LoadString(inst, LVC_CONNECTION + i, ptr, 14);
        if (SendMessage(list, LVM_INSERTCOLUMN, (WPARAM) &i, (LPARAM) (&lvc)) == -1)
            MessageBox(parent, "Error inserting column", "Error", 0);
    }

    /* updateConnectionListMutex = CreateMutex(NULL, FALSE, NULL); */

    eventHttpRespondSetCallback(callbackHttpStart);
    eventHttpFinishSetCallback(callbackHttpEnd);
    eventSocketAcceptSetCallback(callbackNewSocket);
    eventSocketCloseSetCallback(callbackCloseSocket);

    return list;
}

#pragma endregion

#pragma region Running Server Window

void setupTreeAdapterInformation(char *protocol, sa_family_t family, unsigned short port) {
    AdapterAddressArray *adapters = platformGetAdapterInformation(family);
    size_t i, j;

    TVINSERTSTRUCT treeInsert = {0};
    HTREEITEM adapterItem, addressItem = adapterItem = NULL;

    /* Clear old entries in case of a refresh */
    SendMessage(sAdapterTv, TVM_DELETEITEM, 0, (LPARAM) TVI_ROOT);

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
                    if (IsWindowVisible(sConnectionWin))
                        ShowWindow(sConnectionWin, SW_HIDE);
                    else {
                        HDC hdc = GetDC(hwnd);
                        RECT rect;
                        int hRes, x, w = 320 + GetSystemMetrics(SM_CXBORDER);

                        /* Move connection window to the right (or left if not enough space) of the server window */
                        GetWindowRect(hwnd, &rect);
                        hRes = hdc ? GetDeviceCaps(hdc, HORZRES) : 0;
                        x = hRes > 700 && rect.right + w >= hRes ? rect.left - w : rect.right;

                        MoveWindow(sConnectionWin, x, rect.top, w, 240 + GetSystemMetrics(SM_CYCAPTION), 0);
                        ShowWindow(sConnectionWin, SW_SHOW);
                    }
                    break;
            }
            break;
        }
        case WM_DESTROY:
            /* Volatile global variable to make the server thread stop gracefully */
            serverRun = 0;

            /* Detach all callbacks */
            eventHttpFinishSetCallback(NULL);
            eventHttpRespondSetCallback(NULL);
            eventSocketAcceptSetCallback(NULL);
            eventSocketCloseSetCallback(NULL);

            /* Wait for thread to stop before tear down */
            serverPoke();
            /* WaitForSingleObject(serverThread, INFINITE); */
            {
                SOCKET *sockets = serverGetReadSocketArray();
                if (sockets)
                    platformCloseBindSockets(sockets), free(sockets);
            }
            platformIpStackExit();

            free(globalRootPath);
            PostQuitMessage(0);
            break;

        case WM_CLOSE:
            DestroyWindow(sConnectionWin);
        /* Fall through */
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

void connectionsWindowCreate(WNDCLASSEX *class, HINSTANCE inst, int show) {
    RECT rect;
    HWND window = sConnectionWin = CreateWindow(class->lpszClassName, _T("Connection Details - Newth"), WS_OVERLAPPED,
                                   CW_USEDEFAULT, CW_USEDEFAULT, 320 + GetSystemMetrics(SM_CXBORDER),
                                   240 + GetSystemMetrics(SM_CYCAPTION), HWND_DESKTOP, NULL, inst, NULL);

    GetClientRect(window, &rect);

    sConnectionList = connectionsListCreate(class, inst, window, &rect);

    ShowWindow(window, show);
}

void runServerWindowCreate(WNDCLASSEX *class, HINSTANCE inst, int show) {
    HWND window, misc;
    RECT winRect, miscRect;

    /* Create the running server desktop window */
    window = CreateWindow(class->lpszClassName,                             /* Class name */
                          _T("Http Server - Newth"),                        /* Title Text */
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
    /* TODO: sStats could be made non-global if the labels below it where parented to the top window instead */
    sStats = misc = CreateWindow(_T("BUTTON"), _T("Statistics:"), NORMAL_GROUPBOX, 5, 5, winRect.right - 10, 60, window,
                                 0, inst, 0);

    GetClientRect(misc, &miscRect);

    CreateWindow(_T("STATIC"), _T(""), NORMAL_LABEL, 10, 15, miscRect.right - 97, 23, misc,
                 (HMENU) LBL_ACTIVE_CONNECTIONS, inst, 0);
    CreateWindow(_T("STATIC"), _T(""), NORMAL_LABEL, 10, 33, miscRect.right - 97, 23, misc,
                 (HMENU) LBL_ACTIVE_TRANSFERS, inst, 0);
    CreateWindow(_T("BUTTON"), _T("&Details..."), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_PUSHLIKE,
                 winRect.right - 87, 37, 77, 23, window, (HMENU) BTN_DETAILS, inst, 0);

    updateConnectionsLabel();
    updateTransferLabel();

    misc = CreateWindow(_T("BUTTON"), _T("Network Adapters:"), NORMAL_GROUPBOX, 5, miscRect.bottom + 5,
                        winRect.right - 10, winRect.bottom - miscRect.bottom - 10, window, 0, inst, 0);

    /* Make the window visible on the screen */
    ShowWindow(window, show);

    GetClientRect(misc, &miscRect);

    sAdapterTv = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, 0,
                                WS_VISIBLE | WS_CHILD | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_DISABLEDRAGDROP, 5, 15,
                                miscRect.right - 10, miscRect.bottom - 20, misc, 0, inst, NULL);

    sConnectionListClass = iWindowCreateClass(inst, "ThConnectionDetails", detailsWindowCallback);
    sConnectionListClass.style = CS_NOCLOSE;

    connectionsWindowCreate(&sConnectionListClass, inst, SW_HIDE);

    /* Setup system font on all children widgets */
    EnumChildWindows(window, iWindowSetSystemFontEnumerator, (LPARAM) &g_hfDefault);

    setupTreeAdapterInformation("http", (sa_family_t) (platformOfficiallySupportsIpv6() ? AF_UNSPEC : AF_INET),
                                getPort(&serverListenSocket[1]));
}

#pragma endregion
