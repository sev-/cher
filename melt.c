#include "melt.h"

unsigned bitbuf = 0;	/* Bit I/O buffers */
			/* use native word size, it's faster */

int  bitlen = 0;	/* Number of bits actually in `bitbuf' */
int  overrun = 0;	/* at least as many bytes were read as EOF */

/*********** for HUFFMAN ENCODING ***********/

/* TABLES OF ENCODE/DECODE for upper 6 bits position information */

/* The contents of `Table' are used for freezing only, so we use
 * it freely when melting.
 */

#ifndef HUFVALUES
#define HUFVALUES 0,0,1,2,6,19,34,0
#endif

uc_t Table2[9] = { 0, HUFVALUES };

/* d_len is really the number of bits to read to complete literal
 * part of position information.
 */
uc_t p_len[64];        /* These arrays are built accordingly to values */
uc_t d_len[256];       /* of `Table' above which are default, from the */
		      /* command line or from the header of frozen file */
uc_t code[256];

/* use arrays of native word-size elements to improve speed */

unsigned freq[T2 + 1];          /* frequency table */
int     son[T2];                /* points to son node (son[i],son[i+1]) */
int     prnt[T2 + N_CHAR2];     /* points to parent node */

/*
 * Melt in to out.
 */

void melt(in, out)
FILE *in, *out;
{
	register short    i, j, k, r, c;

/* Huffman-dependent part */
	if(read_header(in) == EOF)
		return;
	StartHuff(N_CHAR2);
	init(Table2);
/* end of Huffman-dependent part */

	InitIO();
	for (i = WINSIZE - LOOKAHEAD; i < WINSIZE; i++)
		text_buf[i] = ' ';
	r = 0;
	for (;; ) {
		c = DecodeChar(in);

		if (c == ENDOF)
			break;
		if (c < 256) {
			text_buf[r++] = c;
			r &= WINMASK;
			if (r == 0) {
				fwrite(text_buf, WINSIZE, 1, out);
				if (ferror(out))
					exit(1);
			}
		} else {
			i = (r - DecodePosition(in) - 1) & WINMASK;
			j = c - 256 + THRESHOLD;
			for (k = 0; k < j; k++) {
				c = text_buf[(i + k) & WINMASK];
				text_buf[r++] = c;
				r &= WINMASK;
				if (r == 0) {
					fwrite(text_buf, WINSIZE, 1, out);
					if (ferror(out))
						exit(1);
				}
			}
		}
	}
	if (r) {
		fwrite(text_buf, r, 1, out);
		if (ferror(out))
			exit(1);
	}
}

/*----------------------------------------------------------------------*/
/*									*/
/*		HUFFMAN ENCODING					*/
/*									*/
/*----------------------------------------------------------------------*/

/* notes :
   prnt[Tx .. Tx + N_CHARx - 1] used by
   indicates leaf position that corresponding to code.
*/

/* Initializes Huffman tree, bit I/O variables, etc.
   Static array is initialized with `table', dynamic Huffman tree
   has `n_char' leaves.
*/

static  int t, r, chars;

void StartHuff (n_char)
	int n_char;
{
	register int i, j;
	t = n_char * 2 - 1;
	r = t - 1;
	chars = n_char;

/* A priori frequences are 1 */

	for (i = 0; i < n_char; i++) {
		freq[i] = 1;
		son[i] = i + t;
		prnt[i + t] = i;
	}
	i = 0; j = n_char;

/* Building the balanced tree */

	while (j <= r) {
		freq[j] = freq[i] + freq[i + 1];
		son[j] = i;
		prnt[i] = prnt[i + 1] = j;
		i += 2; j++;
	}
	freq[t] = 0xffff;
	prnt[r] = 0;
}

/* Reconstructs tree with `chars' leaves */

void reconst ()
{
	register int i, j, k;
	register unsigned f;


/* correct leaf node into of first half,
   and set these freqency to (freq+1)/2
*/
	j = 0;
	for (i = 0; i < t; i++) {
		if (son[i] >= t) {
			freq[j] = (freq[i] + 1) / 2;
			son[j] = son[i];
			j++;
		}
	}
/* Build tree.  Link sons first */

	for (i = 0, j = chars; j < t; i += 2, j++) {
		k = i + 1;
		f = freq[j] = freq[i] + freq[k];
		for (k = j - 1; f < freq[k]; k--);
		k++;
		{       register unsigned *p, *e;
			for (p = &freq[j], e = &freq[k]; p > e; p--)
				p[0] = p[-1];
			freq[k] = f;
		}
		{       register int *p, *e;
			for (p = &son[j], e = &son[k]; p > e; p--)
				p[0] = p[-1];
			son[k] = i;
		}
	}

/* Link parents */
	for (i = 0; i < t; i++) {
		if ((k = son[i]) >= t) {
			prnt[k] = i;
		} else {
			prnt[k] = prnt[k + 1] = i;
		}
	}
}


