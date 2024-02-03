#include <cstdio>
#include <string>
#include <variant>
#include "directory.hpp"
#include "lzss_shared.hpp"
#include "ipk.hpp"


template<typename... Args>
std::string ipk::formatStr(const char *in, Args... args) {
    std::string stmp = "";
    std::variant<std::string, unsigned int, int> ftmp[] = {args...};

    while (in[0]) {
        if (in[0] == '{') {
            int p = in[1] - '0';
            switch(ftmp[p].index()) {
                case 0:
                    stmp += std::get<0>(ftmp[p]);
                    break;
                case 1:
                    if (in[2] == 'X') {
                        const char HEX[] = "0123456789ABCDEF";
                        std::string tmp = "";
                        unsigned num = std::get<1>(ftmp[p]);
                        while (num) {
                            tmp = HEX[num % 16] + tmp;
                            num /= 16;
                        }
                        num = in[3] - '0';
                        if (!tmp.empty()) num = ((num - (tmp.length() % num)) % num);
                        stmp += "0x" + std::string(num, '0') + tmp;

                        in += 2;
                    }
                    else if (in[2] == 'D') {
                        std::string tmp = std::to_string(std::get<1>(ftmp[p]));
                        unsigned num = in[3] - '0';
                        num = ((num - (tmp.length() % num)) % num);
                        stmp += std::string(num, '0') + tmp;

                        in += 2;
                    }
                    else stmp += std::to_string(std::get<1>(ftmp[p]));
                    break;
                case 2:
                    stmp += std::to_string(std::get<2>(ftmp[p]));
                    break;
                default:
                    stmp += *(in++);
                    continue;
            }
            in += 3;
        }
        else stmp += *(in++);
    }
    return stmp;
}


int ipk::fromIPK(unsigned char *idata, unsigned isize, std::string iroot) {
    struct ipk1_info {
        //const char ipk1[4] {'I','P','K','1'};
        unsigned align;
        unsigned ennum;
        unsigned rfsiz;
    } iinfo;

    unsigned ent = 0;
    unsigned char *ibeg = idata, *iend = ibeg + isize;
    auto cmp_str = [&](const char *in1, int length) -> bool {
        while ((*(idata++) == (unsigned char)(*(in1++))) && --length);
        return !length;
    };
    auto get_int = [&]() -> unsigned {
        unsigned out = 0;
        for (int i = 0; i < 4; ++i)
            { out |= (unsigned)*(idata++) << (8 * i); }
        return out;
    };

    if (!cmp_str("IPK1", 4)) {
        fprintf(stderr, "Not an IPK file\n");
        return 0;
    }
    iinfo.align = get_int();    // File alignment
    iinfo.ennum = get_int();    // Number of files in IPK file
    iinfo.rfsiz = get_int();    // Size of current IPK file
    
    if (isize < iinfo.rfsiz) {
        fprintf(stderr, "File size smaller than expected\n");
        return 0;
    }

    this->outLog += formatStr("ROOT_FOLDER           \"{0}\"\n", iroot);
    this->outLog += formatStr("ALIGNMENT             {0}\n", iinfo.align);
    this->outLog += formatStr("AMOUNT_ENTRIES        {0}\n", iinfo.ennum);


    struct file_info {
        std::string fpath;  // Path and file name
        bool fcomp;         // Is compressed
        unsigned fsize;     // Data size (compressed)
        unsigned foffs;     // Data offset
        unsigned rsize;     // Data size (uncompressed)
    } finfo[iinfo.ennum] {};

    idata = ibeg + 0x10;
    for (auto &inf : finfo) {
        inf.fpath = (const char*)idata; idata += 64;
        inf.fcomp = get_int();
        inf.fsize = get_int();
        inf.foffs = get_int();
        inf.rsize = get_int();
    }

    iinfo.ennum = 0;
    for (auto &inf : finfo) {
        idata = ibeg + inf.foffs;
        if (idata >= iend || (idata + inf.fsize) > iend || !inf.rsize) continue;

        std::string fname = inf.fpath.substr(inf.fpath.find_last_of("\\/") + 1),
                    fpath = inf.fpath.substr(0, inf.fpath.find_last_of("\\/") + 1);
        unsigned char *odata = 0;
        unsigned osize = inf.rsize;

        //Replace seperator with more universal one
        while (inf.fpath.find("\\") != std::string::npos) {
            inf.fpath.replace(inf.fpath.find("\\"), 1, "/");
        }
        this->outLog += formatStr("{0} ENTRY_PATH            \"{1}\"\n", ent++, inf.fpath);

        //Create necessary folders
        for (int pos = 0;;) {
            pos = fpath.find_first_of("\\/", pos);

            if (pos == std::string::npos) break;
            else pos += 1;

            std::string npath = iroot + fpath.substr(0, pos);
            createFolder(npath.c_str());
        }

        //Decompress LZSS if applicable
        if (!inf.fcomp) odata = idata;
        else {
            odata = new unsigned char[inf.rsize] {};
            decompress(idata, inf.fsize, odata, osize);
        }
        fpath = iroot + inf.fpath;

        //Extract data
        if (std::string(odata, odata + 4) != "IPK1") {
            if (!createFile(fpath.c_str(), odata, osize));
            else {
                fprintf(stdout, "    EXTRACTED %s\n", fname.c_str());
                iinfo.ennum += 1;
            }
        }
        else {
            this->outLog += formatStr("START_EMBEDDED_IPK    \"{0}\"\n", fname);
            this->outLog += formatStr("EMBEDDED_IPK_SIZE     {0}\n", osize);

            fprintf(stdout, "START OF EMBEDDED %s\n", fname.c_str());
            iinfo.ennum += fromIPK(odata, osize, iroot);
            fprintf(stdout, "END OF EMBEDDED %s\n", fname.c_str());

            this->outLog += formatStr("END_EMBEDDED_IPK      \"{0}\"\n", fname);
        }

        if (inf.fcomp) delete[] odata;
    }

    return iinfo.ennum;
}

void ipk::searchIPK(std::string root, std::string name, unsigned char *fdat, unsigned fsiz) {
    std::string froot = formatStr("{0}@{1}/", root, name.substr(0, name.find_last_of('.'))),
                lname = name.substr(0, name.find_last_of('.')) + ".ipklog";

    if (!createFolder(froot.c_str())) {
        fprintf(stderr, "Unable to create root folder\n");
        return;
    }

    //Log setup
    this->outLog = "";
    this->outLog += formatStr("START_ROOT_IPK        \"{0}\"\n", name);
    this->outLog += formatStr("ROOT_IPK_SIZE         {0}\n", fsiz);

    fprintf(stdout, "START OF %s\n", name.c_str());
    int fcount = fromIPK(fdat, fsiz, froot);
    fprintf(stdout, "END OF %s\n\n", name.c_str());

    this->outLog += formatStr("END_ROOT_IPK          \"{0}\"\n", name);

    if (!createFile((root + lname).c_str(), this->outLog.c_str(), this->outLog.length())) {
        fprintf(stderr, "Unable to save %s\n", lname.c_str());
    }
    fprintf(stdout, "%d total files found and extracted\n\n", fcount);
}
