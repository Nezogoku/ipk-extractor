#include <cstdio>
#include <string>
#include "directory.hpp"
#include "stringstream.hpp"
#include "lzss_shared.hpp"
#include "ipk.hpp"

int ipk::toIPK(std::string iroot, unsigned ialign, unsigned ientries, unsigned level, unsigned char *out) {
    std::string tmpS;
    unsigned dat_offs = 0x10 + (ientries * 0x50), tmpN;
    auto set_str = [](unsigned char *out, const char *src1) -> void {
        for (int i = 0; i < 4; ++i) out[i] = src1[i];
    };
    auto set_int = [](unsigned char *out, const unsigned src1) -> void {
        for (int i = 0; i < 4; ++i) out[i] = (src1 >> (8 * i)) & 0xFF;
    };

    dat_offs += (!ialign) ? 0 : (ialign - (dat_offs % ialign)) % ialign;

    set_str(out + 0x00, "IPK1");
    set_int(out + 0x04, ialign);
    set_int(out + 0x08, ientries);
    set_int(out + 0x0C, dat_offs);
    
    //Universal compression bool thing specifically because
    //uniform_tex.ipk allows all compression
    bool uni_keep_comp = (level > 0 && ientries > 600);

    //Get and apply entry info
    for (int e = 0; e < ientries; ++e) {
        std::string fpath, fname, fext;
        unsigned hoffs = 0, fsize = 0, foffs = 0, rsize = 0;
        unsigned char *fdata = 0;

        //Get entry header info
        tmpN = this->inLog.getUnsigned();
        if (tmpN >= ientries) return 0;
        else hoffs = 0x10 + (tmpN * 0x50);

        //Get entry info
        tmpS = this->inLog.getString(0, "ENTRY_PATH");
        if (tmpS.empty()) {
            fprintf(stderr, "Invalid IPK entry: path name missing\n");
            return 0;
        }
        else fpath = tmpS;

        if (fpath.length() > 64) {
            fprintf(stderr, "Invalid IPK entry: path name length exceeds 64\n");
            return 0;
        }
        else {
            fname = fpath.substr(fpath.find_last_of("\\/") + 1);
            fext = fname.substr(fname.find_last_of('.') + 1);
        }


        if (fext != "ipk") {
            if (!getFileData((iroot + fpath).c_str(), fdata, rsize)) {
                fprintf(stderr, "Unable to open %s\n", fname.c_str());
                return 0;
            }
        }
        else {
            std::string troot;
            bool has_size, has_root, has_align, has_num, is_success;
            unsigned talign, tentries;

            //Get embedded IPK header
            tmpS = this->inLog.getString(0, "START_EMBEDDED_IPK");
            if (tmpS.empty()) {
                fprintf(stderr, "Invalid embedded IPK: missing initializer\n");
                return 0;
            }
            else if (tmpS != fname) {
                fprintf(stderr, "Invalid embedded IPK: invalid initializing name\n");
                return 0;
            }

            //Get embedded IPK info
            has_size = has_root = has_align = has_num = is_success = false;
            for (int t = 0; t < 4; ++t) {
                tmpS = this->inLog.getString();

                if (tmpS == "EMBEDDED_IPK_SIZE" && (int)(rsize = this->inLog.getUnsigned()) > 0) has_size = true;
                else if (tmpS == "ROOT_FOLDER" && !(troot = this->inLog.getString()).empty()) has_root = true;
                else if (tmpS == "ALIGNMENT" && (int)(talign = this->inLog.getUnsigned()) >= 0) has_align = true;
                else if (tmpS == "AMOUNT_ENTRIES" && (int)(tentries = this->inLog.getUnsigned()) > 0) has_num = true;
                else break;
            }
            
            fprintf(stderr, "EMBEDDED_IPK_SIZE 0x%08X\n", rsize);
            fprintf(stderr, "ROOT_FOLDER %s\n", troot.c_str());
            fprintf(stderr, "ALIGNMENT %i\n", talign);
            fprintf(stderr, "AMOUNT_ENTRIES %i\n\n", tentries);

            if (has_size && has_root && has_align && has_num) {
                rsize = rsize * 1.5;
                fdata = new unsigned char[rsize] {};

                do {
                    if (!(rsize = toIPK(troot, talign, tentries, level + 1, fdata))) {
                        fprintf(stderr, "Unable to pack embedded data specified in %s\n", fname.c_str());
                        break;
                    }

                    //Get embedded IPK footer
                    tmpS = this->inLog.getString(0, "END_EMBEDDED_IPK");
                    if (tmpS.empty()) {
                        fprintf(stderr, "Invalid embedded IPK: missing terminator\n");
                        break;
                    }
                    else if (tmpS != fname) {
                        fprintf(stderr, "Invalid embedded IPK: invalid terminating name\n");
                        break;
                    }
                    else is_success = true;
                } while (0);

                if (!is_success) { delete[] fdata; return 0; }
            }
            else {
                fprintf(stderr, "Invalid embedded IPK: missing crucial header data\n");
                return 0;
            }
        }

        //Set compression
        fsize = rsize;
        if (fext != "ipk") compress(fdata, rsize, out + dat_offs, fsize);
        
        if (!uni_keep_comp && (fsize >= rsize)) {
            for (int c = 0; c < fsize; ++c) {
                out[dat_offs + c] = (c < rsize) ? fdata[c] : 0;
            }
            fsize = rsize;
        }
        delete[] fdata;
        
        //Replace seperator with more the dumb one
        while (fpath.find("/") != std::string::npos) {
            fpath.replace(fpath.find("/"), 1, "\\");
        }

        //Update entry info
        fpath.copy((char*)(out + hoffs), 64);
        set_int(out + hoffs + 0x40, (rsize != fsize));
        set_int(out + hoffs + 0x44, fsize);
        set_int(out + hoffs + 0x48, dat_offs);
        set_int(out + hoffs + 0x4C, rsize);

        dat_offs += fsize;
        dat_offs += (!ialign) ? 0 : (ialign - (dat_offs % ialign)) % ialign;

        //Better distinguish between what's embedded and what isn't when printing
        for (int L = 0; L < level; ++L) fprintf(stdout, "    ");
        fprintf(stdout, "    PACKED %s TO ENTRY %d WITH COMPRESSION RATIO %%%.2f\n",
                        fname.c_str(), e, ((double)fsize / rsize) * 100);
    }

    //Update file size
    set_int(out + 0x0C, dat_offs);
    return dat_offs;
}

