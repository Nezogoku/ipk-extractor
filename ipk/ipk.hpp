#ifndef BLINXIPK_HPP
#define BLINXIPK_HPP

#include <string>
#include "stringstream.hpp"
#include "lzss_shared.hpp"

class ipk : protected blzss {
    public:
        ipk() = default;
        ~ipk() = default;
        void searchFile(std::string filename, int filetyp);

    private:
        void reset();
        template<typename... Args>
        std::string formatStr(const char *in, Args... args);
        void searchIPK(std::string root, std::string name, unsigned char *fdat, unsigned fsiz);
        void searchLOG(std::string root, std::string name);
        int fromIPK(unsigned char *idata, unsigned isize, std::string iroot);
        int toIPK(std::string iroot,
                  unsigned ialign, unsigned ientries,
                  unsigned level, unsigned char *out);

        std::string outLog;
        sstream inLog;
};

#endif
