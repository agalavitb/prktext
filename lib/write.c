#include "prk.h"

typedef unsigned char uchar;
extern int prk_out(uchar *p, char *s, ...);
#define out prk_out

int prkwrite(struct prk *prk, uchar *buf)
{
	return -1;  /* unimplemented */
}
