#include <cstdint>
#include <cstdio>
#include <string>
#include "directory.hpp"
#include "bit_byte.hpp"
#include "lzss/lzss.hpp"
#include "ipk.hpp"


int fromIPK(uchar *idata, unsigned int isize, const char *iroot) {
    uchar *istart = idata, *hstart = istart + 0x10, *obuffer = 0, *odata;

    if (!cmpChar(idata, "IPK1", 4)) { fprintf(stderr, "Not an IPK file\n"); return 0; }
    else idata += 4;
    uint32_t unknown0 = getLeInt(idata, 4);         // Something
    uint32_t entryNum = getLeInt(idata, 4);         // Number of files in IPK file
    if (isize < getLeInt(idata, 4)) return 0;       // Size of current IPK file

    int ifNum = 0;

    for (int e = 0; e < entryNum; ++e) {
        idata = hstart + (e * 0x50);

        //File definitions
        std::string path = {idata, idata + 64};     //Path and file name
        idata += 64;
        bool compressed = getLeInt(idata, 4);       //Is compressed
        uint32_t fsize = getLeInt(idata, 4);        //Data size (compressed)
        uint32_t foffs = getLeInt(idata, 4);        //Data offset
        uint32_t rsize = getLeInt(idata, 4);        //Data size (uncompressed)

        std::string fname = path.substr(path.find_last_of("\\/") + 1),
                    fpath = path.substr(0, path.find_last_of("\\/") + 1);

        //Create necessary folders
        for (int pos = 0;;) {
            pos = fpath.find_first_of("\\/", pos);

            if (pos == std::string::npos) break;
            else pos += 1;

            std::string npath = iroot + fpath.substr(0, pos);
            createFolder(npath.c_str());
        }

        //Decompress Blinx2 LZSS file if applicable
        if (compressed) {
            if (obuffer) { delete[] obuffer; obuffer = 0; }
            obuffer = new uchar[rsize] {};

            decompressLZSS(istart + foffs, fsize, obuffer, rsize);
            odata = obuffer;

            fprintf(stdout, "DECOMPRESSED %s\n", fname.c_str());
        }
        else odata = istart + foffs;

        path = iroot + path;

        //Extract embedded IPK or data
        if (cmpChar(odata, "IPK1", 4)) {
            fprintf(stdout, "START OF EMBEDDED %s\n", fname.c_str());
            ifNum += fromIPK(odata, rsize, iroot /*path.substr(0, path.find_last_of("\\/") + 1)*/);
            fprintf(stdout, "END OF EMBEDDED %s\n", fname.c_str());
        }
        else {
            if (!createFile(path.c_str(), odata, rsize)) continue;
            else ifNum += 1;

            fprintf(stdout, "    EXTRACTED %s\n", fname.c_str());
        }
    }

    return ifNum;
}

void searchIPK(std::string ipkname) {
    std::string root = ipkname.substr(0, ipkname.find_last_of("\\/")) + "/@ipk_out/",
                name = ipkname.substr(ipkname.find_last_of("\\/") + 1);

    uchar *ipk_data = 0;
    uint32_t ipk_size;

    if (!createFolder(root.c_str())) {
        fprintf(stderr, "Unable to create root folder\n");
        return;
    }
    else if (!getFileData(ipkname.c_str(), ipk_data, ipk_size)) {
        fprintf(stderr, "Unable to open %s\n", name.c_str());
        return;
    }

    fprintf(stdout, "START OF %s\n", name.c_str());
    int fcount = fromIPK(ipk_data, ipk_size, root.c_str());
    fprintf(stdout, "END OF %s\n\n", name.c_str());

    fprintf(stdout, "%d total files found and extracted\n\n", fcount);
}