/* Updates given code's frequency, and updates tree */

void update (c)
	register int c;
{
	register unsigned k, *p;
	register int    i, j, l;

	if (freq[r] == MAX_FREQ) {
		reconst();
	}
	c = prnt[c + t];
	do {
		k = ++freq[c];

		/* swap nodes when become wrong frequency order. */
		if (k > freq[l = c + 1]) {
			for (p = freq+l+1; k > *p++; ) ;
			l = p - freq - 2;
			freq[c] = p[-2];
			p[-2] = k;

			i = son[c];
			prnt[i] = l;
			if (i < t) prnt[i + 1] = l;

			j = son[l];
			son[l] = i;

			prnt[j] = c;
			if (j < t) prnt[j + 1] = c;
			son[c] = j;

			c = l;
		}
	} while ((c = prnt[c]) != 0);	/* loop until reach to root */
}

/* Decodes the literal or length info and returns its value.
	Returns ENDOF, if the file is corrupt.
*/

int DecodeChar (in)
FILE *in;
{
	register int c = r;
	DefBits;

	if (overrun >= sizeof(bitbuf)) {
		fprintf ( stderr, "corrupt input\n" );
		return ENDOF;
	}
	/* As far as MAX_FREQ == 32768, maximum length of a Huffman
	 * code cannot exceed 23 (consider Fibonacci numbers),
	 * so we don't need additional FillBits while decoding
	 * if sizeof(int) == 4.
	 */
	FillBits();
	/* trace from root to leaf,
	   got bit is 0 to small(son[]), 1 to large (son[]+1) son node */

	while ((c = son[c]) < t) {
		c += GetBit();
#ifdef INT_16_BITS
		if (loclen == 0)
			FillBits();
#endif
	}
	KeepBits();
	update(c -= t);
	return c;
}

/* Decodes the position info and returns it */

int DecodePosition (in)
FILE *in;
{
	register        i, j;
	DefBits;

	/* Upper 6 bits can be coded by a byte (8 bits) or less,
	 * plus 7 bits literally ...
	 */
	FillBits();
	/* decode upper 6 bits from the table */
	i = GetByte();
	j = (code[i] << 7) | (i << d_len[i]) & 0x7F;

	/* get lower 7 bits literally */
#ifdef INT_16_BITS
	FillBits();
#endif
	j |= GetNBits(d_len[i]);
	KeepBits();
	return j;
}

/* Initializes static Huffman arrays */

void init(table) uc_t * table; {
	short i, j, k, num;
	num = 0;

/* There are `table[i]' `i'-bits Huffman codes */

	for(i = 1, j = 0; i <= 8; i++) {
		num += table[i] << (8 - i);
		for(k = table[i]; k; j++, k--)
			p_len[j] = i;
	}
	if (num != 256) {
		fprintf(stderr, "Invalid position table\n");
		exit(1);
	}
	num = j;

/* Melting: building the table for decoding */

		for(k = j = 0; j < num; j ++)
			for(i = 1 << (8 - p_len[j]); i--;)
				code[k++] = j;

		for(k = j = 0; j < num; j ++)
			for(i = 1 << (8 - p_len[j]); i--;)
				d_len[k++] =  p_len[j] - 1;
}

/* Reconstructs `Table' from the header of the frozen file and checks
	its correctness. Returns 0 if OK, EOF otherwise.
*/

int read_header(in)
FILE *in;
{
	int i, j;
	i = fgetc(in) & 0xFF;
	i |= (fgetc(in) & 0xFF) << 8;
	Table2[1] = i & 1; i >>= 1;
	Table2[2] = i & 3; i >>= 2;
	Table2[3] = i & 7; i >>= 3;
	Table2[4] = i & 0xF; i >>= 4;
	Table2[5] = i & 0x1F; i >>= 5;

	if (i & 1 || (i = fgetc(in)) & 0xC0) {
		fprintf(stderr, "Unknown header format.\n");
		return EOF;
	}

	Table2[6] = i & 0x3F;

	i = Table2[1] + Table2[2] + Table2[3] + Table2[4] +
	Table2[5] + Table2[6];

	i = 62 - i;     /* free variable length codes for 7 & 8 bits */

	j = 128 * Table2[1] + 64 * Table2[2] + 32 * Table2[3] +
	16 * Table2[4] + 8 * Table2[5] + 4 * Table2[6];

	j = 256 - j;    /* free byte images for these codes */

	j -= i;
	if (j < 0 || i < j) {
		fprintf ( stderr, "corrupt input\n" );
		return EOF;
	}
	Table2[7] = j;
	Table2[8] = i - j;

	return 0;
}
