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

using namespace std;

//prototypes

static int xferinfo(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

void SetStdinEcho(bool enable);


//I like to have main at the top. call me crazy
int main(int argc, char* argv[]) {
    setDlDir();
    curl_global_init(CURL_GLOBAL_ALL);

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

    std::cout << "Checking for game updates... Current version is " << verfile.gameVersion << endl;

    version_info ver = checkLatestVersion(verfile.gameVersion);

    bool update = false;
    bool updatelauncher = false;
    if (ver.gameVersion > verfile.gameVersion) {
        cout << "Update found, downloading..." << endl;
        update = true;
    } else {
        cout << "No update found!" << endl;
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
            cout << endl << " <<< New version of SoAUpdater is available to download. Please update as soon as possible. >>>" << endl << endl;
            system("pause");
            }
    }
    int curlout;
    if (updatelauncher) {
        cout << "Updating launcher at " << dldir << endl;
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

    login_info login = checkLogin();
    string username, password;
    if (!login.success) {
        cout << endl << endl << "Please login with your SoA forum account or press enter to skip login" << endl;
        cout << "Username: ";
        getline(cin, username);
        if (username.length() > 1) {
            cout << "Password: ";
            SetStdinEcho(false);
            getline(cin, password);
            SetStdinEcho(true);
            cout << endl;
            cout << "Trying to log in..." << endl;
            login = doLogin(username, password);
        }

    }
    if (login.success) {
        cout << "Hello " << login.username << ", " << login.custom_title << ", " << login.email << "! You have been logged in. Your user id seems to be " << login.userID << endl;
    }

    if (update) {
        curlout = curlLoadFileFromUrl(ver.gameUrl, dldir + "latest.zip", xferinfo);
        cout << "cURL returns " << curlout << endl;
        cout << "Unpacking the update..." << endl;
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
    cout << "Starting up SoA..." << endl;
    runSoA(ver.gameVersion);
    return 0;
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
            fprintf(stdout, "\r%.0lf%% of %.1f MB", dlRatio, dltotal / 1024.0f / 1024.0f);
        }
    }
    //if (dlnow > STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES)
    //	return 1;
    return 0;
}


void SetStdinEcho(bool enable = true) {
#ifdef WIN32
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hStdin, &mode);

    if (!enable)
        mode &= ~ENABLE_ECHO_INPUT;
    else
        mode |= ENABLE_ECHO_INPUT;

    SetConsoleMode(hStdin, mode);

#else
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    if (!enable)
        tty.c_lflag &= ~ECHO;
    else
        tty.c_lflag |= ECHO;

    (void)tcsetattr(STDIN_FILENO, TCSANOW, &tty);
#endif
}