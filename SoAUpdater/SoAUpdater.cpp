// SoAUpdater.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <iostream>
#include "ZipFile.h"
#include "curl/curl.h"
#include <stdio.h>
using namespace std;
#define READ_SIZE 1024

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
	//}
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

int _tmain(int argc, _TCHAR* argv[])
{
	static const char *bodyfilename = "soa.zip";



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
	// Open the zip file
	unzFile zipfile = unzOpen(bodyfilename);

	if (zipfile == NULL)
	{
		printf("%s: not found\n");
		return -1;
	}

	// Get info about the zip file
	unz_global_info global_info;
	if (unzGetGlobalInfo(zipfile, &global_info) != UNZ_OK)
	{
		printf("could not read file global info\n");
		unzClose(zipfile);
		return -1;
	}

	// Buffer to hold data read from the zip file.
	char read_buffer[READ_SIZE];

	// Loop to extract all files
	uLong i;
	for (i = 0; i < global_info.number_entry; ++i)
	{
		// Get info about current file.
		unz_file_info file_info;
		char filename[FILENAME_MAX];
		if (unzGetCurrentFileInfo(
			zipfile,
			&file_info,
			filename,
			FILENAME_MAX,
			NULL, 0, NULL, 0) != UNZ_OK)
		{
			printf("could not read file info\n");
			unzClose(zipfile);
			return -1;
		}

		// Check if this entry is a directory or file.
		const size_t filename_length = strlen(filename);
		if (filename[filename_length - 1] == '/')
		{
			// Entry is a directory, so create it.
			printf("dir:%s\n", filename);
			size_t origsize = strlen(filename) + 1;
			const size_t newsize = 100;
			size_t convertedChars = 0;
			wchar_t wcstring[newsize];
			mbstowcs_s(&convertedChars, wcstring, origsize - 1, filename, _TRUNCATE);
			if (!CreateDirectory(wcstring, NULL)){
				LPVOID lpMsgBuf;
				DWORD dw = GetLastError();

				FormatMessage(
					FORMAT_MESSAGE_ALLOCATE_BUFFER |
					FORMAT_MESSAGE_FROM_SYSTEM |
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					dw,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR)&lpMsgBuf,
					0, NULL);
				_tprintf(L"%s", lpMsgBuf);
			}
		}
		else
		{
			// Entry is a file, so extract it.
			printf("file:%s\n", filename);
			if (unzOpenCurrentFile(zipfile) != UNZ_OK)
			{
				printf("could not open file\n");
				unzClose(zipfile);
				return -1;
			}

			// Open a file to write out the data.
			FILE *out = fopen(filename, "wb");
			if (out == NULL)
			{
				printf("could not open destination file\n");
				unzCloseCurrentFile(zipfile);
				unzClose(zipfile);
				return -1;
			}

			int error = UNZ_OK;
			do
			{
				error = unzReadCurrentFile(zipfile, read_buffer, READ_SIZE);
				if (error < 0)
				{
					printf("error %d\n", error);
					unzCloseCurrentFile(zipfile);
					unzClose(zipfile);
					return -1;
				}

				// Write data to file.
				if (error > 0)
				{
					fwrite(read_buffer, error, 1, out); // You should check return of fwrite...
				}
			} while (error > 0);

			fclose(out);
		}

		unzCloseCurrentFile(zipfile);

		// Go the the next entry listed in the zip file.
		if ((i + 1) < global_info.number_entry)
		{
			if (unzGoToNextFile(zipfile) != UNZ_OK)
			{
				printf("cound not read next file\n");
				unzClose(zipfile);
				return -1;
			}
		}
	}

	unzClose(zipfile);

	return 0;
}

