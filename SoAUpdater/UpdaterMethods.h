#include "stdafx.h"
#include <iostream>
#include "curl/curl.h"



//Version of this release
const int VERSION = 4;


struct myprogress {
	double lastruntime;
	CURL *curl;
};

struct login_info {
	bool success;
	std::string errorMsg;
	int userID;
	std::string username;
	std::string email;
	std::string custom_title;
};



std::string getDlDir();
void setDlDir(std::string forcedDir = false);
login_info checkLogin();
login_info doLogin(std::string username, std::string password);
login_info parseUserData(std::string response);
int curlLoadFileFromUrl(std::string url, std::string savefilename, int(*xferinfo)(void(*), curl_off_t, curl_off_t, curl_off_t, curl_off_t));
std::string curlLoadStringFromUrl(std::string url, std::string postfields = "");
std::wstring ExpandEnvStrings(const std::wstring& source);
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t WriteToStringCallback(void *contents, size_t size, size_t nmemb, void *userp);

