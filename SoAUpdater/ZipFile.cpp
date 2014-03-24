//****** ZipFile.cpp ******
#include "stdafx.h"
#include "ZipFile.h"
#include <ZLIB/unzip.c>
#include <ZLIB/ioapi.c>
#include <iostream>
using namespace std;



//constructor for the zipfile. You must initialize it with the zipfile name
ZipFile::ZipFile(string fileName) : zipfile(NULL), failure(0) // <-initialization list. Its a fast way to initialize variables
{
	//open the zipfile
	zipfile = unzOpen(fileName.c_str());

	//unzOpen returns NULL when it fails, so it will set failure to 1.
	if (zipfile == NULL)
	{
		failure = 1;
		//Error is a function I created. You can make your own error function that just does a printf or dumps to a file
		printf((fileName + " not found\n").c_str());
		return;
	}

	//grabs the zipfile info and error checks it
	if (unzGetGlobalInfo(zipfile, &global_info) != UNZ_OK)
	{
		failure = 1;
		printf(("could not read file global info in " + fileName).c_str());
		unzClose(zipfile);
		return;
	}
}

//closes the zipfile if it hasn't been destroyed
//destructors are called when the object is deallocated
ZipFile::~ZipFile()
{
	if (zipfile != NULL){
		unzClose(zipfile);
	}
}

//grabs a specific file indicated by fileName from the current zipfile and
//returns its data as a byte array (unsigned char *) and sets fileSize to the size of the array

unsigned char *ZipFile::readFile(string fileName, size_t &fileSize)
{
	const int READ_SIZE = 8196;
	const int MAX_FILENAME = 256;
	const char dir_delimiter = '/';

	unzGoToFirstFile(zipfile);

	// Loop to extract all files
	uLong i;
	for (i = 0; i < global_info.number_entry; ++i)
	{
		// Get info about current file.
		unz_file_info file_info;
		char filename[MAX_FILENAME];
		if (unzGetCurrentFileInfo(
			zipfile,
			&file_info,
			filename,
			MAX_FILENAME,
			NULL, 0, NULL, 0) != UNZ_OK)
		{
			printf("could not read file info\n");
			return NULL;
		}

		// Check if this entry is a directory or file.
		const size_t filename_length = strlen(filename);
		if (filename[filename_length - 1] != dir_delimiter && fileName.size() >= filename_length && fileName == &(filename[filename_length - fileName.size()])) //check that its not a dir and check that it is this file.
		{
			unsigned char *buffer = new unsigned char[file_info.uncompressed_size];
			// Entry is a file, so extract it.
			printf("file:%s\n", filename);
			//	fflush(stdout);
			if (unzOpenCurrentFile(zipfile) != UNZ_OK)
			{
				delete[] buffer;
				printf("could not open file\n");
				return NULL;
			}

			int error = UNZ_OK;

			error = unzReadCurrentFile(zipfile, buffer, file_info.uncompressed_size);
			if (error < 0)
			{
				printf("error %d\n", error);
				unzCloseCurrentFile(zipfile);
				delete[] buffer;
				return NULL;
			}
			fileSize = file_info.uncompressed_size;
			return buffer;
		}

		unzCloseCurrentFile(zipfile);

		// Go the the next entry listed in the zip file.
		if ((i + 1) < global_info.number_entry)
		{
			if (unzGoToNextFile(zipfile) != UNZ_OK)
			{
				printf("cound not read next file\n");
				return NULL;
			}
		}
	}
	return NULL;
}

//checks if the zipfile failed to open
bool ZipFile::fail()
{
	return failure;
}
