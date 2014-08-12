// SoAUpdater.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "UpdaterMethods.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "ZipFile.h"
#include "curl/curl.h"
#include <stdio.h>
#include <direct.h>

#include <dwmapi.h>

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#include "resource.h"


using namespace std;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
HRESULT EnableBlurBehind(HWND hwnd);
HRESULT ExtendIntoClientAll(HWND hwnd);
char * LoadStringFromResource(UINT id);
static int xferinfo(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);


#define IDC_MAIN_BUTTON	101			// Button identifier
#define IDC_MAIN_USERNAME	102	
#define IDC_MAIN_PASSWORD	103	
HWND hUsername;
HWND hPassword;
HWND hProgress;
HINSTANCE hInstance;

char usernameLabel[40];
char passwordLabel[40];


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
	setDlDir("");
	curl_global_init(CURL_GLOBAL_ALL);
	hInstance = hInst;
	WNDCLASSEX wClass;
	ZeroMemory(&wClass, sizeof(WNDCLASSEX));
	wClass.cbClsExtra = NULL;
	wClass.cbSize = sizeof(WNDCLASSEX);
	wClass.cbWndExtra = NULL;
	wClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wClass.hIcon = NULL;
	wClass.hIconSm = NULL;
	wClass.hInstance = hInst;
	wClass.lpfnWndProc = (WNDPROC)WinProc;
	wClass.lpszClassName = "Window Class";
	wClass.lpszMenuName = NULL;
	wClass.style = CS_HREDRAW | CS_VREDRAW;

	if (!RegisterClassEx(&wClass))
	{
		int nResult = GetLastError();
		MessageBox(NULL,
			"Window class creation failed\r\n",
			"Window Class Failed",
			MB_ICONERROR);
	}

	LoadString(hInst, IDS_USERNAMELABEL, usernameLabel, 40);
	LoadString(hInst, IDS_PASSWORDLABEL, passwordLabel, 40);



	HWND hWnd = CreateWindowEx(NULL,
		"Window Class",
		LoadStringFromResource(IDS_WINDOWTITLE),
		WS_OVERLAPPEDWINDOW,
		200,
		200,
		640,
		480,
		NULL,
		NULL,
		hInst,
		NULL);

	if (!hWnd)
	{
		int nResult = GetLastError();

		MessageBox(NULL,
			"Window creation failed\r\n",
			"Window Creation Failed",
			MB_ICONERROR);
	}
	//Cool aero effect, still needs fixing though:
	//EnableBlurBehind(hWnd);
	//ExtendIntoClientAll(hWnd);
	
	ShowWindow(hWnd, nShowCmd);

	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

