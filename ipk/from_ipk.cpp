#include <cstdint>
#include <cstdio>
#include <string>
#include "directory.hpp"
#include "bit_byte.hpp"
#include "lzss/lzss.hpp"
#include "ipk.hpp"


int fromIPK(std::string &outlog, unsigned char *idata, unsigned isize, const char *iroot) {
    uchar *istart = idata, *hstart = istart + 0x10, *obuffer = 0, *odata;

    if (!cmpChar(idata, "IPK1", 4)) { fprintf(stderr, "Not an IPK file\n"); return 0; }
    else idata += 4;
    uint32_t alignment = getLeInt(idata, 4);        // File alignment
    uint32_t entryNum = getLeInt(idata, 4);         // Number of files in IPK file
    if (isize < getLeInt(idata, 4)) return 0;       // Size of current IPK file

    int ifNum = 0;

    outlog += getFData("ROOT_FOLDER           \"%s\"\n", iroot);
    outlog += getFData("ALIGNMENT             %d\n", alignment);
    outlog += getFData("AMOUNT_ENTRIES        %d\n", entryNum);

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

        outlog += getFData("%-4d ENTRY_PATH            \"%s\"\n", e, path.c_str());

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
        }
        else odata = istart + foffs;

        path = iroot + path;

        //Extract embedded IPK or data
        if (cmpChar(odata, "IPK1", 4)) {
            outlog += getFData("START_EMBEDDED_IPK    \"%s\"\n", fname.c_str());
            outlog += getFData("EMBEDDED_IPK_SIZE     %d\n", rsize);

            fprintf(stdout, "START OF EMBEDDED %s\n", fname.c_str());
            ifNum += fromIPK(outlog, odata, rsize, iroot);
            fprintf(stdout, "END OF EMBEDDED %s\n", fname.c_str());

            outlog += getFData("END_EMBEDDED_IPK      \"%s\"\n", fname.c_str());
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
    std::string root = ipkname.substr(0, ipkname.find_last_of("\\/") + 1),
                name = ipkname.substr(ipkname.find_last_of("\\/") + 1),
                froot = getFData("%s@%s/", root.c_str(), name.substr(0, name.find_last_of('.')).c_str()),
                lname = name.substr(0, name.find_last_of('.')) + ".ipklog",
                ipk_outlog = "";

    uchar *ipk_data = 0;
    uint32_t ipk_size;

    if (!createFolder(froot.c_str())) {
        fprintf(stderr, "Unable to create root folder\n");
        return;
    }
    else if (!getFileData(ipkname.c_str(), ipk_data, ipk_size)) {
        fprintf(stderr, "Unable to open %s\n", name.c_str());
        return;
    }

    //Log setup
    ipk_outlog += getFData("START_ROOT_IPK        \"%s\"\n", name.c_str());
    ipk_outlog += getFData("ROOT_IPK_SIZE         %d\n", ipk_size);

    fprintf(stdout, "START OF %s\n", name.c_str());
    int fcount = fromIPK(ipk_outlog, ipk_data, ipk_size, froot.c_str());
    fprintf(stdout, "END OF %s\n\n", name.c_str());

    ipk_outlog += getFData("END_ROOT_IPK          \"%s\"\n", name.c_str());

    if (!createFile((root + lname).c_str(), ipk_outlog.c_str(), ipk_outlog.length())) {
        fprintf(stderr, "Unable to save %s\n", lname.c_str());
        return;
    }

    fprintf(stdout, "%d total files found and extracted\n\n", fcount);
}
