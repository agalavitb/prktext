#include <string.h>
#include "prk.h"

typedef unsigned char uchar;
extern int prk_out(uchar *p, char *s, ...);
#define out prk_out

static unsigned long cksum(uchar *p, int n);
static unsigned long key(char *s);

static int write_map(struct prk *prk, uchar *p);
static int write_points(struct prk *prk, uchar *p);

static int putvalue(uchar *p, char *k, unsigned long value)
{
	if (value == 0)
		return out(p, "uu4", 0x12, key(k));
	if (value <= 255)
		return out(p, "uu4u", 0x10, key(k), (int) value);
	return out(p, "uu4u2", 0x11, key(k), (unsigned) value);
}

int prkwrite(struct prk *prk, uchar *p)
{
	uchar *hdr, *hdrend;

	hdr = p;
	if (prk->nobj*8LU + prk->ngap*48LU > PRK_GOLEN
	 || prk->x < 0 || prk->y < 0 || prk->nx < 0 || prk->ny < 0
	 || prk->x+prk->nx > 58 || prk->y+prk->ny > 58)
		return -1;
	p += out(p, ".4.4.4.4u4", 6LU);
	p += putvalue(p, "num_edited_goals", 0);
	p += putvalue(p, "maxplayers", prk->players);
	p += putvalue(p, "num_gaps", prk->ngap);
	p += putvalue(p, "num_pieces", prk->nobj);
	p += putvalue(p, "theme", prk->theme);
	p += out(p, "uu4u4", 0xd, key("tod_script"), key("default"));
	p += out(p, "b2u", "\x90\xef", (int) prk->nx);
	p += putvalue(p, "length", prk->ny);
	p += out(p, "uu4", 0x3, key("filename"));
	memcpy(p, prk->name, strlen(prk->name));
	p += strlen(prk->name);
	hdrend = p + 2;
	p += out(p, "..uu4b3", 0xc, key("park_editor_map"), "\x01\xa6\x0e");
	p += write_map(prk, p);
	p += write_points(prk, p);
	out(hdr, ".4.4u4u4", (long) (hdrend-hdr - 5*4), (long) (p-hdr));
	out(hdr, ".4u4", cksum(hdr + 4*5, hdrend-hdr - 4*5));
	out(hdr, "u4", cksum(hdr, p-hdr));
	memset(p, 0x69, PRKLEN - (p - hdr));
	return 0;
}

int write_map(struct prk *prk, uchar *p)
{
	struct prkobj *o;
	struct prkgap *g;
	uchar *hdr = p;
	int i;

	p += out(p, ".8 i2.. iiii i2 i2 i2 .6 s64",
	         prk->theme, prk->x, prk->y, prk->nx, prk->ny, prk->nobj,
	         prk->ngap, prk->players, prk->name);
	for (i = 0; i < 58*58; i++)
		p += out(p, "i", prk->ground[i%58*58 + i/58]);
	for (o = prk->obj; o < prk->obj+prk->nobj; o++)
		p += out(p, "u:10 u:6 .:2u:6 . u:2 u:6 ...",
		         o->id, o->x, o->y, o->rtn, o->z+16);
	for (g = prk->gap; g < prk->gap+prk->ngap; g++) {
		struct prkgap_side *s1 = &g->side[0];
		struct prkgap_side *s2 = &g->side[1];

		p += out(p, "ii ii ii ii i:4i:4. s32 u2 u4",
		         s1->x, s2->x, s1->z+16,s2->z+16, s1->y, s2->y, s1->len,
		         s2->len, s1->rtn, s2->rtn, g->name, g->score, g->type);
	}
	memset(p, 0, hdr+PRK_MAPLEN - p);
	out(hdr, ".4 i2 u2", (int) (p-hdr), 0x4e24);
	out(hdr, "u4", cksum(hdr+4, PRK_MAPLEN-4));
	return PRK_MAPLEN;
}

static int nrails(struct prk *prk)
{
	struct prkpoint *pt;
	int n;

	n = 0;
	for (pt = prk->pt; pt < prk->pt+prk->npt; pt++)
		n += !pt->cont;
	return n;
}

static int npoints(struct prk *prk, struct prkpoint *pt)
{
	int n = 0;

	do {
		n++;
		pt++;
	} while (pt < prk->pt+prk->npt && pt->cont);
	return n;
}

int write_points(struct prk *prk, uchar *p)
{
	struct prkpoint *pt;
	uchar *savep = p;

	p += out(p, "uu4uu4....uu4",
	         0xa, key("park_editor_goals"),
	         0xc, key("goals"),
	         0xc, key("createdrails"));
	if (prk->npt == 0) {
		p += out(p, "..");
		goto write_players;
	}
	p += out(p, "ui2", 0xa, nrails(prk));
	for (pt = prk->pt; pt < prk->pt+prk->npt; pt++) {
		if (!pt->cont) {
			p += out(p, prk->pt == pt ? "" : ".");
			p += out(p, "uu4ui2", 0xc, key("points"), 0xa,
			         npoints(prk, pt));
		}
		p += out(p, "uu4f4f4f4", 6, key("pos"), pt->x, pt->z, pt->y);
		if (pt->post)
			p += out(p, "u....u4", 0xd, key("haspost"));
		p += out(p, ".");
	}
write_players:
	p += out(p, ".uu4 i2", 0x10, key("maxplayers"), prk->players);
	return p - savep;
}

unsigned long key(char *s)
{
	return cksum((uchar *) s, strlen(s));
}

/* CRC-32. See <http://www.libpng.org/pub/png/spec/1.2/PNG-CRCAppendix.html> */
unsigned long cksum(uchar *p, int n)
{
	unsigned long c, d;
	int i, j;

	c = 0xffffffff;
	for (i = 0; i < n; i++) {
		d = (c ^ p[i]) & 0xff;
		for (j = 0; j < 8; j++)
			d = d&1 ? 0xedb88320LU ^ d>>1 : d >> 1;
		c = d ^ c>>8;
	}
	return c;
}
