#include "stdafx.h"
#include <iostream>



//Version of this release
const int VERSION = 6;

struct login_info {
    bool success;
    std::string errorMsg;
    int userID;
    std::string username;
    std::string email;
    std::string custom_title;
};

struct version_info {
    int gameVersion;
    std::string gameUrl;
    int updaterVersion;
    std::string updaterUrl;
};



std::string getDlDir();
void setDlDir(std::string forcedDir = "");
login_info checkLogin();
login_info doLogin(std::string username, std::string password);
login_info parseUserData(std::string response);
version_info checkLatestVersion(int gameVersion);
version_info parseVersionInfo(std::string response);
version_info parseVersionInfo(std::istream &strm);
version_info readVersionFile();
void writeVersionFile(version_info ver);
void runSoA(int version);


int curlLoadFileFromUrl(std::string url, std::string savefilename);
std::string curlLoadStringFromUrl(std::string url, std::string postfields = "");
std::wstring ExpandEnvStrings(const std::wstring& source);
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t WriteToStringCallback(void *contents, size_t size, size_t nmemb, void *userp);

