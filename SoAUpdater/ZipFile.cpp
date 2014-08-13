//****** ZipFile.cpp ******

#include "stdafx.h"
#include <Windows.h>
#include <ZLIB/unzip.c>
#include <ZLIB/ioapi.c>
#include "ZipFile.h"
#include <iostream>
#include <stdio.h>
using namespace std;

#define READ_SIZE 8192

void MakeDirectory(const char *fileName) {
    DWORD dwAttrib = GetFileAttributes(fileName);
    if (!(dwAttrib != INVALID_FILE_ATTRIBUTES &&
        (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))) {

        if (!CreateDirectory(fileName, NULL)) {
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
            _tprintf("%s", lpMsgBuf);
        }
    }
}

//constructor for the zipfile. You must initialize it with the zipfile name
ZipFile::ZipFile(string fileName) : zipfile(NULL), failure(0) // <-initialization list. Its a fast way to initialize variables
{
    //open the zipfile
    zipfile = unzOpen(fileName.c_str());

    //unzOpen returns NULL when it fails, so it will set failure to 1.
    if (zipfile == NULL) {
        failure = 1;
        //Error is a function I created. You can make your own error function that just does a printf or dumps to a file
        printf((fileName + " not found\n").c_str());
        return;
    }

    //grabs the zipfile info and error checks it
    if (unzGetGlobalInfo(zipfile, &global_info) != UNZ_OK) {
        failure = 1;
        printf(("could not read file global info in " + fileName).c_str());
        unzClose(zipfile);
        return;
    }
}

//closes the zipfile if it hasn't been destroyed
//destructors are called when the object is deallocated
ZipFile::~ZipFile() {
    if (zipfile != NULL) {
        unzClose(zipfile);
    }
}

//grabs a specific file indicated by fileName from the current zipfile and
//returns its data as a byte array (unsigned char *) and sets fileSize to the size of the array

unsigned char *ZipFile::readFile(string fileName, size_t &fileSize) {
    const int MAX_FILENAME = 256;
    const char dir_delimiter = '/';

    unzGoToFirstFile(zipfile);

    // Loop to extract all files
    uLong i;
    for (i = 0; i < global_info.number_entry; ++i) {
        // Get info about current file.
        unz_file_info file_info;
        char filename[MAX_FILENAME];
        if (unzGetCurrentFileInfo(
            zipfile,
            &file_info,
            filename,
            MAX_FILENAME,
            NULL, 0, NULL, 0) != UNZ_OK) {
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
            if (unzOpenCurrentFile(zipfile) != UNZ_OK) {
                delete[] buffer;
                printf("could not open file\n");
                return NULL;
            }

            int error = UNZ_OK;

            error = unzReadCurrentFile(zipfile, buffer, file_info.uncompressed_size);
            if (error < 0) {
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
        if ((i + 1) < global_info.number_entry) {
            if (unzGoToNextFile(zipfile) != UNZ_OK) {
                printf("cound not read next file\n");
                return NULL;
            }
        }
    }
    return NULL;
}

int ZipFile::extractZip(string outputDir) {
    // Buffer to hold data read from the zip file.
    char read_buffer[READ_SIZE];

    //check if output dir exists and make it if not
    MakeDirectory(outputDir.c_str());
    printf("\n");
    // Loop to extract all files
    uLong i;
    double percentage = 0;
    for (i = 0; i < global_info.number_entry; ++i) {
        percentage = (double)i / (double)global_info.number_entry * 100.0;

        // Get info about current file.
        unz_file_info file_info;
        char filename[FILENAME_MAX];
        char outFileName[FILENAME_MAX];
        if (unzGetCurrentFileInfo(
            zipfile,
            &file_info,
            filename,
            FILENAME_MAX,
            NULL, 0, NULL, 0) != UNZ_OK) {
            printf("could not read file info\n");
            unzClose(zipfile);
            return -1;
        }
        //add the target dir onto the filename
        strcpy(outFileName, (outputDir + filename).c_str());

        // Check if this entry is a directory or file.
        const size_t filename_length = strlen(filename);
        if (filename[filename_length - 1] == '/') {
            // Entry is a directory, so create it.
            printf("\r%.0lf%% - dir: %65.65s", percentage, filename);

            //check if it already exists and make it if not
            MakeDirectory(outFileName);
        } else {
            // Entry is a file, so extract it.
            printf("\r%.0lf%% - file: %65.65s", percentage, filename);

            if (unzOpenCurrentFile(zipfile) != UNZ_OK) {
                printf("\ncould not open file\n");
                unzClose(zipfile);
                zipfile = NULL;
                return -1;
            }

            // Open a file to write out the data.
            FILE *out = fopen(outFileName, "wb");
            if (out == NULL) {
                printf("\ncould not open destination file\n");
                unzCloseCurrentFile(zipfile);
                unzClose(zipfile);
                zipfile = NULL;
                return -1;
            }

            int error = UNZ_OK;
            do {
                error = unzReadCurrentFile(zipfile, read_buffer, READ_SIZE);
                if (error < 0) {
                    printf("\nerror %d\n", error);
                    unzCloseCurrentFile(zipfile);
                    unzClose(zipfile);
                    zipfile = NULL;
                    return -1;
                }

                // Write data to file.
                if (error > 0) {
                    fwrite(read_buffer, error, 1, out); // You should check return of fwrite...
                }
            } while (error > 0);

            fclose(out);
        }

        unzCloseCurrentFile(zipfile);

        // Go the the next entry listed in the zip file.
        if ((i + 1) < global_info.number_entry) {
            if (unzGoToNextFile(zipfile) != UNZ_OK) {
                printf("\ncound not read next file\n");
                unzClose(zipfile);
                zipfile = NULL;
                return -1;
            }
        }
    }

    printf("\n");
    return 0;
}

//checks if the zipfile failed to open
bool ZipFile::fail() {
    return failure;
}
