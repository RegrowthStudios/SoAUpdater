#include "stdafx.h"
#include "UpdaterMethods.h";
#include <iostream>
#include <fstream>
#include <sstream>
#include "ZipFile.h"
#include "curl/curl.h"
#include <stdio.h>
#include <direct.h>

string dldir;

void setDlDir(string forcedDir){
	if (!forcedDir.empty()){
		dldir = forcedDir;
	}
	else{
		dldir = "%appdata%\\SoA\\";
		wstring temp = ExpandEnvStrings(L"%appdata%\\SoA\\");
		dldir.resize(temp.size()); //make enough room in copy for the string 
		std::copy(temp.begin(), temp.end(), dldir.begin()); //copy it 
		_mkdir(dldir.c_str());
	}
}

string getDlDir(){
	if (dldir.empty()){
		//This should not happen
		setDlDir();
	}
	else{
		return dldir;
	}
}


login_info checkLogin(){
	return parseUserData(curlLoadStringFromUrl("http://www.seedofandromeda.com/updater/auth.php?action=checklogin")); //TODO: Change to libcurl with ssl so https can be used
}
login_info doLogin(string username, string password){
	string post = string("username=") + username + "&password=" + password;
	return parseUserData(curlLoadStringFromUrl("http://www.seedofandromeda.com/updater/auth.php?action=login", post)); //TODO: Change to libcurl with ssl so https can be used
}

login_info parseUserData(string response){
	login_info ret;
	int i = 0;
	stringstream strstrm(response);
	string line;
	while (getline(strstrm, line)){
		switch (i){
		case 0:
			if (line == "OK"){
				ret.success = true;
			}
			break;
		case 1:
			if (!ret.success){
				ret.errorMsg = line;
				return ret;
			}
			else{
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



size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t written;
	written = fwrite(ptr, size, nmemb, stream);
	return written;
}

int curlLoadFileFromUrl(string url, string savefilename, int(*xferinfo)(void(*), curl_off_t, curl_off_t, curl_off_t, curl_off_t))
{
	CURL *curl_handle;
	struct myprogress prog;

	
	/* init the curl session */
	curl_handle = curl_easy_init();
	if (curl_handle){
		prog.lastruntime = 0;
		prog.curl = curl_handle;
		/* set URL to get */
		curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());

		/* no progress meter please */
		curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl_handle, CURLOPT_XFERINFOFUNCTION, xferinfo);
		/* pass the struct pointer into the xferinfo function, note that this is
		an alias to CURLOPT_PROGRESSDATA */
		curl_easy_setopt(curl_handle, CURLOPT_XFERINFODATA, &prog);

		/* send all data to this function  */
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);

		/* we tell libcurl to follow redirection */
		curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);

		/* open the file */
		FILE *bodyfile;
		fopen_s(&bodyfile, savefilename.c_str(), "wb");
		if (bodyfile == NULL) {
			curl_easy_cleanup(curl_handle);
			return -1;
		}

		/* we want the body be written to this file handle instead of stdout */
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, bodyfile);
		CURLcode res = CURLE_OK;
		/* get it! */
		cout << endl;
		res = curl_easy_perform(curl_handle);
		cout << endl;

		/* close the body file */
		fclose(bodyfile);

		/* cleanup curl stuff */
		curl_easy_cleanup(curl_handle);
		if (res != CURLE_OK){
			fprintf(stderr, "%s\n", curl_easy_strerror(res));
			return -1;
		}
	}
	else{
		return -1;
	}
}

string curlLoadStringFromUrl(string url, string postfields)
{
	string readBuffer;
	CURL *curl;
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_COOKIEFILE, dldir + "session.txt");
		curl_easy_setopt(curl, CURLOPT_COOKIEJAR, dldir + "session.txt");
		if (!postfields.empty()){
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields.c_str());
		}

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToStringCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}
	return readBuffer;
}

size_t WriteToStringCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}


std::wstring ExpandEnvStrings(const std::wstring& source)
{
	DWORD len;
	std::wstring result;
	len = ::ExpandEnvironmentStringsW(source.c_str(), 0, 0);
	if (len == 0)
	{
		return result;
	}
	result.resize(len);
	len = ::ExpandEnvironmentStringsW(source.c_str(), &result[0], len);
	if (len == 0)
	{
		return result;
	}
	result.pop_back(); //Get rid of extra null
	return result;
}