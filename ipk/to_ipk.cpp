#include <cstdint>
#include <cstdio>
#include <string>
#include "directory.hpp"
#include "bit_byte.hpp"
#include "lzss/lzss.hpp"
#include "ipk.hpp"

//Struct for custom stringstream
struct sstream {
    uchar *stream;
    unsigned strsize = 0, spos = 0, ssize = 0;

    std::string ssub; unsigned isub;
};

//Code for custom stringstream
int getStream(sstream &str) {
    int ret = 0;
    bool quotes = 0;

    str.ssub = "";
    str.isub = -1;

    if (!str.stream) return ret;

    str.spos += str.ssize;
    while (true) {
        if (str.spos >= str.strsize) break;

        if (str.stream[str.spos] == '\"') { quotes = !quotes; str.spos += 1; }
        else if (isgraph(str.stream[str.spos])) break;
        else str.spos += 1;
    }

    str.ssize = 0;
    while (true) {
        if (str.spos + str.ssize >= str.strsize) break;

        if (str.stream[str.spos + str.ssize] == '\"') { quotes = !quotes; str.ssize += 1; }
        else if (isspace(str.stream[str.spos + str.ssize]) && !quotes) break;
        else str.ssize += 1;
    }

    for (int c = 0; c < str.ssize; ++c) {
        if (str.stream[str.spos + c] == '\"') continue;
        else str.ssub += str.stream[str.spos + c];
    }

    if (!str.ssub.empty()) {
        bool isnum = true;
        for (auto &c : str.ssub) if (!isdigit(c)) { isnum = false; break; }
        if (isnum) { str.isub = stol(str.ssub, 0, 10); str.ssub.clear(); }

        ret = 1;
    }
    else { delete[] str.stream; str.stream = 0; }

    return ret;
}


int toIPK(sstream &str, std::string iroot, unsigned ialign, unsigned ientries, unsigned &isize, unsigned level, uchar *out) {
    const unsigned IPK1 = 0x49504B31;
    unsigned hed_size = 0x10,
             ent_size = hed_size + (ientries * 0x50),
             dat_offs = ent_size;

    if (ialign) {
        unsigned di = 0;
        while ((di += ialign) < dat_offs) { }

        dat_offs = di;
    }

    setBeInt(&out[0x00], IPK1, 4);
    setLeInt(&out[0x04], ialign, 4);
    setLeInt(&out[0x08], ientries, 4);
    setLeInt(&out[0x0C], 0, 4);

    for (int e = 0; e < ientries; ++e) {
        std::string path, name;
        unsigned hed_offs, comp, csiz, offs, rsiz, alin, amnt;
        bool is_ipk = false;
        uchar *edata = 0;

        if (!getStream(str) || str.isub >= ientries) return 0;
        else hed_offs = hed_size + (str.isub * 0x50);

        //Get entry info
        if (!getStream(str) || str.ssub != "ENTRY_PATH" || !getStream(str)) {
            fprintf(stderr, "Invalid IPK entry: path name missing\n");
            return 0;
        }
        else path = str.ssub;

        if (path.size() > 64) {
            fprintf(stderr, "Invalid IPK entry: path name length exceeds 64\n");
            return 0;
        }
        else is_ipk = (path.find(".ipk") != std::string::npos);

        //Initial entry info
        path.copy((char*)&out[hed_offs], 64);
        setLeInt(&out[hed_offs + 0x40], 0, 4);
        setLeInt(&out[hed_offs + 0x44], 0, 4);
        setLeInt(&out[hed_offs + 0x48], 0, 4);
        setLeInt(&out[hed_offs + 0x4C], 0, 4);

        name = path.substr(path.find_last_of("\\/") + 1);

        //Get data from specified path(s)
        if (is_ipk) {
            if (!getStream(str) || str.ssub != "START_EMBEDDED_IPK") {
                fprintf(stderr, "Invalid embedded IPK: missing initializer\n");
                return 0;
            }
            else if (!getStream(str) || str.ssub != name) {
                fprintf(stderr, "Invalid embedded IPK: invalid initializing name\n");
                return 0;
            }

            //Get embedded IPK info
            bool has_size = false, has_root = false, has_align = false, has_num = false;
            for (int t = 0; t < 4; ++t) {
                if (!getStream(str)) break;

                if (str.ssub == "EMBEDDED_IPK_SIZE" && getStream(str) && (int)str.isub > 0)     { rsiz = str.isub; has_size = true; }
                else if (str.ssub == "ROOT_FOLDER" && getStream(str))                           { path = str.ssub; has_root = true; }
                else if (str.ssub == "ALIGNMENT" && getStream(str) && (int)str.isub >= 0)       { alin = str.isub; has_align = true; }
                else if (str.ssub == "AMOUNT_ENTRIES" && getStream(str) && (int)str.isub > 0)   { amnt = str.isub; has_num = true; }
            }

            if (!has_size || !has_root || !has_align || !has_num) {
                fprintf(stderr, "Invalid embedded IPK: missing crucial header data\n");
                return 0;
            }

            rsiz = rsiz * 1.5;
            edata = new uchar[rsiz] {};

            if (!toIPK(str, path, alin, amnt, rsiz, level + 1, edata)) {
                fprintf(stderr, "Unable to pack embedded data specified in %s\n", name.c_str());
                return 0;
            }

            if (!getStream(str) || str.ssub != "END_EMBEDDED_IPK") {
                fprintf(stderr, "Invalid embedded IPK: missing terminator\n");
                return 0;
            }
            else if (!getStream(str) || str.ssub != name) {
                fprintf(stderr, "Invalid embedded IPK: invalid terminating name\n");
                return 0;
            }
        }
        else {
            rsiz = 0;
            if (!getFileData((iroot + path).c_str(), edata, rsiz)) {
                fprintf(stderr, "Unable to open %s\n", name.c_str());
                return 0;
            }
        }

        csiz = rsiz;
        if (!is_ipk) compressLZSS(edata, rsiz, &out[dat_offs], csiz);

        if (csiz >= rsiz) {
            for (int c = 0; c < csiz; ++c) {
                if (c < rsiz) out[dat_offs + c] = edata[c];
                else out[dat_offs + c] = 0;
            }
            csiz = rsiz;
        }

        //Update entry info
        setLeInt(&out[hed_offs + 0x40], (rsiz != csiz), 4);
        setLeInt(&out[hed_offs + 0x44], csiz, 4);
        setLeInt(&out[hed_offs + 0x48], dat_offs, 4);
        setLeInt(&out[hed_offs + 0x4C], rsiz, 4);

        if (ialign) {
            unsigned di = dat_offs;
            while ((di += ialign) < (dat_offs + csiz)) { }

            dat_offs = di;
        }
        else dat_offs += csiz;

        //Better distinguish between what's embedded and what isn't when printing
        for (int L = 0; L < level; ++L) fprintf(stdout, "    ");
        fprintf(stdout, "    PACKED %s TO ENTRY %d WITH COMPRESSION RATIO %%%.2f\n",
                        name.c_str(), e, ((double)csiz / rsiz) * 100);
    }

    //Update file size
    isize = dat_offs;
    setLeInt(&out[0x0C], isize, 4);

    return 1;
}

