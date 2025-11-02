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

int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nCmdShow) {
	MSG messages;            /* Here messages to the application are saved */
	WNDCLASSEX newWindowClass;

	InitCommonControls();
	iWindowSetupSystemFontStyle();

	newWindowClass = iWindowCreateClass(hThisInstance, _T("ThNewServer"), newServerWindowCallback);
	newServerWindowCreate(&newWindowClass, hThisInstance, nCmdShow);

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
