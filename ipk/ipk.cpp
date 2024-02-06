#include <cstdio>
#include <string>
#include "directory.hpp"
#include "stringstream.hpp"
#include "ipk.hpp"


void ipk::searchFile(std::string filename, int filetyp) {
    std::string root = filename.substr(0, filename.find_last_of("\\/") + 1),
                name = filename.substr(filename.find_last_of("\\/") + 1);

    unsigned char *fdata = 0;
    unsigned fsize = 0;

    this->outLog = "";
    if (!getFileData(filename.c_str(), fdata, fsize)) {
        fprintf(stderr, "Unable to open %s\n", name.c_str());
        return;
    }
    
    if (filetyp == 0) searchIPK(root, name, fdata, fsize);
    else if (filetyp == 1) {
        this->inLog = {fdata, fsize};
        searchLOG(root, name);
    }
    else fprintf(stderr, "%s is not a supported file type\n", name.c_str());
    
    if (fdata) delete[] fdata;
}
