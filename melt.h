#include <stdio.h>
#include <ctype.h>
#include <signal.h>

# include <sys/types.h>
# include <sys/stat.h>

/* define if sizeof(int) == 2                                           */
#undef INT_16_BITS

/* default Huffman values, define if you don't like the default        */
/* 0,0,1,2,6,19,34,0. These below are reasonably good also.            */
/* #define HUFVALUES 0,1,1,1,4,10,27,18                                */
/* Some definitions for faster bit-level I/O */
/* Assumptions: local variables "loclen" and "locbuf" are defined
 * via "DefBits";
 * AvailBits() is called before all bit input operations, the
 * maximum allowed argument for AvailBits() is bits(bitbuf) -7;
 * FlushBits() is called often enough while bit output operations;
 * KeepBits() is called as opposite to DefBits.
 */

#define bits(x) ((int)sizeof(x)*8)
#define BYSH  (bits(bitbuf)-8)
#define BISH  (bits(bitbuf)-1)

#define InitIO()        { overrun = bitlen = 0; bitbuf = 0; }

#define DefBits         register unsigned locbuf = bitbuf;\
register int loclen = bitlen

#define FillBits()   if (loclen <= bits(bitbuf) - 8) {\
	do {\
		locbuf |= (unsigned)(fgetc(in) & 0xFF) << (BYSH - loclen);\
		loclen += 8;\
	} while (loclen <= bits(bitbuf) - 8);\
if (feof(in)) overrun++;\
}

#define KeepBits()      bitbuf = locbuf, bitlen = loclen

/* GetX() macros may be used only in "var op= GetX();" statements !! */

#define GetBit()  /* var op= */locbuf >> BISH, locbuf <<= 1, loclen--

#define GetByte() /* var op= */locbuf >> BYSH, locbuf <<= 8, loclen -= 8

/* NB! `n' is used more than once here! */
#define GetNBits(n) /* var op= */ locbuf >> (bits(bitbuf) - (n)),\
	locbuf <<= (n), loclen -= (n)

typedef unsigned short us_t;
typedef unsigned char uc_t;

#define LOOKAHEAD       256     /* pre-sence buffer size */
#define MAXDIST         7936
#define WINSIZE         (MAXDIST + LOOKAHEAD)   /* must be a power of 2 */
#define WINMASK         (WINSIZE - 1)

#define THRESHOLD	2

#define N_CHAR2         (256 - THRESHOLD + LOOKAHEAD + 1) /* code: 0 .. N_CHARi - 1 */
#define T2              (N_CHAR2 * 2 - 1)       /* size of table */

#define ENDOF           256                     /* pseudo-literal */

uc_t    text_buf[WINSIZE + LOOKAHEAD - 1];/* cyclic buffer with an overlay */

char *strchr(), *strrchr();
void StartHuff(), init();
int DecodeChar(), DecodePosition();
#define MAX_FREQ        (us_t)0x8000 /* Tree update timing */
