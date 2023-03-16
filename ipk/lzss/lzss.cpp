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

#include <cstdio>
#include "lzss.hpp"

#define BUFFER_SIZE 4096
#define NODE_END BUFFER_SIZE
#define MIN_COPY 3
#define MAX_COPY 18


unsigned char text_buf[BUFFER_SIZE + MAX_COPY - 1];
int match_position, match_length,
	lson[BUFFER_SIZE + 1], rson[BUFFER_SIZE + 257], dad[BUFFER_SIZE + 1];

void InsertNode(int r) {
	int i, p, cmp;
	unsigned char  *key;

	cmp = 1;
    key = &text_buf[r];
    p = BUFFER_SIZE + 1 + key[0];

	rson[r] = lson[r] = NODE_END;
    match_length = 0;

	while (true) {
		if (cmp >= 0) {
			if (rson[p] != NODE_END) p = rson[p];
			else { rson[p] = r; dad[r] = p; return; }
		} else {
			if (lson[p] != NODE_END) p = lson[p];
			else { lson[p] = r; dad[r] = p; return; }
		}

		for (i = 1; i < MAX_COPY; i++) if ((cmp = key[i] - text_buf[p + i]) != 0)  break;

        if (i > match_length) {
			match_position = p;
			if ((match_length = i) >= MAX_COPY)  break;
		}
	}
	dad[r] = dad[p];
    lson[r] = lson[p];
    rson[r] = rson[p];

	dad[lson[p]] = r;
    dad[rson[p]] = r;

	if (rson[dad[p]] == p) rson[dad[p]] = r;
	else                   lson[dad[p]] = r;

    dad[p] = NODE_END;
}

void DeleteNode(int p) {
	int  q;

	if (dad[p] == NODE_END) return;
	if (rson[p] == NODE_END) q = lson[p];
	else if (lson[p] == NODE_END) q = rson[p];
	else {
		q = lson[p];

        if (rson[q] != NODE_END) {
			do {  q = rson[q];  } while (rson[q] != NODE_END);

            rson[dad[q]] = lson[q];
            dad[lson[q]] = dad[q];
			lson[q] = lson[p];
            dad[lson[p]] = q;
		}

		rson[q] = rson[p];
        dad[rson[p]] = q;
	}
	dad[q] = dad[p];

	if (rson[dad[p]] == p) rson[dad[p]] = q;
    else lson[dad[p]] = q;

	dad[p] = NODE_END;
}

