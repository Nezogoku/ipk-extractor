#include <iostream>
#include <algorithm>
#include <fstream>
#include <vector>
#include <string>
#include <direct.h>


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


int fromIPK(ifstream &file, int secdex) {
    uint32_t something0;                                                        // 32 bit le, Something
    uint32_t contentSize;                                                       // 32 bit le, Number of files in IPK file
    uint32_t fileSize;                                                          // 32 bit le, Size of current IPK file

    file.seekg(secdex + 0x04);
    file.read((char*)(&something0), sizeof(uint32_t));
    file.seekg(secdex + 0x08);
    file.read((char*)(&contentSize), sizeof(uint32_t));
    file.seekg(secdex + 0x0C);
    file.read((char*)(&fileSize), sizeof(uint32_t));


    uint32_t workAddr = secdex + 0x10;
    int fileFound = 0,
        ipkFileFound = 0;

    char buff;
    while (file.get(buff) && fileFound < contentSize) {
        // Get name and path of file being extracted
        file.seekg(workAddr);
        string temp_name;
        while(true) {
            file.get(buff);
            if (buff == 0x00) break;
            temp_name += buff;
        }
        workAddr += 0x44;

        // Get size of file being extracted (32 bit le)
        file.seekg(workAddr);
        uint32_t temp_size;
        file.read((char*)(&temp_size), sizeof(uint32_t));
        workAddr += 0x04;

        // Get location of file being extracted (32 bit le)
        file.seekg(workAddr);
        uint32_t temp_data_addr;
        file.read((char*)(&temp_data_addr), sizeof(uint32_t));
        workAddr += 0x08;


        // Get data of file being extracted
        file.seekg(secdex + temp_data_addr);


        // Create necessary folders
        int num_change = 0;
        for (int ind = 0; ind < temp_name.size(); ++ind) {
            if (temp_name[ind] == '\\' || temp_name[ind] == '/') {
                num_change += 1;
                _mkdir(temp_name.substr(0, ind).c_str());
            }
        }

        // Extract file
        ofstream extr_out(temp_name.c_str(), ios::binary);
        file.seekg(secdex + temp_data_addr);
        for(int ind = 0; ind < temp_size; ++ind) {
            file.get(buff);
            extr_out.put(buff);
        }
        extr_out.close();

        cout << "Extracted " << temp_name << endl;


        // Extract IPK from current IPK
        if (temp_name.substr(temp_name.size() - 4) == ".ipk") {
            _chdir((temp_name.substr(0, temp_name.find_last_of("\\/") + 1)).c_str());

            cout << "\tContents of " << temp_name << endl;
            ipkFileFound += fromIPK(file, secdex + temp_data_addr);

            cout << "\tEnd of " << temp_name << endl;

            for (int c = 0; c < num_change; ++c) _chdir("..");
        }

        fileFound += 1;
    }

    ipkFileFound += contentSize;

    return ipkFileFound;
}

void searchIPK(string ipk_filename) {
    ifstream file;
    char *chunk = new char[0x04];

    // IPK/IPH header
    string IPK1 = "IPK1";

    ipk_filename.erase(remove(ipk_filename.begin(), ipk_filename.end(), '\"'), ipk_filename.end());

    _chdir((ipk_filename.substr(0, ipk_filename.find_last_of("\\/") + 1)).c_str());

    ipk_filename = ipk_filename.substr(ipk_filename.find_last_of("\\/") + 1);

    file.open(ipk_filename.c_str(), ios::binary);
    if (!file.is_open()) {
        cerr << "Unable to open \"" << ipk_filename << "\"\n" << endl;
        return;
    }

    cout << "Searching through \"" << ipk_filename << "\"\n" << endl;

    uint32_t index = 0x00;
    int num_content = 0;

    file.seekg(index);
    file.read(chunk, sizeof(uint32_t));

    if (string(chunk) == IPK1) {
        num_content = fromIPK(file, index);
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
        cout << "Usage: " << prgm << " <infile(s)>\n"
             << endl;
    }
    else {
        for (int fileIndex = 1; fileIndex < argc; ++fileIndex) {
            searchIPK(argv[fileIndex]);
        }
    }

    _sleep(600);

    return 0;
}
