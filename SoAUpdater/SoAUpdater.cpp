// SoAUpdater.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "ZipFile.h"
#include "curl/curl.h"
#include <stdio.h>
#include <direct.h>

using namespace std;

//prototypes

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);
static int xferinfo(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
int curlLoadFileFromUrl(string url, string savefilename);
std::wstring ExpandEnvStrings(const std::wstring& source);


//I like to have main at the top. call me crazy
int _tmain(int argc, _TCHAR* argv[])
{
	string dldir = "";
	bool useappdata = false;
	ifstream testfile("version.txt");
	if (!testfile){
		dldir = "%appdata%\\SoA\\";

		wstring temp = ExpandEnvStrings(L"%appdata%\\SoA\\");
		dldir.resize(temp.size()); //make enough room in copy for the string 
		std::copy(temp.begin(), temp.end(), dldir.begin()); //copy it 
		cout << "version.txt was not found here, trying " << dldir << endl;
		useappdata = true;
		_mkdir(dldir.c_str());
	}
	else{
		testfile.close();
	}
	ifstream ifile(dldir + "version.txt");
	std::string line;
	int version = 0;
	int i = 0;
	if (ifile) {
		while (std::getline(ifile, line)) {
			std::cout << "Current version line " << i << ": " << line << endl;
			version = atoi(line.c_str());
			break;
			i++;
		}
		ifile.close();
	}
	std::cout << "Checking for updates... Current version is " << version << endl;
	std::stringstream urlstrm;
	urlstrm << "http://seedofandromeda.com/update.php?version=" << version;
	string url = urlstrm.str();
	cout << "Downloading from " << url << endl;
	curlLoadFileFromUrl(url, dldir + "latest.txt");

	std::ifstream input(dldir + "latest.txt");
	if (!input){
		cout << "latest.txt did not get downloaded!" << endl;
		return -1;
	}
	i = 0;
	bool update = false;
	while (std::getline(input, line)) {
		std::cout << "Latest version line " << i << ": " << line << std::endl;
		if (i == 0){
			if (atoi(line.c_str()) > version) {
				cout << "Update found, downloading..." << endl;
			}
			else{
				cout << "No update found!" << endl;
				break;
			}
		}
		else if (i == 1){
			curlLoadFileFromUrl(line, dldir + "latest.zip");
			update = true;
			break;
		}
		i++;
	}
	input.close();
	if (update){
		//open zip file
		ZipFile zipFile(dldir + "latest.zip");
		if (zipFile.fail()){
			printf("%s: not found\n", dldir + "latest.zip");
			return -1;
		}

		//extract zip file
		if (zipFile.extractZip(dldir) != 0){
			printf("failed to extract zip file.");
			int a;
			cin >> a;
			return -2;
		}
		else{
			if (remove((dldir + "latest.zip").c_str()) != 0){
				cout << "Failed to remove latest.txt!" << endl;
			}
			if (remove((dldir + "version.txt").c_str()) != 0){
				cout << "Failed to remove latest.txt!" << endl;
			}
			if (rename((dldir + "latest.txt").c_str(), (dldir + "version.txt").c_str()) != 0){
				cout << "Failed to rename latest.txt to version.txt!" << endl;
			}
		}
	}
	cout << "Starting up SoA..." << endl;
	_chdir((dldir + "Release\\").c_str());
	system((dldir + "Release\\SoA.exe").c_str());
	return 0;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t written;
	written = fwrite(ptr, size, nmemb, stream);
	return written;
}

struct myprogress {
	double lastruntime;
	CURL *curl;
};

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
			fprintf(stdout, "%.0lf%% of %.1f MB\n", dlRatio, dltotal / 1024.0f / 1024.0f);
		}
	}
	//if (dlnow > STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES)
	//	return 1;
	return 0;
}

int curlLoadFileFromUrl(string url, string savefilename)
{
	CURL *curl_handle;
	struct myprogress prog;

	FILE *bodyfile;

	curl_global_init(CURL_GLOBAL_ALL);

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
		fopen_s(&bodyfile, savefilename.c_str(), "wb");
		if (bodyfile == NULL) {
			curl_easy_cleanup(curl_handle);
			return -1;
		}

		/* we want the body be written to this file handle instead of stdout */
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, bodyfile);
		CURLcode res = CURLE_OK;
		/* get it! */
		res = curl_easy_perform(curl_handle);

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