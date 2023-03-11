#include <algorithm>
#include <cstdio>
#include <string>
#include "ipk/ipk.hpp"

int main(int argc, char *argv[]) {
    std::string prgm = argv[0];
    prgm.erase(remove(prgm.begin(), prgm.end(), '\"'), prgm.end());
    prgm = prgm.substr(prgm.find_last_of("\\/") + 1, prgm.find_last_of('.'));

    if (argc < 2) {
        fprintf(stderr, "Usage: %s [<infile(s).ipk>]\n", prgm.c_str());
    }
    else {
        for (int i = 1; i < argc; ++i) {
            std::string tmp = argv[i];
            tmp.erase(remove(tmp.begin(), tmp.end(), '\"'), tmp.end());
            searchIPK(tmp);
        }
    }

    return 0;
}
