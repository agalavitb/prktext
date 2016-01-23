#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <prk.h>

int fromtext(struct prk *prk);
int totext(struct prk *prk);

int main(int argc, char *argv[])
{
	static struct prk prk;
	char filename[sizeof prk.name + sizeof ".PRK"];

	if (argc > 2) {
		fprintf(stderr, "usage: prktext [park.PRK]\n");
		return EXIT_FAILURE;
	}
	if (argc == 2) {
		if (prkin(&prk, argv[1])) {
			fprintf(stderr, "%s: can't read PRK file\n", argv[1]);
			return EXIT_FAILURE;
		}
		if (totext(&prk)) {
			fprintf(stderr, "<stdout>: can't write to file\n");
			return EXIT_FAILURE;
		}
	} else {
		if (fromtext(&prk)) {
			fprintf(stderr, "<stdin>: can't read from file\n");
			return EXIT_FAILURE;
		}
		sprintf(filename, "%s.PRK", prk.name);
		if (prkout(&prk, filename)) {
			fprintf(stderr, "%s: can't write to file\n", filename);
			return EXIT_FAILURE;
		}
	}
	return 0;
}

int fromtext(struct prk *prk)
{
	struct prkpoint *p;
	struct prkgap *g;
	struct prkobj *o;
	char s[8192], *t;
	int i;

	if (scanf("%64[^\r\n] %d %d %d %d %d %d",
	          prk->name, &prk->theme, &prk->players,
	          &prk->x, &prk->y, &prk->nx, &prk->ny) != 7)
		return -1;
	for (i = 0; i < prk->nx*prk->ny; i++) {
		int x = prk->x + i%prk->nx;
		int y = prk->y + i/prk->nx;

		if (scanf("%d", &prk->ground[y*58 + x]) != 1)
			return -1;
	}

	p = prk->pt;
	o = prk->obj;
	g = prk->gap;
	while (fgets(s, sizeof s, stdin) != NULL) {
		struct prkgap_side *s1 = &g->side[0];
		struct prkgap_side *s2 = &g->side[1];

		t = s + strspn(s, " \t\r\n");
		if (sscanf(t, "o %d %d %d %d %d",
		           &o->x, &o->y, &o->z, &o->rtn, &o->id) == 5)
			o++;
		else if (sscanf(t, "p %d %d %lg %lg %lg", &p->cont, &p->post,
		           &p->x, &p->y, &p->z) == 5)
			p++;
		else if (sscanf(t, "g %d %d %d %d %d %d %d %d %d %d"
		              "%08lx %u%*c%32[^\r\n]",
		           &s1->x, &s1->y, &s1->z, &s1->len, &s1->rtn,
		           &s2->x, &s2->y, &s2->z, &s2->len, &s2->rtn,
		           &g->type, &g->score, g->name) == 13)
			g++;
		else if (*t != '\0')
			return -1;
	}
	prk->npt = p - prk->pt;
	prk->nobj = o - prk->obj;
	prk->ngap = g - prk->gap;
	return 0;
}

int totext(struct prk *prk)
{
	struct prkpoint *p;
	struct prkgap *g;
	struct prkobj *o;
	int i;

	printf("%s\n%d %d\n%d %d %d %d\n",
	       prk->name, prk->theme, prk->players,
	       prk->x, prk->y, prk->nx, prk->ny);

	for (i = 0; i < prk->nx*prk->ny; i++) {
		int x = prk->x + i%prk->nx;
		int y = prk->y + i/prk->nx;

		printf(" %3d", prk->ground[y*58 + x]);
		if (x+1 == prk->x+prk->nx)
			printf("\n");
	}

	for (o = prk->obj; o < prk->obj+prk->nobj; o++)
		printf("o %d %d %d %d %d\n", o->x, o->y, o->z, o->rtn, o->id);

	for (g = prk->gap; g < prk->gap+prk->ngap; g++) {
		struct prkgap_side *s1 = &g->side[0];
		struct prkgap_side *s2 = &g->side[1];

		printf("g %d %d %d %d %d  %d %d %d %d %d  %08lx %u %s\n",
		       s1->x, s1->y, s1->z, s1->len, s1->rtn,
		       s2->x, s2->y, s2->z, s2->len, s2->rtn,
		       g->type, g->score, g->name);
	}

	for (p = prk->pt; p < prk->pt+prk->npt; p++)
		printf("p %d %d %.17g %.17g %.17g\n", p->cont, p->post, p->x,p->y,p->z);

	return ferror(stdout);
}
