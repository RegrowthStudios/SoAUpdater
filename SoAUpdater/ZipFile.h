//****** ZipFile.h ******
// I omitted some includes, you will probably need to include others to compile,
// but the zlib specific includes are included here
#include <string>
#include <ZLIB/unzip.h>
#include <ZLIB/ioapi.h>

using namespace std;

class ZipFile{
public:
	ZipFile(string fileName);
	~ZipFile();
	unsigned char *readFile(string fileName, size_t &fileSize);
	bool fail();
private:
	unzFile zipfile;
	unz_global_info global_info;
	bool failure;
};
