#include <string.h>
#include "prk.h"

typedef unsigned char uchar;
extern int prk_out(uchar *p, char *s, ...);
#define out prk_out

static unsigned long cksum(uchar *p, int n);
static unsigned long key(char *s);

static int write_container(struct prk *prk, uchar *p, int n);
static int write_map(struct prk *prk, uchar *p);
static int write_points(struct prk *prk, uchar *p);

int prkwrite(struct prk *prk, uchar *p)
{
	uchar *hdr;

	if (prk->nobj*8LU + prk->ngap*48LU > PRK_GOLEN)
		return -1;
	hdr = p;
	p += out(p, "uu4uuu", 0xc, key("park_editor_map"), 0x1, 0xa6, 0xe);
	p += write_map(prk, p);
	p += write_points(prk, p);
	write_container(prk, hdr, p - hdr);
	return 0;
}

int write_container(struct prk *prk, uchar *p, int n)
{
	unsigned long container_len;
	uchar *hdr;

	hdr = p;
	container_len = 5*4 + 6*5 + 9 + 3 + 6 + 5 + strlen(prk->name) + 2;
	memmove(p + container_len, p, n);
	memset(hdr+container_len+n, 0x69, PRKLEN-container_len-n);
	p += out(p, ".4u4u4u4u4", cksum(p+container_len, n),
	         container_len - 5*4, container_len + n, 6LU);
	p += out(p, "uu4u", 0x10, key("num_edited_goals"), 0);
	p += out(p, "uu4u", 0x10, key("maxplayers"), prk->players);
	p += out(p, "uu4u", 0x10, key("num_gaps"), prk->ngap);
	p += out(p, "uu4u", 0x10, key("num_pieces"), prk->nobj);
	p += out(p, "uu4u", 0x10, key("theme"), prk->theme);
	p += out(p, "uu4u4", 0x0d, key("tod_script"), key("default"));
	p += out(p, "ub2",  0x90, "\xef\x1c");
	p += out(p, "uu4u", 0x10, key("length"), 0x20);
	p += out(p, "uu4s.", 0x3, key("filename"), prk->name);
	out(hdr, "u4", cksum(p, PRKLEN));
	return PRKLEN;
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
		         o->id, o->x, o->y, o->rtn, o->z);
	for (g = prk->gap; g < prk->gap+prk->ngap; g++) {
		struct prkgap_side *s1 = &g->side[0];
		struct prkgap_side *s2 = &g->side[1];

		p += out(p, "ii ii ii ii i:4i:4. s32 u2 u4",
		         s1->x, s2->x, s1->z, s2->z, s1->y, s2->y, s1->len,
		         s2->len, s1->rtn, s2->rtn, g->name, g->score, g->type);
	}
	out(hdr, "u4 i2 u2", cksum(hdr, p-hdr), (int) (p-hdr), 0x4e24);
	memset(p, 0, hdr+PRK_MAPLEN - p);
	return PRK_MAPLEN;
}

int write_points(struct prk *prk, uchar *p)
{
	struct prkpoint *pt;
	uchar *savep = p;
	int nrails;

	nrails = 0;
	for (pt = prk->pt; pt < prk->pt+prk->npt; pt++)
		nrails += !pt->cont;

	p += out(p, "uu4uu4uu4ui2",
	         0xa, key("park_editor_goals"),
	         0xc, key("goals"),
	         0xc, key("createdrails"),
	         0xa, nrails);
	for (pt = prk->pt; pt < prk->pt+prk->npt; pt++) {
		if (!pt->cont && pt > prk->pt)
			p += out(p, ".");
		if (!pt->cont)
			p += out(p, "uu4u", 0xc, key("points"), 0xa);
		p += out(p, "uu4f4f4f4", 6, key("pos"), pt->x, pt->z, pt->y);
		if (!pt->post)
			p += out(p, "u....u4", 0xd, key("haspost"));
		p += out(p, ".");
	}
	p += out(p, "uu4 i2", 0x10, key("maxplayers"), prk->players);
	return p - savep;
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
	return c ^ 0xffffffff;
}

unsigned long key(char *s)
{
	return cksum((uchar *) s, strlen(s)) ^ 0xffffffff;
}
