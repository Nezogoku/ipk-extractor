#ifndef DIRECTORY_HPP
#define DIRECTORY_HPP

#include <cstdio>
#include <direct.h>


///Create a folder
static int createFolder(const char *folder) {
    if (!folder || !folder[0]) return 0;

    _mkdir(folder);

    return 1;
}

///Write to a file
static int createFile(const char *file, unsigned char *data, uint32_t data_size) {
    if (!data || !data_size) return 0;
    else if (!file || !file[0]) return 0;

    FILE *out = fopen(file, "wb");
    if (!out) return 0;

    fseek(out, 0, SEEK_SET);
    fflush(out);
    if (!fwrite(data, 1, data_size, out)) return 0;
    else if (fclose(out)) return 0;

    return 1;
}

///Get data from a file
static int getFileData(const char *file, unsigned char *&data, uint32_t &data_size) {
    if (!file || !file[0]) return 0;

    FILE *in = fopen(file, "rb");
    if (!in) return 0;

    if (!data_size) {
        fseek(in, 0, SEEK_END);
        data_size = ftell(in);
    }
    fseek(in, 0, SEEK_SET);

    data = new unsigned char[data_size];
    if (!fread(data, 1, data_size, in)) return 0;
    else if (fclose(in)) return 0;

    return 1;
}


#endif
