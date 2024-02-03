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

//#include <cstdio>
#include "lzss_shared.hpp"

void blzss::ResetNode() {
    for (auto &t : this->text_buf) t = 0;
    for (auto &s : this->lson) s = 0;
    for (int i = 0; i < BUFFER_SIZE + 257; ++i) {
        this->rson[i] = (i > BUFFER_SIZE) ? NODE_END : 0;
    }
    for (auto &d : this->dad) d = NODE_END;
    this->dad[BUFFER_SIZE] = 0;
    this->match_position = this->match_length = 0;
}

void blzss::InsertNode(int r) {
	int i, p, cmp;
	unsigned char  *key;

	cmp = 1;
    key = &this->text_buf[r];
    p = BUFFER_SIZE + 1 + key[0];

	this->rson[r] = this->lson[r] = NODE_END;
    this->match_length = 0;

	while (true) {
		if (cmp >= 0) {
			if (this->rson[p] != NODE_END) p = this->rson[p];
			else { this->rson[p] = r; this->dad[r] = p; return; }
		} else {
			if (this->lson[p] != NODE_END) p = this->lson[p];
			else { this->lson[p] = r; this->dad[r] = p; return; }
		}

		for (i = 1; i < MAX_COPY; i++) if ((cmp = key[i] - this->text_buf[p + i]) != 0)  break;

        if (i > this->match_length) {
			this->match_position = p;
			if ((this->match_length = i) >= MAX_COPY)  break;
		}
	}
	this->dad[r] = this->dad[p];
    this->lson[r] = this->lson[p];
    this->rson[r] = this->rson[p];

	this->dad[this->lson[p]] = r;
    this->dad[this->rson[p]] = r;

	if (this->rson[this->dad[p]] == p) this->rson[this->dad[p]] = r;
	else this->lson[this->dad[p]] = r;

    dad[p] = NODE_END;
}

void blzss::DeleteNode(int p) {
	int  q;

	if (this->dad[p] == NODE_END) return;
	if (this->rson[p] == NODE_END) q = this->lson[p];
	else if (this->lson[p] == NODE_END) q = this->rson[p];
	else {
		q = this->lson[p];

        if (this->rson[q] != NODE_END) {
			do {  q = this->rson[q];  } while (this->rson[q] != NODE_END);

            this->rson[this->dad[q]] = this->lson[q];
            this->dad[this->lson[q]] = this->dad[q];
			this->lson[q] = this->lson[p];
            this->dad[this->lson[p]] = q;
		}

		this->rson[q] = this->rson[p];
        this->dad[this->rson[p]] = q;
	}
	this->dad[q] = this->dad[p];

	if (this->rson[this->dad[p]] == p) this->rson[this->dad[p]] = q;
    else this->lson[this->dad[p]] = q;

	this->dad[p] = NODE_END;
}

//Saxman compression
//Couldn't be bothered to rewrite, maybe later
void blzss::compress(unsigned char *in, unsigned in_size, unsigned char *out, unsigned &out_size) {
	unsigned char *out_cur = out,
				  *in_cur = in,
				  *in_end = in + in_size;

	if (in && out) {
		ResetNode();

        int  i, c, len, r, s, last_match_length, code_buf_ptr;
		out_size = 0;
		unsigned char  code_buf[17], mask;

		code_buf[0] = 0;
		code_buf_ptr = mask = 1;
		s = 0;
		r = BUFFER_SIZE - MAX_COPY;
		for (len = 0; len < MAX_COPY && (in_cur < in_end); len++) this->text_buf[r + len] = *(in_cur++);

		if (len == 0) return;

		for (i = 1; i <= MAX_COPY; i++) InsertNode(r - i);
		InsertNode(r);

		do {
			if (this->match_length > len) this->match_length = len;
			if (this->match_length < MIN_COPY) {
				this->match_length = 1;
				code_buf[0] |= mask;
				code_buf[code_buf_ptr++] = this->text_buf[r];
			}
			else {
				code_buf[code_buf_ptr++] = (unsigned char) this->match_position;
				code_buf[code_buf_ptr++] = (unsigned char)(((this->match_position >> 4) & 0xF0) |
                                           (this->match_length - MIN_COPY));
			}

			if ((mask <<= 1) == 0) {
				for (i = 0; i < code_buf_ptr; i++) *(out_cur++) = code_buf[i];
				out_size += code_buf_ptr;
				code_buf[0] = 0;  code_buf_ptr = mask = 1;
			}

			last_match_length = this->match_length;
			for (i = 0; i < last_match_length && (in_cur < in_end); i++) {
				DeleteNode(s);
				this->text_buf[s] = in_cur[0];

				if (s < MAX_COPY - 1) this->text_buf[s + BUFFER_SIZE] = in_cur[0];

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

//Saxman decompression
void blzss::decompress(unsigned char *in, unsigned in_size, unsigned char *out, unsigned out_size) {
	unsigned char *out_start = out, *out_cur = out, *out_end = out + out_size, *out_next = out,
				  *in_cur = in, *in_end = in + in_size,
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