void ipk::searchLOG(std::string root, std::string name) {
    std::string tmp = this->inLog.getString(0, "START_ROOT_IPK"), iroot;
    bool has_size, has_root, has_align, has_num;
    unsigned isize, ialign, ientries;

    if (tmp.empty()) {
        fprintf(stderr, "%s is not a true IPKLOG file: missing initializer\n", name.c_str());
        return;
    }
    else if (name.find(tmp) == std::string::npos) {
        fprintf(stderr, "Invalid IPK: invalid initializing name\n");
        return;
    }
    
    has_size = has_root = has_align = has_num = false;
    for (int t = 0; t < 4; ++t) {
        tmp = this->inLog.getString();

        if (tmp == "ROOT_IPK_SIZE" && (int)(isize = this->inLog.getUnsigned()) > 0) has_size = true;
        else if (tmp == "ROOT_FOLDER" && !(iroot = this->inLog.getString()).empty()) has_root = true;
        else if (tmp == "ALIGNMENT" && (int)(ialign = this->inLog.getUnsigned()) >= 0) has_align = true;
        else if (tmp == "AMOUNT_ENTRIES" && (int)(ientries = this->inLog.getUnsigned()) > 0) has_num = true;
        else break;
    }
    
    if (has_size && has_root && has_align && has_num) {
        isize = isize * 1.5;
        unsigned char *idata = new unsigned char[isize] {};

        do {
            //Get files' data
            if (!(isize = toIPK(iroot, ialign, ientries, 0, idata))) {
                fprintf(stderr, "Unable to pack data specified in %s\n", name.c_str());
                break;
            }

            //Ensure log is true IPKLOG file
            tmp = this->inLog.getString(0, "END_ROOT_IPK");
            if (tmp.empty()) {
                fprintf(stderr, "%s is not a true IPKLOG file: missing terminator\n", name.c_str());
                break;
            }
            else if (name.find(tmp) == std::string::npos) {
                fprintf(stderr, "Invalid IPK: invalid terminating name\n");
                break;
            }

            //Save IPK and IPH to files
            name = name.substr(0, name.find_last_of('.'));
            if (!createFile((root + name + ".iph.new").c_str(), idata, (0x10 + (ientries * 0x50)))) {
                fprintf(stderr, "Unable to save %s.iph.new\n", name.c_str());
            }
            else if (!createFile((root + name + ".ipk.new").c_str(), idata, isize)) {
                fprintf(stderr, "Unable to save %s.ipk.new\n", name.c_str());
            }
            else fprintf(stdout, "IPH and IPK files successfully packed\n");
        } while (0);

        delete[] idata;
    }
    else fprintf(stderr, "Invalid IPK: missing crucial header data\n");
}
