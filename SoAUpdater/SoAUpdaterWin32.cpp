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
#include <thread>


using namespace std;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void DoCreateStatusBar(HWND hwndParent, int idStatus, HINSTANCE
    hinst, int cParts);
HRESULT EnableBlurBehind(HWND hwnd);
HRESULT ExtendIntoClientAll(HWND hwnd);
char * LoadStringFromResource(UINT id);
static int xferinfo(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
DWORD WINAPI CheckLoginThread(LPVOID lpParam);
DWORD WINAPI DoLoginThread(LPVOID lpParam);
DWORD WINAPI DownloadThread(LPVOID lpParam);


#define IDC_MAIN_BUTTON			101	
#define IDC_MAIN_USERNAME		102	
#define IDC_MAIN_PASSWORD		103	
#define IDC_MAIN_STATUSBAR		104	
#define IDC_MAIN_LAUNCH_BUTTON	105	
HWND hWnd;
HWND hUsername;
HWND hPassword;
HWND hProgress;
HWND hStatus;
HANDLE dlHandle;
HANDLE doLoginHandle;
HANDLE checkLoginHandle;

char usernameLabel[40];
char passwordLabel[40];

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd) {
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_WNDW);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_WNDW);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_WNDW);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    setDlDir("");
    curl_global_init(CURL_GLOBAL_ALL);
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

    if (!RegisterClassEx(&wClass)) {
        int nResult = GetLastError();
        MessageBox(NULL,
            "Window class creation failed\r\n",
            "Window Class Failed",
            MB_ICONERROR);
    }

    LoadString(hInst, IDS_USERNAMELABEL, usernameLabel, 40);
    LoadString(hInst, IDS_PASSWORDLABEL, passwordLabel, 40);



    hWnd = CreateWindowEx(NULL,
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

    if (!hWnd) {
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
    //thread th(&curlLoadFileFromUrl, "http://files.seedofandromeda.com/game/0.1.6/SoA.zip", getDlDir() + "latest.zip", xferinfo);
    checkLoginHandle = CreateThread(NULL, 0,
        CheckLoginThread, NULL, 0, NULL);
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PAINTSTRUCT ps;
    HDC hdc;
    switch (msg) {
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


        HWND hWndLaunchButton = CreateWindowEx(NULL,
            "BUTTON",
            LoadStringFromResource(IDS_LAUNCHBUTTON),
            WS_TABSTOP | WS_VISIBLE |
            WS_CHILD | BS_DEFPUSHBUTTON,
            105,
            140,
            100,
            24,
            hWnd,
            (HMENU)IDC_MAIN_LAUNCH_BUTTON,
            GetModuleHandle(NULL),
            NULL);
        SendMessage(hWndLaunchButton,
            WM_SETFONT,
            (WPARAM)hfDefault,
            MAKELPARAM(FALSE, 0));


        DoCreateStatusBar(hWnd, IDC_MAIN_STATUSBAR, GetModuleHandle(NULL), 2);

    }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_MAIN_BUTTON:
        {

            doLoginHandle = CreateThread(NULL, 0,
                DoLoginThread, NULL, 0, NULL);

            //MessageBox(NULL,
            //	buffer,
            //	"Information",
            //	MB_ICONINFORMATION);
        }
            break;
        case IDC_MAIN_LAUNCH_BUTTON:
        {
            dlHandle = CreateThread(NULL, 0,
                DownloadThread, NULL, 0, NULL);
            //char buffer[256];
            //SendMessage(hUsername,
            //	WM_GETTEXT,
            //	sizeof(buffer) / sizeof(buffer[0]),
            //	reinterpret_cast<LPARAM>(buffer));
            //MessageBox(NULL,
            //	buffer,
            //	"Information",
            //	MB_ICONINFORMATION);
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
        if (hOldFont = (HFONT)SelectObject(hdc, hFont)) {
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

HRESULT EnableBlurBehind(HWND hwnd) {
    HRESULT hr = S_OK;

    // Create and populate the blur-behind structure.
    DWM_BLURBEHIND bb = {0};

    // Specify blur-behind and blur region.
    bb.dwFlags = DWM_BB_ENABLE;
    bb.fEnable = true;
    bb.hRgnBlur = NULL;

    // Enable blur-behind.
    hr = DwmEnableBlurBehindWindow(hwnd, &bb);
    if (SUCCEEDED(hr)) {
        // ...
    }
    return hr;
}

HRESULT ExtendIntoClientAll(HWND hwnd) {
    HRESULT hr = S_OK;

    // Negative margins have special meaning to DwmExtendFrameIntoClientArea.
    // Negative margins create the "sheet of glass" effect, where the client 
    // area is rendered as a solid surface without a window border.
    MARGINS margins = {-1};

    // Extend the frame across the whole window.
    hr = DwmExtendFrameIntoClientArea(hwnd, &margins);
    if (SUCCEEDED(hr)) {
        // ...
    }
    return hr;
}


// Description: 
//   Creates a status bar and divides it into the specified number of parts.
// Parameters:
//   hwndParent - parent window for the status bar.
//   idStatus - child window identifier of the status bar.
//   hinst - handle to the application instance.
//   cParts - number of parts into which to divide the status bar.
// Returns:
//   The handle to the status bar.
//
void DoCreateStatusBar(HWND hwndParent, int idStatus, HINSTANCE
    hinst, int cParts) {
    RECT rcClient;
    HLOCAL hloc;
    PINT paParts;
    int i, nWidth;

    // Ensure that the common control DLL is loaded.
    //InitCommonControls();

    // Create the status bar.
    hStatus = CreateWindowEx(
        0,                       // no extended styles
        STATUSCLASSNAME,         // name of status bar class
        (PCTSTR)"Ready.",           // no text when first created
        SBARS_SIZEGRIP |         // includes a sizing grip
        WS_CHILD | WS_VISIBLE,   // creates a visible child window
        0, 0, 0, 0,              // ignores size and position
        hwndParent,              // handle to parent window
        (HMENU)idStatus,       // child window identifier
        hinst,                   // handle to application instance
        NULL);                   // no window creation data

    // Get the coordinates of the parent window's client area.
    GetClientRect(hwndParent, &rcClient);

    // Allocate an array for holding the right edge coordinates.
    hloc = LocalAlloc(LHND, sizeof(int) * cParts);
    paParts = (PINT)LocalLock(hloc);

    // Calculate the right edge coordinate for each part, and
    // copy the coordinates to the array.
    nWidth = rcClient.right / cParts;
    int rightEdge = nWidth;
    for (i = 0; i < cParts; i++) {
        paParts[i] = rightEdge;
        OutputDebugString(to_string(rightEdge).c_str());
        rightEdge += nWidth;
    }

    // Tell the status bar to create the window parts.
    SendMessage(hStatus, SB_SETPARTS, (WPARAM)cParts, (LPARAM)
        paParts);
    hProgress = CreateWindowEx(0, PROGRESS_CLASS, (LPTSTR)NULL,
        WS_CHILD | WS_VISIBLE,
        paParts[0],
        0,
        200,
        rcClient.bottom - rcClient.top,
        hStatus, (HMENU)0, GetModuleHandle(NULL), NULL);
    // Free the array, and return.
    LocalUnlock(hloc);
    LocalFree(hloc);
}

char szBuffer[100];
char * LoadStringFromResource(UINT id) {
    // szBuffer is a globally pre-defined buffer of some maximum length
    LoadString(GetModuleHandle(NULL), id, szBuffer, 100);
    // yes, I know that strdup has problems. But you get the idea.
    return _strdup(szBuffer);
}


/* this is how the CURLOPT_XFERINFOFUNCTION callback works */
static int xferinfo(void *p,
    curl_off_t dltotal, curl_off_t dlnow,
    curl_off_t ultotal, curl_off_t ulnow) {
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
    if (dltotal > 0) {
        //output is slow as shit, so lets cut out unneeded output.
        if ((int)dlRatio != oldDlRatio) {
            oldDlRatio = (int)dlRatio;
            SendMessage(hProgress, PBM_SETPOS, dlRatio, 0);
        }
    }
    //if (dlnow > STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES)
    //	return 1;
    return 0;
}

DWORD WINAPI CheckLoginThread(LPVOID lpParam) {
    SendMessage(hStatus,
        WM_SETTEXT,
        NULL,
        (LPARAM)"Checking for login");
    login_info login = checkLogin();
    if (!login.success) {
        SendMessage(hStatus,
            WM_SETTEXT,
            NULL,
            (LPARAM)((string)"Login Error: " + login.errorMsg).c_str());
    } else {
        SendMessage(hStatus,
            WM_SETTEXT,
            NULL,
            (LPARAM)((string)"Login successful! Username: " + login.username + ", Email: " + login.email + ", Title: " + login.custom_title + ", ID: " + to_string(login.userID)).c_str());
        SendMessage(hUsername,
            WM_SETTEXT,
            NULL,
            (LPARAM)login.username.c_str());
    }
    return 0;
}

DWORD WINAPI DoLoginThread(LPVOID lpParam) {

    SendMessage(hStatus,
        WM_SETTEXT,
        NULL,
        (LPARAM)"Trying to login");
    char username[100];
    SendMessage(hUsername,
        WM_GETTEXT,
        sizeof(username) / sizeof(username[0]),
        reinterpret_cast<LPARAM>(username));
    char password[100];
    SendMessage(hPassword,
        WM_GETTEXT,
        sizeof(password) / sizeof(password[0]),
        reinterpret_cast<LPARAM>(password));
    login_info login = doLogin(username, password);
    if (!login.success) {
        SendMessage(hStatus,
            WM_SETTEXT,
            NULL,
            (LPARAM)((string)"Login Error: " + login.errorMsg).c_str());
    } else {
        SendMessage(hStatus,
            WM_SETTEXT,
            NULL,
            (LPARAM)((string)"Login successful! Username: " + login.username + ", Email: " + login.email + ", Title: " + login.custom_title + ", ID: " + to_string(login.userID)).c_str());
        SendMessage(hUsername,
            WM_SETTEXT,
            NULL,
            (LPARAM)login.username.c_str());
    }
    return 0;
}

DWORD WINAPI DownloadThread(LPVOID lpParam) {
    //version_info ver = *((version_info*)lpParam);

    string dldir = getDlDir();

    version_info verfile = readVersionFile();
    if (verfile.updaterVersion > VERSION) {
        ifstream newupdater(dldir + "SoAUpdater.exe");
        if (newupdater) {
            newupdater.close();
            cout << "Newer version the updater (" << verfile.updaterVersion << " vs " << VERSION << ") found in " << dldir << endl;
        } else {
            verfile.updaterVersion = 0;
        }
    }

    SendMessage(hStatus,
        WM_SETTEXT,
        NULL,
        (LPARAM)"Checking for game updates...");

    version_info ver = checkLatestVersion(verfile.gameVersion);

    bool update = false;
    bool updatelauncher = false;
    if (ver.gameVersion > verfile.gameVersion) {
        SendMessage(hStatus,
            WM_SETTEXT,
            NULL,
            (LPARAM)"Update found, downloading...");
        update = true;
    }

    if (ver.updaterVersion > VERSION) {
        if (ver.updaterVersion == verfile.updaterVersion) {
            cout << "Launching updater from " << dldir << endl;
            _chdir((dldir).c_str());
            system((dldir + "SoAUpdater.exe -noappdata").c_str());
            return 0;
        } else
            if (ver.updaterVersion > verfile.updaterVersion) {
            updatelauncher = true;
            } else {
            MessageBox(hWnd, " <<< New version of SoAUpdater is available to download. Please update as soon as possible. >>>", "Attention", 0);
            }
    }
    int curlout;
    if (updatelauncher) {
        SendMessage(hStatus,
            WM_SETTEXT,
            NULL,
            (LPARAM)"Updating launcher...");
        curlout = curlLoadFileFromUrl(ver.updaterUrl, dldir + "SoAUpdater.exe", xferinfo);
        cout << "cURL returns " << curlout << endl;
        if (!update) {
            //If no SoA update is available, update version.txt to prevent downloading the launcher on every launch
            writeVersionFile(ver);
        }
        cout << "Launching updater from " << dldir << endl;
        _chdir((dldir).c_str());
        system((dldir + "SoAUpdater.exe -noappdata").c_str());
        return 0;
    }

    if (update) {
        SendMessage(hStatus,
            WM_SETTEXT,
            NULL,
            (LPARAM)"Downloading update...");
        curlout = curlLoadFileFromUrl(ver.gameUrl, dldir + "latest.zip", xferinfo);
        cout << "cURL returns " << curlout << endl;
        SendMessage(hStatus,
            WM_SETTEXT,
            NULL,
            (LPARAM)"Unpacking the update...");
        //open zip file
        ZipFile zipFile(dldir + "latest.zip");
        if (zipFile.fail()) {
            printf("%s: not found\n", dldir + "latest.zip");
            system("pause");
            return -1;
        }

        //extract zip file
        if (zipFile.extractZip(dldir + "\\" + to_string(ver.gameVersion) + "\\") != 0) {
            printf("failed to extract zip file.");
            system("pause");
            return -2;
        } else {
            //TODO: Close the zip file so it can be deleted!
            //zipFile.~ZipFile();
            writeVersionFile(ver);
        }
    }
    SendMessage(hStatus,
        WM_SETTEXT,
        NULL,
        (LPARAM)"Starting up SoA...");
    runSoA(ver.gameVersion);

    return 0;
}