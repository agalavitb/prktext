#include <stdio.h>
#include <prk.h>

int prkinf(struct prk *prk, FILE *fin)
{
	unsigned char buf[PRKLEN] = { 0 };

	fread(buf, 1, sizeof buf, fin);
	return prkread(prk, buf) ? -1 : 0;
}

int prkin(struct prk *prk, char *path)
{
	FILE *fin;
	int r;

	if ((fin = fopen(path, "rb")) == NULL)
		return -2;
	r = prkinf(prk, fin);
	fclose(fin);
	return r;
}

int prkoutf(struct prk *prk, FILE *fout)
{
	unsigned char buf[PRKLEN] = { 0 };

	if (prkwrite(prk, buf))
		return -1;
	if (fwrite(buf, 1, sizeof buf, fout) != sizeof buf)
		return -3;
	return 0;
}

int prkout(struct prk *prk, char *path)
{
	FILE *fout;
	int r;

	if ((fout = fopen(path, "wb")) == NULL)
		return -2;
	r = prkoutf(prk, fout);
	if (fclose(fout))
		return -3;
	return r;
}
