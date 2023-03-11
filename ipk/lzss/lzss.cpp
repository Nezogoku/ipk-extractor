/**************************************************************
	LZSS.C -- A Data Compression Program
	(tab = 4 spaces)
***************************************************************
	4/6/1989 Haruhiko Okumura
	Use, distribute, and modify this program freely.
	Please send me your improved versions.
		PC-VAN		SCIENCE
		NIFTY-Serve	PAF01022
		CompuServe	74050,1022
**************************************************************/

#include "lzss.hpp"

//Just now realizing this is generic LZSS decompression... oops
void decompressLZSS(unsigned char *in, int in_size, unsigned char *&out, int out_size) {
	const int BUFFER_SIZE = 4096, MAX_COPY = 18;
	unsigned char *out_cur = out,
				  *out_end = out + out_size,
				  *in_cur = in,
				  *in_end = in + in_size,
				   buf[BUFFER_SIZE] = {},
				   cur = 0;

	if (in && out) {
		int shift = 8, rdx = BUFFER_SIZE - MAX_COPY;

		do {
			if (shift > 7) {
                cur = *(in_cur++);
                shift = 0;
                if (in_cur >= in_end) break;
            }

			if ((cur >> shift++) & 0x01) {
				if (in_cur >= in_end) break;

				*(out_cur++) = in_cur[0];
				buf[(rdx++ % BUFFER_SIZE)] = *(in_cur++);
			} else {
				if (in_cur >= in_end || (in_cur + 1) >= in_end) break;

				short toff, tsiz;
				toff = (in_cur[1] & 0xF0);
				toff = (toff << 4) | in_cur[0];
				tsiz = (in_cur[1] & 0x0F) + 3;

				while (tsiz--) {
					out_cur[0] = buf[(toff++ % BUFFER_SIZE)];
					buf[(rdx++ % BUFFER_SIZE)] = *(out_cur++);
				}

				in_cur += 2;
			}
		} while (in_cur < in_end && out_cur < out_end);
	}
}
