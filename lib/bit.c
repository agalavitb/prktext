#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char uchar;

/* parse:  extract a token, store the type in *type, the number of bits to  *
 *         operate on in *len, and return number of bytes to consume from s */
static int parse(int *type, int *len, char *s)
{
	char *savep;
	int bits;

	savep = s;
	s += strspn(s, " \t");
	*type = *s;
	if (strchr("0123456789:", *s++) != NULL)
		return 0;

	bits = 0;
	if (*s == ':') {
		bits = 1;
		s++;
	}
	*len = *s!='\0' && strchr("0123456789", *s) ? strtol(s, &s, 10) : 1;
	if (!bits)
		*len *= 8;

	return s - savep;
}

/* -------------------------------------------------------------------------- */

int bitput(uchar *p, char *s, ...)
{
	int type, len, n, bit;
	uchar buf[8192];
	va_list ap;

	bit = 0;
	va_start(ap, s);
	while ((n = parse(&type, &len, s)) > 0) {
		s += n, bit += len;
	}
	va_end(ap);
	return 0;
}

/* -------------------------------------------------------------------------- */

static unsigned long intu(uchar *p, int n);
static void copystr(char *to, uchar *p, int n);
static long ints(uchar *p, int bits);
static double flt(uchar *p, int bits);

static int writebits(uchar *to, uchar *p, int offset, int len)
{
	int i;

	to += offset / 8;
	offset %= 8;
	for (i = 0; len > 0; i++) {
		to[i] = p[i]<<offset | (to[i] & 0xff>>(8-offset));
		to[i+1] = p[i+1] >> 8-offset;
		len -= 8;
	}
	return i;
}

static int readbits(uchar *to, uchar *p, int offset, int len)
{
	int i;

	p += offset / 8;
	offset %= 8;
	for (i = 0; len > 0; i++) {
		to[i] = p[i] >> offset;
		to[i] |= p[i+1] << 8-offset;
		if (len < 8)
			to[i] &= 0xff >> 8-len;
		len -= 8;
	}
	return i;
}

int bitget(uchar *p, char *s, ...)
{
	int bit, n, type, len;
	uchar buf[8192];
	va_list ap;

	bit = 0;
	va_start(ap, s);
	while ((n = parse(&type, &len, s)) > 0) {
		readbits(buf, p, bit, len);
		if (type == 'u' && len <= 8*2)
			*va_arg(ap, unsigned *) = intu(buf, len);
		else if (type == 'u')
			*va_arg(ap, unsigned long *) = intu(buf, len);
		else if (type == 's')
			copystr(va_arg(ap, char *), buf, len);
		else if (type == 'b')
			memcpy(va_arg(ap, unsigned char *), buf, len);
		else if (type == 'f' && len <= 8*4)
			*va_arg(ap, float *) = flt(buf, len);
		else if (type == 'f')
			*va_arg(ap, double *) = flt(buf, len);
		else if (type == 'i' && len <= 8*2)
			*va_arg(ap, int *)  = ints(buf, len);
		else if (type == 'i')
			*va_arg(ap, long *) = ints(buf, len);
		s += n, bit += len;
	}
	va_end(ap);
	return (bit+7) / 8;
}

unsigned long intu(uchar *p, int n)
{
	unsigned long v;

	v = 0;
	while (n > 0)
		v = v<<8 | p[--n];
	return v;
}

void copystr(char *to, uchar *p, int n)
{
	memcpy(to, p, n);
	to[n] = '\0';
}

long ints(uchar *p, int bits)
{
	unsigned char buf[sizeof (unsigned long)];
	unsigned long value, sign;

	value = intu(buf, readbits(buf, p, 0, bits-1));
	sign = intu(buf, readbits(buf, p, bits-1, 1));
	return !sign ? value : -((1 << bits-1) - value);
}

static int expbits(int bits)
{
	if (bits == 16)
		return 5;
	if (bits == 32)
		return 8;
	return (int) (log(bits)/log(2)*4 + 0.5) - 13;
}

double flt(uchar *p, int bits)
{
	unsigned char buf[sizeof (double)];
	double value, sign, exp;
	int ebits;

	ebits = expbits(bits);
	value = intu(buf, readbits(buf, p, 0, bits-1-ebits));
	exp = intu(buf, readbits(buf, p, bits-1-ebits, ebits));
	sign = intu(buf, readbits(buf, p, bits-1, 1)) ? -1 : +1;

	value = 1.0 + value/(1 << bits-1-ebits);
	exp -= (1 << (ebits-1)) - 1;

	return sign * pow(2, exp) * value;
}