//Saxman compression
//Couldn't be bothered to rewrite, maybe later
void compressLZSS(unsigned char *in, unsigned in_size, unsigned char *out, unsigned &out_size) {
	unsigned char *out_cur = out,
				  *in_cur = in,
				  *in_end = in + in_size;

	if (in && out) {
		int  i, c, len, r, s, last_match_length, code_buf_ptr;
		out_size = 0;
		unsigned char  code_buf[17], mask;

		for (int i = BUFFER_SIZE + 1; i <= BUFFER_SIZE + 256; i++) rson[i] = NODE_END;
		for (int i = 0; i < BUFFER_SIZE; i++) dad[i] = NODE_END;

		code_buf[0] = 0;
		code_buf_ptr = mask = 1;
		s = 0;
		r = BUFFER_SIZE - MAX_COPY;
		for (i = s; i < r; i++) text_buf[i] = 0;
		for (len = 0; len < MAX_COPY && (in_cur < in_end); len++) text_buf[r + len] = *(in_cur++);

		if (len == 0) return;

		for (i = 1; i <= MAX_COPY; i++) InsertNode(r - i);
		InsertNode(r);

		do {
			if (match_length > len) match_length = len;
			if (match_length < MIN_COPY) {
				match_length = 1;
				code_buf[0] |= mask;
				code_buf[code_buf_ptr++] = text_buf[r];
			}
			else {
				code_buf[code_buf_ptr++] = (unsigned char) match_position;
				code_buf[code_buf_ptr++] = (unsigned char)(((match_position >> 4) & 0xf0) | (match_length - MIN_COPY));
			}

			if ((mask <<= 1) == 0) {
				for (i = 0; i < code_buf_ptr; i++) *(out_cur++) = code_buf[i];
				out_size += code_buf_ptr;
				code_buf[0] = 0;  code_buf_ptr = mask = 1;
			}

			last_match_length = match_length;
			for (i = 0; i < last_match_length && (in_cur < in_end); i++) {
				DeleteNode(s);
				text_buf[s] = in_cur[0];

				if (s < MAX_COPY - 1) text_buf[s + BUFFER_SIZE] = in_cur[0];

				s = (s + 1) & (BUFFER_SIZE - 1);
				r = (r + 1) & (BUFFER_SIZE - 1);
				InsertNode(r);

				in_cur += 1;
			}

			while (i++ < last_match_length) {
				DeleteNode(s);
				s = (s + 1) & (BUFFER_SIZE - 1);
				r = (r + 1) & (BUFFER_SIZE - 1);
				if (--len) InsertNode(r);
			}
		} while (len > 0);

		if (code_buf_ptr > 1) {
			for (i = 0; i < code_buf_ptr; i++) *(out_cur++) = code_buf[i];
			out_size += code_buf_ptr;
		}
	}
}
/*
void compressLZSS(unsigned char *in, unsigned in_size, unsigned char *out, unsigned &out_size) {
	unsigned char

	if (in && out) {
		do {

            for (int s = 0, c, max_c, t, toff; s < 8;) {
                max_c = (in_end - in_cur < MAX_COPY) ? in_end - in_cur : MAX_COPY;
				cpy = 1;

				t = (in_cur - in) - BUFFER_SIZE;
				while ((in_next + t) < in_cur) {
					c = 0;

					while (t >= -MAX_COPY && in_cur[c] == in_next[t + c]) {
						if (++c >= max_c) break;
					}
					if (c > cpy) { cpy = c; toff = t; }

					t = t + 1;
                }

				if (cpy < MIN_COPY) {
					while (cpy--) {
						if (s >= 8) break;

						fprintf(stderr, "Raw byte to offset 0x%08X: 0x%02X\n", (out_next-out), in_cur[0]);

						out_cur[0] = out_cur[0] | (0x01 << s++);
						*(out_next++) = *(in_cur++);
					}
                }
                else {
					out_cur[0] = out_cur[0] | (0x00 << s++);

					//Check if all zereos
					if (cpy < MAX_COPY) {
						bool all_zero = true;
						for (int z = 0; z < cpy; ++z) {
							if (in_next[toff + z]) { all_zero = false; break; }
						}
						if (all_zero) toff = 0 - cpy;
					}

					short maddr = toff & (BUFFER_SIZE - 1);
					maddr = maddr - MAX_COPY;

					unsigned short adcp = 0x0000;
					adcp = adcp | ((maddr << 8) & 0xFF00);
					adcp = adcp | ((maddr >> 4) & 0x00F0);
					adcp = adcp | ((cpy - MIN_COPY) & 0x000F);

					fprintf(stderr, "Adr byte to offset 0x%08X: 0x%04X\n", (out_next-out), adcp & 0xFFFF);

					*(out_next++) = (adcp >> 8) & 0xFF;
					*(out_next++) = (adcp >> 0) & 0xFF;

					in_cur = in_cur + cpy;
                }
            }
            fprintf(stderr, "\n");

            out_cur = out_next;
            out_next = out_cur + 1;
		} while (in_cur < in_end);

		out_size = out_cur - out;
	}
}
*/

//Saxman decompression
void decompressLZSS(unsigned char *in, unsigned in_size, unsigned char *out, unsigned out_size) {
	unsigned char *out_cur = out,
				  *out_end = out + out_size,
				  *out_next = out,
				  *in_cur = in,
				  *in_end = in + in_size,
				   cur = 0;

	if (in && out) {
		int shift = 8;

		do {
			if (shift > 7) {
                cur = *(in_cur++);
                shift = 0;
                if (in_cur >= in_end) break;
            }

			if ((cur >> shift++) & 0x01) {
				if (in_cur >= in_end) break;

				*(out_cur++) = *(in_cur++);
			} else {
				if (in_cur >= in_end || (in_cur + 1) >= in_end) break;

				unsigned tsiz;
				tsiz = (in_cur[1] & 0x0F) + MIN_COPY;
				signed toff;
				toff = (in_cur[1] & 0xF0);
				toff = (toff << 4) | in_cur[0];
				toff = (toff + MAX_COPY) % BUFFER_SIZE;

				while (tsiz--) {
					unsigned ocur = out_cur - out;
					toff = ((toff - ocur) % BUFFER_SIZE) + ocur - BUFFER_SIZE;

					if (toff < 0) out_next = out_end;
					else out_next = out;

					*(out_cur++) = out_next[toff++];
				}

				in_cur += 2;
			}
		} while (in_cur < in_end && out_cur < out_end);
	}
}
