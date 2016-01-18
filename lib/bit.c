#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char uchar;

static int expbits(int bits)
{
	if (bits == 16)
		return 5;
	if (bits == 32)
		return 8;
	return (int) (log(bits)/log(2)*4 + 0.5) - 13;
}

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

static int wrbits(uchar *to, uchar *p, int offset, int len)
{
	int i;

	to += offset / 8;
	offset %= 8;
	for (i = 0; len > 0; i++) {
		to[i] = p[i]<<offset | (to[i] & 0xff>>(8-offset));
		to[i+1] = p[i]>>(8-offset) | (to[i+1] & 0xff>>offset);
		len -= 8;
	}
	return i;
}

static void wruint(uchar *p, int n, unsigned long v);
static void wrint(uchar *p, int n, long v);
static void wrflt(uchar *p, int bits, double v);

int bitput(uchar *p, char *s, ...)
{
	int type, len, n, bit;
	uchar buf[8192];
	va_list ap;

	bit = 0;
	va_start(ap, s);
	while ((n = parse(&type, &len, s)) > 0) {
		if (type == 'u' && len <= 8*2)
			wruint(buf, (len+7)/8, va_arg(ap, unsigned));
		else if (type == 'u')
			wruint(buf, (len+7)/8, va_arg(ap, unsigned long));
		else if (type == 's')
			strncpy((char *) buf, va_arg(ap, char *), (len+7)/8);
		else if (type == 'b')
			memcpy(buf, va_arg(ap, void *), (len+7)/8);
		else if (type == 'f')
			wrflt(buf, len, va_arg(ap, double));
		else if (type == 'i' && len <= 8*2)
			wrint(buf, len, va_arg(ap, int));
		else if (type == 'i')
			wrint(buf, len, va_arg(ap, long));
		else
			memset(buf, 0, (len+7)/8);
		wrbits(p, buf, bit, len);
		s += n, bit += len;
	}
	va_end(ap);
	return (bit+7) / 8;
}

void wruint(uchar *p, int n, unsigned long v)
{
	int i;

	for (i = 0; i < n; i++) {
		p[i] = v;
		v >>= 8;
	}
}

void wrint(uchar *p, int bits, long v)
{
	wruint(p, (bits+7)/8, v < 0 ? (2LU << bits-1) - -v : (unsigned long) v);
}

void wrflt(uchar *p, int bits, double v)
{
	unsigned long sign, value;
	int ebits, expt;

	ebits = expbits(bits);
	if (sign = v < 0)
		v = -v;
	for (expt = 0; v >= 2.0; expt++)
		v /= 2.0;
	for (; v < 1.0; expt--)
		if (v != 0.0)
			v *= 2.0;
		else {
			wruint(p, (bits+7)/8, sign << bits-1);
			return;
		}
	value = (v - 1.0) * ((1LU << bits-1-ebits) + 0.5);
	expt += (1 << ebits-1) - 1;
	wruint(p, (bits+7)/8, sign << bits-1 | expt << bits-ebits-1 | value);
}

/* -------------------------------------------------------------------------- */

static unsigned long rduint(uchar *p, int n);
static void rdstr(char *to, uchar *p, int n);
static long rdint(uchar *p, int bits);
static double rdflt(uchar *p, int bits);

static int rdbits(uchar *to, uchar *p, int offset, int len)
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
		rdbits(buf, p, bit, len);
		if (type == 'u' && len <= 8*2)
			*va_arg(ap, unsigned *) = rduint(buf, (len+7)/8);
		else if (type == 'u')
			*va_arg(ap, unsigned long *) = rduint(buf, (len+7)/8);
		else if (type == 's')
			rdstr(va_arg(ap, char *), buf, (len+7)/8);
		else if (type == 'b')
			memcpy(va_arg(ap, unsigned char *), buf, (len+7)/8);
		else if (type == 'f' && len <= 8*4)
			*va_arg(ap, float *) = rdflt(buf, len);
		else if (type == 'f')
			*va_arg(ap, double *) = rdflt(buf, len);
		else if (type == 'i' && len <= 8*2)
			*va_arg(ap, int *)  = rdint(buf, len);
		else if (type == 'i')
			*va_arg(ap, long *) = rdint(buf, len);
		s += n, bit += len;
	}
	va_end(ap);
	return (bit+7) / 8;
}

unsigned long rduint(uchar *p, int n)
{
	unsigned long v;

	v = 0;
	while (n > 0)
		v = v<<8 | p[--n];
	return v;
}

void rdstr(char *to, uchar *p, int n)
{
	memcpy(to, p, n);
	to[n] = '\0';
}

long rdint(uchar *p, int bits)
{
	unsigned char buf[sizeof (unsigned long)];
	unsigned long value, sign;

	value = rduint(buf, rdbits(buf, p, 0, bits-1));
	sign = rduint(buf, rdbits(buf, p, bits-1, 1));
	return !sign ? value : -((1 << bits-1) - value);
}

double rdflt(uchar *p, int bits)
{
	unsigned char buf[sizeof (double)];
	double value, sign, exp;
	int ebits;

	ebits = expbits(bits);
	value = rduint(buf, rdbits(buf, p, 0, bits-1-ebits));
	exp = rduint(buf, rdbits(buf, p, bits-1-ebits, ebits));
	sign = rduint(buf, rdbits(buf, p, bits-1, 1)) ? -1 : +1;

	if (exp == 0) {
		value = value/(1 << bits-1-ebits);
		exp -= (1 << (ebits-1)) - 2;
	} else {
		value = 1.0 + value/(1 << bits-1-ebits);
		exp -= (1 << (ebits-1)) - 1;
	}

	return sign * pow(2, exp) * value;
}