void searchLOG(std::string logname) {
    std::string name = logname.substr(logname.find_last_of("\\/") + 1);

    sstream sslog;
    if (!getFileData(logname.c_str(), sslog.stream, sslog.strsize)) {
        fprintf(stderr, "Unable to open %s\n", name.c_str());
        return;
    }
    else if (!getStream(sslog) || sslog.ssub != "START_ROOT_IPK") {
        fprintf(stderr, "%s is not a true IPKLOG file: missing initializer\n", name.c_str());
        return;
    }
    else if (!getStream(sslog) || logname.find(sslog.ssub) == std::string::npos) {
        fprintf(stderr, "Invalid IPK: invalid initializing name\n");
        return;
    }

    std::string nip_ = sslog.ssub.substr(0, sslog.ssub.find_last_of('.')), iroot;
    bool has_size = false, has_root = false, has_align = false, has_num = false;
    unsigned isize, ialign, ientries;

    for (int t = 0; t < 4; ++t) {
        if (!getStream(sslog)) break;

        if (sslog.ssub == "ROOT_IPK_SIZE" && getStream(sslog) && (int)sslog.isub > 0)       { isize = sslog.isub; has_size = true; }
        else if (sslog.ssub == "ROOT_FOLDER" && getStream(sslog))                           { iroot = sslog.ssub; has_root = true; }
        else if (sslog.ssub == "ALIGNMENT" && getStream(sslog) && (int)sslog.isub >= 0)     { ialign = sslog.isub; has_align = true; }
        else if (sslog.ssub == "AMOUNT_ENTRIES" && getStream(sslog) && (int)sslog.isub > 0) { ientries = sslog.isub; has_num = true; }
        else break;
    }

    if (has_size && has_root && has_align && has_num) {
        isize = isize * 1.5;
        uchar *idata = new uchar[isize] {};

        //Get files' data
        if (!toIPK(sslog, iroot, ialign, ientries, isize, 0, idata)) {
            fprintf(stderr, "Unable to pack data specified in %s\n", name.c_str());
            return;
        }

        //Ensure log is true IPKLOG file
        if (!getStream(sslog) || sslog.ssub != "END_ROOT_IPK") {
            fprintf(stderr, "%s is not a true IPKLOG file: missing terminator\n", name.c_str());
            return;
        }
        else if (!getStream(sslog) || logname.find(sslog.ssub) == std::string::npos) {
            fprintf(stderr, "Invalid IPK: invalid terminating name\n");
            return;
        }

        iroot = logname.substr(0, logname.find_last_of("\\/") + 1);

        //Save IPK and IPH to files
        if (!createFile((iroot + nip_ + ".ipk.new").c_str(), idata, isize)) {
            fprintf(stderr, "Unable to save %s.ipk.new\n", name.c_str());
            return;
        }
        else if (!createFile((iroot + nip_ + ".iph.new").c_str(), idata, (0x10 + (ientries * 0x50)))) {
            fprintf(stderr, "Unable to save %s.iph.new\n", name.c_str());
            return;
        }

        fprintf(stdout, "IPH and IPK files successfully packed\n");
    }
    else fprintf(stderr, "Invalid IPK: missing crucial header data\n");
}
