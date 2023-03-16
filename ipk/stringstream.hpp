#ifndef STRINGSTREAM_HPP
#define STRINGSTREAM_HPP

#include <string>

///Struct for custom stringstream
struct sstream {
    unsigned char *stream;
    unsigned strsize = 0, spos = 0, ssize = 0;

    std::string ssub; unsigned isub;
};

///Code for custom stringstream
static int getStream(sstream &str, const char *delim) {
    int ret = 0;
    bool quotes = 0;
    char sep = (delim) ? delim[0] : '\0';

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
        else if (str.stream[str.spos + str.ssize] == sep && !quotes) { str.ssize += 1; break; }
        else str.ssize += 1;
    }

    for (int c = 0; c < str.ssize; ++c) {
        if (str.stream[str.spos + c] == '\"' || str.stream[str.spos + c] == sep) continue;
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
static int getStream(sstream &str) { return getStream(str, 0); }

#endif
