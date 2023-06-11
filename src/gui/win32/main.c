#if defined(UNICODE) && !defined(_UNICODE)
#define _UNICODE
#elif defined(_UNICODE) && !defined(UNICODE)
#define UNICODE
#endif

#include <tchar.h>
#include <windows.h>
#include <commctrl.h>

#include "iwindows.h"
#include "wnewserv.h"
#include "wrunserv.h"

int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nCmdShow) {
    MSG messages;            /* Here messages to the application are saved */
    WNDCLASSEX newWindowClass, runWindowClass;

    InitCommonControls();
    iWindowSetupSystemFontStyle();

    newWindowClass = iWindowCreateClass(hThisInstance, _T("ThNewServer"), newServerWindowCallback);
    runWindowClass = iWindowCreateClass(hThisInstance, _T("ThRunServer"), runServerWindowCallback);

    newServerWindowCreate(&newWindowClass, hThisInstance, nCmdShow);
    runServerWindowCreate(&runWindowClass, hThisInstance, nCmdShow); /* Uncomment to test */

    /* Run the message loop. It will run until GetMessage() returns 0 */
    while (GetMessage(&messages, NULL, 0, 0)) {
        /* Translate virtual-key messages into character messages */
        TranslateMessage(&messages);
        /* Send message to newServerWindowCallback */
        DispatchMessage(&messages);
    }

    iWindowTearDownSystemFontStyle();

    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    return messages.wParam;
}
