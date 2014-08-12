// SoAUpdater.cpp : Defines the entry point for the console application.
//
#include "stdafx.h";
#include "UpdaterMethods.h";
#include <iostream>
#include <fstream>
#include <sstream>
#include "ZipFile.h"
#include "curl/curl.h"
#include <stdio.h>
#include <direct.h>

using namespace std;

//prototypes

static int xferinfo(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

void SetStdinEcho(bool enable);


//I like to have main at the top. call me crazy
int main(int argc, char* argv[])
{
	setDlDir();
	curl_global_init(CURL_GLOBAL_ALL);

	string dldir = getDlDir();

	ifstream ifile(dldir + "version.txt");
	std::string line;
	int version = 0;
	int newupdaterversion = 0;
	int i = 0;
	if (ifile) {
		while (std::getline(ifile, line)) {
			std::cout << "Current version line " << i << ": " << line << endl;
			if (i == 0){
				version = atoi(line.c_str());
			}
			else if (i == 2){
				newupdaterversion = atoi(line.c_str());
				if (newupdaterversion > VERSION){
					ifstream newupdater(dldir + "SoAUpdater.exe");
					if (newupdater){
						newupdater.close();
						cout << "Newer version the updater (" << newupdaterversion << " vs " << VERSION << ") found in " << dldir << endl;
					}
					else{
						newupdaterversion = 0;
					}
				}
				break;
			}
			i++;
		}
		ifile.close();
	}
	stringstream strstrm;
	int userid;
	string username, password, email, title;
	bool loginok = false;
	strstrm << curlLoadStringFromUrl("http://www.seedofandromeda.com/updater/auth.php?action=checklogin"); //TODO: Change to libcurl with ssl so https can be used
	i = 0;
	while (getline(strstrm, line)){
		switch (i){
		case 0:
			if (line == "OK"){
				loginok = true;
			}
			break;
		case 1:
			if (!loginok){
				cout << "Login error: " << line << endl;
			}
			else{
				userid = atoi(line.c_str());
			}
			break;

		case 2:
			username = line;
			break;
		case 3:
			email = line;
			break;
		case 4:
			title = line;
			break;
		}
		i++;
	}
	strstrm.clear();
	strstrm.str(std::string());
	if (!loginok){
		cout << "Please login with your SoA forum account or press enter to skip login" << endl;
		cout << "Username: ";
		getline(cin, username);
		if (username.length() > 1){
			cout << "Password: ";
			SetStdinEcho(false);
			getline(cin, password);
			SetStdinEcho(true);
			cout << endl;
			cout << "Trying to log in..." << endl;
			string post = string("username=") + username + "&password=" + password; //TODO: Escape username and password
			strstrm << curlLoadStringFromUrl("http://www.seedofandromeda.com/updater/auth.php?action=login", post); //TODO: Change to libcurl with ssl so https can be used
			i = 0;
			while (getline(strstrm, line)){
				switch (i){
				case 0:
					if (line == "OK"){
						loginok = true;
					}
					break;
				case 1:
					if (!loginok){
						cout << "Login error: " << line << endl;
					}
					else{
						userid = atoi(line.c_str());
					}
					break;

				case 2:
					username = line;
					break;
				case 3:
					email = line;
					break;
				case 4:
					title = line;
					break;
				}
				i++;
			}
			strstrm.clear();
			strstrm.str(std::string());
		}

	}
	if (loginok){
		cout << "Hello " << username << ", " << title << ", " << email << "! You have been logged in. Your user id seems to be " << userid << endl;
	}
	std::cout << "Checking for updates... Current version is " << version << endl;
	std::stringstream urlstrm;
	urlstrm << "http://www.seedofandromeda.com/updater/update.php?version=" << version;
	string url = urlstrm.str();
	cout << "Downloading from " << url << endl;
	int curlout = curlLoadFileFromUrl(url, dldir + "latest.txt", xferinfo);

	cout << "cURL returns " << curlout << endl;

	bool update = false;
	string updateurl = "";

	std::ifstream input(dldir + "latest.txt");
	if (!input){
		cout << "latest.txt did not get downloaded!" << endl;
		if (version < 1){
			cout << "Launcher cannot continue, no downloaded version available and update check failed." << endl;
			system("pause");
			return -1;
		}
	}
	else{

		i = 0;
		bool updatelauncher = false;
		while (std::getline(input, line)) {
			std::cout << "Latest version line " << i << ": " << line << std::endl;
			if (i == 0){
				if (atoi(line.c_str()) > version) {
					cout << "Update found, downloading..." << endl;
					update = true;
				}
				else{
					cout << "No update found!" << endl;
				}
			}
			else if (i == 1 && update){
				updateurl = line;
			}
			else if (i == 2){
				int latestupdaterver = atoi(line.c_str());
				if (latestupdaterver > VERSION){
					if (latestupdaterver == newupdaterversion){
						cout << "Launching updater from " << dldir << endl;
						_chdir((dldir).c_str());
						system((dldir + "SoAUpdater.exe -noappdata").c_str());
						return 0;
					}
					else
						if (latestupdaterver > newupdaterversion){
						updatelauncher = true;
						}
						else{
							cout << endl << " <<< New version of SoAUpdater is available to download. Please update as soon as possible. >>>" << endl << endl;
							system("pause");
						}
				}
			}
			else if (i == 3 && updatelauncher){
				cout << "Updating launcher at " << dldir << endl;
				input.close();
				curlout = curlLoadFileFromUrl(line, dldir + "SoAUpdater.exe", xferinfo);
				cout << "cURL returns " << curlout << endl;
				if (!update){
					//If no SoA update is available, rename the latest.txt to version.txt to prevent downloading the launcher on every launch
					if (remove((dldir + "version.txt").c_str()) != 0){
						cout << "Failed to remove version.txt!" << endl;
					}
					if (rename((dldir + "latest.txt").c_str(), (dldir + "version.txt").c_str()) != 0){
						cout << "Failed to rename latest.txt to version.txt!" << endl;
					}
				}
				cout << "Launching updater from " << dldir << endl;
				_chdir((dldir).c_str());
				system((dldir + "SoAUpdater.exe -noappdata").c_str());
				return 0;
			}
			i++;
		}
		input.close();

	}
	if (update){
		curlout = curlLoadFileFromUrl(updateurl, dldir + "latest.zip", xferinfo);
		cout << "cURL returns " << curlout << endl;
		cout << "Unpacking the update..." << endl;
		//open zip file
		ZipFile zipFile(dldir + "latest.zip");
		if (zipFile.fail()){
			printf("%s: not found\n", dldir + "latest.zip");
			system("pause");
			return -1;
		}

		//extract zip file
		if (zipFile.extractZip(dldir) != 0){
			printf("failed to extract zip file.");
			system("pause");
			return -2;
		}
		else{
			//TODO: Close the zip file so it can be deleted!
			//zipFile.~ZipFile();
			if (remove((dldir + "latest.zip").c_str()) != 0){
				cout << "Failed to remove latest.zip!" << endl;
			}
			if (remove((dldir + "version.txt").c_str()) != 0){
				cout << "Failed to remove version.txt!" << endl;
			}
			if (rename((dldir + "latest.txt").c_str(), (dldir + "version.txt").c_str()) != 0){
				cout << "Failed to rename latest.txt to version.txt!" << endl;
			}
		}
	}
	cout << "Starting up SoA..." << endl;
	_chdir((dldir + "Release\\").c_str());
	system("SoA.exe");
	return 0;
}





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
			fprintf(stdout, "\r%.0lf%% of %.1f MB", dlRatio, dltotal / 1024.0f / 1024.0f);
		}
	}
	//if (dlnow > STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES)
	//	return 1;
	return 0;
}


void SetStdinEcho(bool enable = true)
{
#ifdef WIN32
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode;
	GetConsoleMode(hStdin, &mode);

	if (!enable)
		mode &= ~ENABLE_ECHO_INPUT;
	else
		mode |= ENABLE_ECHO_INPUT;

	SetConsoleMode(hStdin, mode);

#else
	struct termios tty;
	tcgetattr(STDIN_FILENO, &tty);
	if (!enable)
		tty.c_lflag &= ~ECHO;
	else
		tty.c_lflag |= ECHO;

	(void)tcsetattr(STDIN_FILENO, TCSANOW, &tty);
#endif
}