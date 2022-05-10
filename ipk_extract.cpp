#include <iostream>
#include <algorithm>
#include <fstream>
#include <vector>
#include <string>
#include <direct.h>
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    #include <winsock2.h>
#else
    #include <arpa/inet.h>
#endif


using std::cout;
using std::cerr;
using std::dec;
using std::endl;
using std::fstream;
using std::hex;
using std::ifstream;
using std::ios;
using std::istreambuf_iterator;
using std::ofstream;
using std::string;
using std::to_string;
using std::vector;
using std::remove;


char chr;
char *chunk = new char[0x04];
char *name = new char[0x44];

int extractIPK(ifstream &file, int secdex) {
    uint32_t something0;                                                        // 32 bit le, Something
    uint32_t contentSize;                                                       // 32 bit le, Number of files in IPK file
    uint32_t fileSize;                                                          // 32 bit le, Size of current IPK file

    file.seekg(secdex + 0x04);
    file.read(reinterpret_cast<char*>(&something0), sizeof(uint32_t));
    file.seekg(secdex + 0x08);
    file.read(reinterpret_cast<char*>(&contentSize), sizeof(uint32_t));
    file.seekg(secdex + 0x0C);
    file.read(reinterpret_cast<char*>(&fileSize), sizeof(uint32_t));


    uint32_t workAddr = secdex + 0x10;
    int fileFound = 0,
        ipkFileFound = 0;

    while (fileFound < contentSize) {
        // Get name and path of file being extracted
        file.seekg(workAddr);
        file.read(name, 0x44);
        string temp_name = name;
        workAddr += 0x44;

        // Get size of file being extracted (32 bit le)
        file.seekg(workAddr);
        uint32_t temp_size;
        file.read(reinterpret_cast<char*>(&temp_size), sizeof(uint32_t));
        workAddr += 0x04;

        // Get location of file being extracted (32 bit le)
        file.seekg(workAddr);
        uint32_t temp_data_addr;
        file.read(reinterpret_cast<char*>(&temp_data_addr), sizeof(uint32_t));
        workAddr += 0x08;


        // Get data of file being extracted
        file.seekg(secdex + temp_data_addr);


        // Create necessary folders
        for (int ind = 0; ind < temp_name.size(); ++ind) {
            if (temp_name[ind] == '\\' ||
                temp_name[ind] == '/') _mkdir(temp_name.substr(0, ind).c_str());
        }

        // Extract file
        ofstream extr_out(temp_name.c_str(), ios::binary);
        file.seekg(secdex + temp_data_addr);
        for(int ind = 0; ind < temp_size; ++ind) {
            char buff;
            file.get(buff);
            extr_out.put(buff);
        }
        extr_out.close();

        cout << "Extracted " << temp_name << endl;

        // Extract IPK from current IPK
        string ext = temp_name.substr(temp_name.find_last_of('.') + 1);
        if (ext == "ipk") {
            cout << "\tContents of " << temp_name << endl;
            ipkFileFound += extractIPK(file, secdex + temp_data_addr);
            cout << "\tEnd of " << temp_name << "\n" << endl;
        }

        fileFound += 1;
    }

    fileFound += ipkFileFound;

    return fileFound;
}

void searchIPK(string ipk_filename) {
    ifstream file;

    // IPK/IPH header
    string IPK1 = "IPK1";

    // Storage of IPK contents
    string extracted_folder = "";

    ipk_filename.erase(remove(ipk_filename.begin(), ipk_filename.end(), '\"'), ipk_filename.end());

    extracted_folder = ipk_filename;
    extracted_folder = extracted_folder.substr(0, extracted_folder.find_last_of("\\/") + 1);

    _chdir(extracted_folder.c_str());

    ipk_filename = ipk_filename.substr(ipk_filename.find_last_of("\\/") + 1);

    file.open(ipk_filename.c_str(), ios::binary);
    if (!file.is_open()) {
        cerr << "Unable to open \"" << ipk_filename << "\"\n" << endl;
        return;
    }

    extracted_folder += "IPK_CONTENTS/";

    _mkdir(extracted_folder.c_str());
    _chdir(extracted_folder.c_str());

    cout << "Searching through \"" << ipk_filename << "\"\n" << endl;

    uint32_t index = 0x0;
    int num_content = 0;

    file.seekg(index);
    file.read(chunk, sizeof(uint32_t));

    if (string(chunk) == IPK1) {
        num_content = extractIPK(file, index);
    }
    else {
        cout << ipk_filename << " is not an IPK file\n\n" << endl;
    }
    file.close();

    cout << num_content << " files found and extracted\n\n" << endl;
}

int main(int argc, char *argv[]) {
    string prgm = argv[0];
    prgm.erase(remove(prgm.begin(), prgm.end(), '\"'), prgm.end());
    prgm = prgm.substr(prgm.find_last_of("\\/") + 1, prgm.find_last_of('.'));


    if (argc < 2) {
        cout << "Usage: " << prgm << " <infile(s)>\n\n"
             << "Best to have corresponding IPH file in same folder\n"
             << endl;
    }
    else {
        for (int fileIndex = 1; fileIndex < argc; ++fileIndex) {
            searchIPK(argv[fileIndex]);
        }
    }

    Sleep(600);

    return 0;
}
