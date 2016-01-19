#define PRKLEN 35840L

#define PRK_MAPLEN 15000
#define PRK_HDRLEN 92
#define PRK_GNDLEN 58*58
#define PRK_GOLEN (PRK_MAPLEN - PRK_HDRLEN - PRK_GNDLEN)
#define PRK_OBJLEN 8
#define PRK_GAPLEN 48

struct prkobj {
	int id;
	int x;
	int y;
	int z;
	int rtn;
};

struct prkgap_side {
	int x;
	int y;
	int z;
	int len;
	int rtn;
};

struct prkgap {
	char name[33];
	unsigned score;
	unsigned long type;
	struct prkgap_side side[2];
};

struct prkpoint {
	double x;
	double y;
	double z;
	int post;
	int cont;
};

struct prk {
	char name[65];
	int theme;
	int players;
	int ground[58 * 58];
	int x;
	int y;
	int nx;
	int ny;
	struct prkobj obj[PRK_GOLEN / PRK_OBJLEN];
	int nobj;
	struct prkgap gap[PRK_GOLEN / PRK_GAPLEN];
	int ngap;
	struct prkpoint pt[(PRKLEN - PRK_MAPLEN) / (5 + 4*3 + 1)];
	int npt;
};

int prkread(struct prk *prk, unsigned char *buf);
int prkwrite(struct prk *prk, unsigned char *buf);

#ifdef EOF
  int prkinf(struct prk *prk, FILE *fin);
  int prkoutf(struct prk *prk, FILE *fout);
#endif

int prkin(struct prk *prk, char *path);
int prkout(struct prk *prk, char *path);
