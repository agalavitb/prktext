#include <string.h>
#include "prk.h"

typedef unsigned char uchar;
extern int prk_in(uchar *p, char *s, ...);
#define in prk_in

static int read_map(struct prk *prk, uchar *p);
static int read_points(struct prk *prk, uchar *p, long n);

static uchar *find(uchar *p, long n, uchar *key, long kn)
{
	long i;

	for (i = 0; i <= n-kn; i++)
		if (memcmp(p+i, key, kn) == 0)
			return p+i;
	return NULL;
}

int prkread(struct prk *prk, uchar *buf)
{
	uchar map[] = { 0xc, 0x89, 0x52, 0x7c, 0x33, 0x1, 0xa6, 0xe };
	uchar rails[] = { 0xc, 0xa6, 0x50, 0x45, 0x24, 0xa };
	uchar *p;
	long n;

	if ((p = find(buf, PRKLEN, map, sizeof map)) == NULL
	 || (PRKLEN - (p+sizeof map - buf)) < PRK_MAPLEN
	 || read_map(prk, p + sizeof map))
		return 1;

	prk->npt = 0;

	if ((p = find(buf, PRKLEN, rails, sizeof rails)) != NULL
	 && read_points(prk, p+sizeof rails, PRKLEN - (p+sizeof rails - buf)))
		return 1;

	/* make object, gap, and point coordinates consistent with ground */
	for (n = 0; n < prk->nobj; n++)
		prk->obj[n].z -= 16;
	for (n = 0; n < prk->ngap; n++) {
		prk->gap[n].side[0].z -= 16;
		prk->gap[n].side[1].z -= 16;
	}
	for (n = 0; n < prk->npt; n++) {
		prk->pt[n].x = (prk->pt[n].x + 3480) / 120.0;
		prk->pt[n].y = (prk->pt[n].y + 3480) / 120.0;
		prk->pt[n].z /= 48.0;
	}

	return prk->x < 0 || prk->y < 0 || prk->nx < 0 || prk->ny < 0
	    || prk->x+prk->nx > 58 || prk->y+prk->ny > 58;
}

int read_map(struct prk *prk, uchar *p)
{
	struct prkobj *o;
	struct prkgap *g;
	int i;

	p += in(p, ".8 i2.. iiii i2 i2 i2 .6 s64",
	        &prk->theme, &prk->x, &prk->y, &prk->nx, &prk->ny,
	        &prk->nobj, &prk->ngap, &prk->players, &prk->name);
	if (prk->nobj*8LU + prk->ngap*48LU > PRK_GOLEN)
		return -1;
	for (i = 0; i < 58*58; i++)
		p += in(p, "i", &prk->ground[i%58*58 + i/58]);
	for (o = prk->obj; o < prk->obj+prk->nobj; o++)
		p += in(p, "u:10 u:6 .:2u:6 . u:2 u:6 ...",
		        &o->id, &o->x, &o->y, &o->rtn, &o->z);
	for (g = prk->gap; g < prk->gap+prk->ngap; g++)
		p += in(p, "ii ii ii ii i:4i:4. s32 u2 u4",
		        &g->side[0].x, &g->side[1].x, &g->side[0].z,
		        &g->side[1].z, &g->side[0].y, &g->side[1].y,
		        &g->side[0].len, &g->side[1].len, &g->side[0].rtn,
		        &g->side[1].rtn, &g->name, &g->score, &g->type);
	return 0;
}

int read_points(struct prk *prk, uchar *p, long n)
{
	uchar rail[] = { 0xc, 0xd6, 0x71, 0x45, 0xd8, 0xa };
	uchar point[] = { 0x6, 0x53, 0x19, 0x26, 0x7f };
	struct prkpoint pt;
	float x, y, z;

	if (n < 2 || memcmp(p, "\0\0", 2) == 0)
		return 0;
	p += 2, n -= 2;
	while (n >= 8 && memcmp(p, rail, sizeof rail) == 0) {
		p += 8, n -= 8;
		pt.cont = 0;
		while (n >= 5+4+4+4+10 && memcmp(p, point, sizeof point) == 0) {
			in(p, ".....f4f4f4", &x, &z, &y);
			pt.x = x, pt.z = z, pt.y = y;
			pt.post = p[5+4+4+4] == 0xd;
			prk->pt[prk->npt++] = pt;
			pt.cont = 1;
			p += 5+4+4+4+(pt.post ? 10 : 1);
			n -= 5+4+4+4+(pt.post ? 10 : 1);
		}
		p += (n >= 1), n -= (n >= 1);
	}
	return 0;
}
