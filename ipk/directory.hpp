#ifndef DIRECTORY_HPP
#define DIRECTORY_HPP

#include <cstdio>
#include <errno.h>
#ifdef _WIN32
    #include <direct.h>
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define _mkdir(filename) mkdir(filename, 0777)
#endif


///Create a folder
static int createFolder(const char *folder) {
    int ret = 1;
    if (_mkdir(folder) < 0) ret = (errno == EEXIST);

    return ret;
}

///Write binary to a file
static int createFile(const char *file, unsigned char *data, uint32_t data_size) {
    int ret = 1;

    FILE *out = fopen(file, "wb");
    if (!out) ret = 0;
    else {
        fseek(out, 0, SEEK_SET);
        fflush(out);
        if (!fwrite(data, 1, data_size, out) || fclose(out)) ret = 0;
    }

    return ret;
}

///Write C-style string to a file
static int createFile(const char *file, const char *data, uint32_t data_size) {
    return createFile(file, (unsigned char*)data, data_size);
}

///Get data from a file
static int getFileData(const char *file, unsigned char *&data, uint32_t &data_size) {
    int ret = 1;

    FILE *in = fopen(file, "rb");
    if (!in) ret = 0;
    else {
        if (!data_size) {
            fseek(in, 0, SEEK_END);
            data_size = ftell(in);
        }
        fseek(in, 0, SEEK_SET);

        data = new unsigned char[data_size] {};

        if (!fread(data, 1, data_size, in) || fclose(in)) ret = 0;
    }

    return ret;
}


#endif
