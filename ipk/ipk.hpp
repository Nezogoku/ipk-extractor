#ifndef BLINXIPK_HPP
#define BLINXIPK_HPP

#include <string>
#include "stringstream.hpp"
#include "lzss\lzss_shared.hpp"

class ipk : protected blzss {
    public:
        ipk() { outLog = ""; inLog = 0; }
        ~ipk() { if (!outLog.empty()) outLog.clear(); inLog = 0; }
        void searchFile(std::string filename, int filetyp);

    private:
        void setStr(unsigned char *&out, const char *in, int length);
        void setInt(unsigned char *&out, unsigned int in, int length);
        template<typename... Args>
        std::string formatStr(const char *in, Args... args);
        void searchIPK(std::string root, std::string name, unsigned char *fdat, unsigned fsiz);
        void searchLOG(std::string root, std::string name);
        int fromIPK(unsigned char *idata, unsigned isize, std::string iroot);
        int toIPK(std::string iroot,
                  unsigned ialign, unsigned ientries,
                  unsigned level, unsigned char *out);

        std::string outLog;
        sstream *inLog;
};

#endif
