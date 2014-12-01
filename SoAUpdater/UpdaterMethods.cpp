#include "stdafx.h"
#include "UpdaterMethods.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <direct.h>

using namespace std;

string dldir;

void setDlDir(string forcedDir) {
    if (!forcedDir.empty()) {
        dldir = forcedDir;
    } else {
        dldir = "%appdata%\\SoA\\";
        wstring temp = ExpandEnvStrings(L"%appdata%\\SoA\\");
        dldir.resize(temp.size()); //make enough room in copy for the string 
        std::copy(temp.begin(), temp.end(), dldir.begin()); //copy it 
        _mkdir(dldir.c_str());
    }
}

string getDlDir() {
    if (dldir.empty()) {
        //This should not happen
        setDlDir();
    }
    return dldir;
}


login_info checkLogin() {
    return parseUserData(curlLoadStringFromUrl("https://www.seedofandromeda.com/updater/auth.php?action=checklogin")); //TODO: Change to libcurl with ssl so https can be used
}
login_info doLogin(string username, string password) {
    string post = string("username=") + username + "&password=" + password;
    return parseUserData(curlLoadStringFromUrl("https://www.seedofandromeda.com/updater/auth.php?action=login", post)); //TODO: Change to libcurl with ssl so https can be used
}

login_info parseUserData(string response) {
    login_info ret;
    int i = 0;
    stringstream strstrm(response);
    string line;
    while (getline(strstrm, line)) {
        switch (i) {
        case 0:
            ret.success = line == "OK";
            break;
        case 1:
            if (!ret.success) {
                ret.errorMsg = line;
                return ret;
            } else {
                ret.userID = atoi(line.c_str());
            }
            break;

        case 2:
            ret.username = line;
            break;
        case 3:
            ret.email = line;
            break;
        case 4:
            ret.custom_title = line;
            break;
        }
        i++;
    }
    return ret;
}

version_info checkLatestVersion(int gameVersion) {
    return parseVersionInfo(curlLoadStringFromUrl("https://www.seedofandromeda.com/updater/update.php?updaterversion=" + to_string(VERSION) + "&gameversion=" + to_string(gameVersion)));
}



version_info parseVersionInfo(string response) {

    stringstream strstrm(response);
    return parseVersionInfo(strstrm);
}
version_info parseVersionInfo(std::istream &strm) {
    version_info ret;

    int i = 0;
    string line;
    while (getline(strm, line)) {
        switch (i) {
        case 0:
            ret.gameVersion = atoi(line.c_str());
            break;
        case 1:
            ret.gameUrl = line;
            break;
        case 2:
            ret.updaterVersion = atoi(line.c_str());
            break;
        case 3:
            ret.updaterUrl = line;
            break;
        }
        i++;
    }
    return ret;
}

version_info readVersionFile() {
    version_info ret;
    ifstream verfile;
    verfile.open(dldir + "version.txt");
    ret = parseVersionInfo(verfile);
    verfile.close();
    return ret;
}

void writeVersionFile(version_info ver) {
    ofstream verfile;
    verfile.open(dldir + "version.txt");
    verfile << ver.gameVersion << endl << ver.gameUrl << endl << ver.updaterVersion << endl << ver.updaterUrl;
    verfile.close();
}

void runSoA(int version) {
    _chdir((dldir + "\\" + to_string(version) + "\\Release\\").c_str());
    system("SOA.exe");
}



int curlLoadFileFromUrl(string url, string savefilename) {

        return -1;
}

string curlLoadStringFromUrl(string url, string postfields) {
    string readBuffer;

    return readBuffer;
}



std::wstring ExpandEnvStrings(const std::wstring& source) {
    DWORD len;
    std::wstring result;
    len = ::ExpandEnvironmentStringsW(source.c_str(), 0, 0);
    if (len == 0) {
        return result;
    }
    result.resize(len);
    len = ::ExpandEnvironmentStringsW(source.c_str(), &result[0], len);
    if (len == 0) {
        return result;
    }
    result.pop_back(); //Get rid of extra null
    return result;
}