// SoAUpdater.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <iostream>
#include "ZipFile.h"
#include "curl/curl.h"
#include <stdio.h>

using namespace std;

//prototypes

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);
static int xferinfo(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
int curlLoadFileFromUrl(const char *url);

const char *bodyfilename = "soa.zip";

//I like to have main at the top. call me crazy
int _tmain(int argc, _TCHAR* argv[])
{
	curlLoadFileFromUrl("http://seedofandromeda.com/SeedofAndromeda/Game/Versions/0.1.5/SOA_0.1.5.zip");

	//open zip file
	ZipFile zipFile(bodyfilename);
	if (zipFile.fail()){
		printf("%s: not found\n", bodyfilename);
		return -1;
	}

	//extract zip file
	if (zipFile.extractZip("test/") != 0){
		printf("failed to extract zip file.");
		int a;
		cin >> a;
		return -2;
	}

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
			fprintf(stdout, "%.0lf%% of %.1f MB\n", dlRatio, dltotal/1024.0f/1024.0f);
		}
	}
	//if (dlnow > STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES)
	//	return 1;
	return 0;
}

int curlLoadFileFromUrl(const char *url)
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
		curl_easy_setopt(curl_handle, CURLOPT_URL, "http://seedofandromeda.com/SeedofAndromeda/Game/Versions/0.1.5/SOA_0.1.5.zip");

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
		fopen_s(&bodyfile, bodyfilename, "wb");
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