LRESULT CALLBACK WinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	switch (msg)
	{
	case WM_CREATE:
	{
		// Create an edit box
		hUsername = CreateWindowEx(WS_EX_CLIENTEDGE,
			"EDIT",
			"",
			WS_CHILD | WS_VISIBLE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
			5,
			30,
			200,
			20,
			hWnd,
			(HMENU)IDC_MAIN_USERNAME,
			GetModuleHandle(NULL),
			NULL);
		HGDIOBJ hfDefault = GetStockObject(DEFAULT_GUI_FONT);
		SendMessage(hUsername,
			WM_SETFONT,
			(WPARAM)hfDefault,
			MAKELPARAM(FALSE, 0));

		// Create an edit box
		hPassword = CreateWindowEx(WS_EX_CLIENTEDGE,
			"EDIT",
			"",
			WS_CHILD | WS_VISIBLE |
			ES_PASSWORD | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
			5,
			80,
			200,
			20,
			hWnd,
			(HMENU)IDC_MAIN_PASSWORD,
			GetModuleHandle(NULL),
			NULL);
		SendMessage(hPassword,
			WM_SETFONT,
			(WPARAM)hfDefault,
			MAKELPARAM(FALSE, 0));

		// Create a push button
		HWND hWndButton = CreateWindowEx(NULL,
			"BUTTON",
			LoadStringFromResource(IDS_LOGINBUTTON),
			WS_TABSTOP | WS_VISIBLE |
			WS_CHILD | BS_DEFPUSHBUTTON,
			105,
			100,
			100,
			24,
			hWnd,
			(HMENU)IDC_MAIN_BUTTON,
			GetModuleHandle(NULL),
			NULL);
		SendMessage(hWndButton,
			WM_SETFONT,
			(WPARAM)hfDefault,
			MAKELPARAM(FALSE, 0));
		hProgress = CreateWindowEx(0, PROGRESS_CLASS, (LPTSTR)NULL,
			WS_CHILD | WS_VISIBLE,
			5,
			150,
			200,
			30,
			hWnd, (HMENU)0, GetModuleHandle(NULL), NULL);
		SendMessage(hProgress, PBM_SETRANGE, 0, (LPARAM)100);

		SendMessage(hProgress, PBM_SETSTEP, (WPARAM)1, 0);
	}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_MAIN_BUTTON:
		{
			curlLoadFileFromUrl("http://files.seedofandromeda.com/game/0.1.6/SoA.zip", getDlDir() + "latest.zip", xferinfo);
			char buffer[256];
			SendMessage(hUsername,
				WM_GETTEXT,
				sizeof(buffer) / sizeof(buffer[0]),
				reinterpret_cast<LPARAM>(buffer));
			MessageBox(NULL,
				buffer,
				"Information",
				MB_ICONINFORMATION);
		}
			break;
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		HFONT hFont, hOldFont;
		hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		SetBkMode(hdc, TRANSPARENT);


		// Select the variable stock font into the specified device context. 
		if (hOldFont = (HFONT)SelectObject(hdc, hFont))
		{
			TextOut(hdc,
				5, 5,
				usernameLabel, _tcslen(usernameLabel));
			TextOut(hdc,
				5, 60,
				passwordLabel, _tcslen(passwordLabel));

			// Restore the original font.        
			SelectObject(hdc, hOldFont);
		}




		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

HRESULT EnableBlurBehind(HWND hwnd)
{
	HRESULT hr = S_OK;

	// Create and populate the blur-behind structure.
	DWM_BLURBEHIND bb = { 0 };

	// Specify blur-behind and blur region.
	bb.dwFlags = DWM_BB_ENABLE;
	bb.fEnable = true;
	bb.hRgnBlur = NULL;

	// Enable blur-behind.
	hr = DwmEnableBlurBehindWindow(hwnd, &bb);
	if (SUCCEEDED(hr))
	{
		// ...
	}
	return hr;
}

HRESULT ExtendIntoClientAll(HWND hwnd)
{
	HRESULT hr = S_OK;

	// Negative margins have special meaning to DwmExtendFrameIntoClientArea.
	// Negative margins create the "sheet of glass" effect, where the client 
	// area is rendered as a solid surface without a window border.
	MARGINS margins = { -1 };

	// Extend the frame across the whole window.
	hr = DwmExtendFrameIntoClientArea(hwnd, &margins);
	if (SUCCEEDED(hr))
	{
		// ...
	}
	return hr;
}

char szBuffer[100];
char * LoadStringFromResource(UINT id)
{
	// szBuffer is a globally pre-defined buffer of some maximum length
	LoadString(hInstance, id, szBuffer, 100);
	// yes, I know that strdup has problems. But you get the idea.
	return _strdup(szBuffer);
}


/* this is how the CURLOPT_XFERINFOFUNCTION callback works */
static int xferinfo(void *p,
	curl_off_t dltotal, curl_off_t dlnow,
	curl_off_t ultotal, curl_off_t ulnow)
{
	struct myprogress *myp = (struct myprogress *)p;
	CURL *curl = myp->curl;
	double curtime = 0;
	static int oldDlRatio = -1;

	curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &curtime);

	/* under certain circumstances it may be desirable for certain functionality
	to only run every N seconds, in order to do this the transaction time can
	be used */
	//if ((curtime - myp->lastruntime) >= MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL) {
	//	myp->lastruntime = curtime;
	//	fprintf(stderr, "TOTAL TIME: %f \r\n", curtime);
	//} //dont see why we need this. Delete if you want

	double dlRatio = (double)dlnow / dltotal * 100.0;
	if (dltotal > 0){
		//output is slow as shit, so lets cut out unneeded output.
		if ((int)dlRatio != oldDlRatio){
			oldDlRatio = (int)dlRatio;
			fprintf(stdout, "\r%.0lf%% of %.1f MB", dlRatio, dltotal / 1024.0f / 1024.0f);
			SendMessage(hProgress, PBM_SETPOS, (int)dlRatio, 0);
		}
	}
	//if (dlnow > STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES)
	//	return 1;
	return 0;
}