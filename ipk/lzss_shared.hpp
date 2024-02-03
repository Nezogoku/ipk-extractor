#ifndef BLINXLZSS_SHARED_HPP
#define BLINXLZSS_SHARED_HPP

#define BUFFER_SIZE 4096
#define NODE_END BUFFER_SIZE
#define MIN_COPY 3
#define MAX_COPY 18

//Slightly edited code from the Okumura LZSS (de)compressor on segaretro
class blzss {
    public:
        void compress(unsigned char *in, unsigned in_size, unsigned char *out, unsigned &out_size);
        void decompress(unsigned char *in, unsigned in_size, unsigned char *out, unsigned out_size);

    private:
        void ResetNode();
        void InsertNode(int r);
        void DeleteNode(int p);

        unsigned char text_buf[BUFFER_SIZE + MAX_COPY - 1] {};
        int match_position, match_length,
            lson[BUFFER_SIZE + 1] {},
            rson[BUFFER_SIZE + 257] {},
            dad[BUFFER_SIZE + 1] {};
};

#endif
