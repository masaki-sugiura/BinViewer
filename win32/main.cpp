// $Id$

#pragma warning(disable : 4786)

#define _WIN32_WINNT  0x500  // to support mouse wheel

#include <windows.h>
#include <commctrl.h>
#include <assert.h>

#include "MainWindow.h"

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
		LPSTR lpszCmdLine, int nCmdShow)
{
	::InitCommonControls();

	try {
		MainWindow mainWnd(hInstance, __argv[1]);

		mainWnd.show(nCmdShow);

		return mainWnd.doModal();

	} catch (...) {
		::MessageBox(NULL, "Failed to ...", NULL, MB_OK);
	}

	return 1;
}

