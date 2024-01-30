#include <algorithm>
#include <cstdio>
#include <string>
#include "ipk/ipk.hpp"
#include "printpause.hpp"

int main(int argc, char *argv[]) {
    std::string prgm = argv[0];
    prgm.erase(remove(prgm.begin(), prgm.end(), '\"'), prgm.end());
    prgm = prgm.substr(prgm.find_last_of("\\/") + 1, prgm.find_last_of('.'));

    if (argc < 2) {
        fprintf(stderr, "Usage: %s [<infile(s).ipk/ipklog>]\n", prgm.c_str());
    }
    else {
        ipk test;
        for (int i = 1; i < argc; ++i) {
            std::string tmp = argv[i], tmpext;
            tmp.erase(remove(tmp.begin(), tmp.end(), '\"'), tmp.end());
            tmpext = tmp.substr(tmp.find_last_of('.') + 1);
            test.searchFile(tmp, (tmpext == "ipk") ? 0 : (tmpext == "ipklog") ? 1 : -1);
        }
    }
    sleep(10);

    return 0;
}
