#include "stdafx.h"
#include <iostream>
#include "curl/curl.h"



//Version of this release
const int VERSION = 4;



std::string getDlDir();
void setDlDir(std::string forcedDir = false);
int curlLoadFileFromUrl(std::string url, std::string savefilename, int(*xferinfo)(void(*), curl_off_t, curl_off_t, curl_off_t, curl_off_t));
std::string curlLoadStringFromUrl(std::string url, std::string postfields = "");
std::wstring ExpandEnvStrings(const std::wstring& source);
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t WriteToStringCallback(void *contents, size_t size, size_t nmemb, void *userp);

struct myprogress {
	double lastruntime;
	CURL *curl;
};