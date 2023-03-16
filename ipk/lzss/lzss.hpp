#ifndef BLINXLZSS_HPP
#define BLINXLZSS_HPP

//Slightly edited code from the Okumura LZSS (de)compressor on segaretro
void compressLZSS(unsigned char *in, unsigned in_size, unsigned char *out, unsigned &out_size);
void decompressLZSS(unsigned char *in, unsigned in_size, unsigned char *out, unsigned out_size);

#endif
