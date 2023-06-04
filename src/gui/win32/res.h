#ifndef NEW_TH_GUI_RES_H
#define NEW_TH_GUI_RES_H

#pragma warning(disable:4068)

#include <windows.h>

#pragma region Normal Widget Types

#define NORMAL_WIDGET (WS_VISIBLE | WS_CHILD)

#define NORMAL_BUTTON (NORMAL_WIDGET | BS_DEFPUSHBUTTON | WS_TABSTOP)

#define NORMAL_GROUPBOX (NORMAL_WIDGET | BS_GROUPBOX)

#define NORMAL_LABEL (NORMAL_WIDGET | SS_CENTERIMAGE)

#define NORMAL_ENTRY (NORMAL_WIDGET | ES_AUTOHSCROLL)

#define NORMAL_RADIO (NORMAL_WIDGET | BS_AUTORADIOBUTTON)

#pragma endregion

#pragma region Widget IDs

#define BTN_START (100)

#define BTN_BROWSE (101)

#define EDT_ROOTPATH (102)

#pragma endregion

#pragma region Legacy toolchain helpers

#ifndef BIF_NEWDIALOGSTYLE
#define BIF_NEWDIALOGSTYLE 0x00000040
#endif

#ifndef BIF_NONEWFOLDERBUTTON
#define BIF_NONEWFOLDERBUTTON 0x00000200
#endif

#pragma endregion

#endif /* NEW_TH_GUI_RES_H */
