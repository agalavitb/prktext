cdef extern from "stdlib.h":
	void *malloc(size_t n)
	void free(void *p)

cdef extern from "prk.h":
	struct prkobj:
		int id
		int x
		int y
		int z
		int rtn

	struct prkgap_side:
		int x
		int y
		int z
		int len
		int rtn
		
	struct prkgap:
		char name[32]
		unsigned score
		unsigned long type
		prkgap_side side[2]

	struct prkpoint:
		float x
		float y
		float z
		int post
		int cont

	struct prk:
		char name[64]
		int theme
		int players
		int ground[58*58]
		int x
		int y
		int nx
		int ny
		prkobj obj[(15000 - 92 - 58*58) // 8]
		int nobj
		prkgap gap[(15000 - 92 - 58*58) // 48]
		int ngap
		prkpoint pt[(35840 - 15000) // 18];
		int npt

	int prkread(prk *prk, unsigned char *buf)
	int prkwrite(prk *prk, unsigned char *buf)

cdef copystr(char *to, s):
	s = s.encode('ASCII')
	for i in range(len(s)):
		to[i] = s[i]
	to[len(s)] = 0

class PrkObject:
	def __init__(self, id, x, y, z, rtn=0):
		self.id = id
		self.x = x
		self.y = y
		self.z = z
		self.rtn = rtn

	def __repr__(self):
		return 'PrkObject(%d (%d,%d,%d))' % (self.id, self.x, self.y, self.z)

class PrkGap:
	def __init__(self, x1, x2, y1, y2, z1, z2, len1=0, len2=0, rtn1=0,
	             rtn2=0, name='gap', score=100, type=1):
		self.x1 = x1
		self.x2 = x2
		self.y1 = y1
		self.y2 = y2
		self.z1 = z1
		self.z2 = z2
		self.len1 = len1
		self.len2 = len2
		self.rtn1 = rtn1
		self.rtn2 = rtn2
		self.name = name
		self.score = score
		self.type = type

	def __repr__(self):
		return 'PrkGap("%s")' % self.name

class PrkPoint:
	def __init__(self, x, y, z, post=True):
		self.x = x
		self.y = y
		self.z = z
		self.post = post

	def __repr__(self):
		return 'PrkPoint(%g,%g,%g %c)' % (self.x, self.y, self.z,
		                        '|' if self.post else ' ')

class PrkException(Exception):
	def __init__(self, msg):
		self.msg = msg
	def __str__(self):
		return self.msg

class Prk:
	def __init__(self, str=None):
		self.name = 'unnamed park'
		self.theme = 0
		self.players = 8
		self.setcoords(24, 24)
		self.ground_data = [0] * (58*58)
		self.objects = []
		self.gaps = []
		self.rails = []
		if str != None:
			self.frombytes(str)

	def __repr__(self):
		return 'Prk("%s")' % self.name

	def ground(self, x, y, value=None):
		x = int(x)
		y = int(y)
		if value == None:
			return self.ground_data[y*58 + x]
		self.ground_data[y*58 + x] = value

	def setcoords(self, nx, ny, x=None, y=None):
		if x == None:
			x = int(58/2 - (nx/2))
		if y == None:
			y = int(58/2 - (ny/2))
		self.x, self.y = x, y
		self.nx, self.ny = nx, ny

	def addobject(self, id, x, y, z=None, rtn=0):
		if z == None:
			z = self.ground(x, y)
		self.objects.append(PrkObject(id, x, y, z, rtn))
		return self.objects[-1]

	def addgap(self, x1, x2, y1, y2, z1=None, z2=None, len1=0, len2=0,
	            rtn1=0, rtn2=0, name=None, score=100, type=1):
		if name == None:
			name = 'gap'
			if self.gaps:
				name += ' %d' % len(self.gaps)
		if z1 == None:
			z1 = self.ground(x1, y1) - 8
		if z2 == None:
			z3 = self.ground(x2, y2) - 8
		self.gaps.append(PrkGap(x1, x2, y1, y2, z1, z2, len1, len2,
		                        rtn1, rtn2, name, score, type))
		return self.gaps[-1]

	def addrail(self, points=[]):
		self.rails.append(points.copy())
		return self.rails[-1]

	def addpoint(self, rail, x, y, z=None, post=True):
		if z == None:
			z = self.ground(x, y) + 0.416
		rail.append(PrkPoint(x, y, z, post))
		return rail[-1]

	def tobytes(self):
		cdef unsigned char *p
		cdef prk pk

		copystr(pk.name, self.name)
		pk.theme = self.theme;
		pk.players = self.players
		pk.x, pk.y = self.x, self.y
		pk.nx, pk.ny = self.nx, self.ny
		for i in range(58*58):
			pk.ground[i] = self.ground_data[i]
		pk.nobj = len(self.objects)
		for i in range(len(self.objects)):
			pk.obj[i].id = self.objects[i].id
			pk.obj[i].x = self.objects[i].x
			pk.obj[i].y = self.objects[i].y
			pk.obj[i].z = self.objects[i].z
			pk.obj[i].rtn = self.objects[i].rtn
		pk.ngap = len(self.gaps)
		for i in range(len(self.gaps)):
			pk.gap[i].side[0].x = self.gaps[i].x1
			pk.gap[i].side[1].x = self.gaps[i].x2
			pk.gap[i].side[0].y = self.gaps[i].y1
			pk.gap[i].side[1].y = self.gaps[i].y2
			pk.gap[i].side[0].z = self.gaps[i].z1
			pk.gap[i].side[1].z = self.gaps[i].z2
			pk.gap[i].side[0].len = self.gaps[i].len1
			pk.gap[i].side[1].len = self.gaps[i].len2
			pk.gap[i].side[0].rtn = self.gaps[i].rtn1
			pk.gap[i].side[1].rtn = self.gaps[i].rtn2
			pk.gap[i].score = self.gaps[i].score
			pk.gap[i].type = self.gaps[i].type
			copystr(pk.gap[i].name, self.gaps[i].name)
		i = 0
		for rail in self.rails:
			for point in rail:
				pk.pt[i].x = point.x
				pk.pt[i].y = point.y
				pk.pt[i].z = point.z
				pk.pt[i].post = point.post
				pk.pt[i].cont = 1 if point!=rail[0] else 0
				i += 1
		pk.npt = i
		p = <unsigned char *> malloc(35840)
		if not p:
			raise MemoryError()
		if prkwrite(&pk, p) != 0:
			free(p)
			raise PrkException("can't write park")
		s = b''
		for i in range(35840):
			s += bytes([p[i]])
		free(p)
		return s

	def frombytes(self, s):
		cdef unsigned char *p
		cdef prk pk

		p = <unsigned char *> malloc(35840)
		if not p:
			raise MemoryError()
		for i in range(35840):
			p[i] = s[i]
		if prkread(&pk, p) != 0:
			free(p)
			raise PrkException("can't read park")
		free(p)
		self.name = bytes.decode(pk.name, 'ASCII')
		self.theme = pk.theme
		self.players = pk.players
		self.x, self.y = pk.x, pk.y
		self.nx, self.ny = pk.nx, pk.ny
		for i in range(58*58):
			self.ground_data[i] = pk.ground[i]
		for i in range(pk.nobj):
			self.addobject(pk.obj[i].id, pk.obj[i].x,
			                pk.obj[i].y, pk.obj[i].z,
			                pk.obj[i].rtn)
		for i in range(pk.ngap):
			self.addgap(pk.gap[i].side[0].x, pk.gap[i].side[1].x,
			            pk.gap[i].side[0].y, pk.gap[i].side[1].y,
			            pk.gap[i].side[0].z, pk.gap[i].side[1].z,
			            pk.gap[i].side[0].len,
			            pk.gap[i].side[1].len,
			            pk.gap[i].side[0].rtn,
			            pk.gap[i].side[1].rtn,
			            bytes.decode(pk.gap[i].name, 'ASCII'),
			            pk.gap[i].score,
			            pk.gap[i].type)
		rail = []
		for i in range(pk.npt):
			if not pk.pt[i].cont:
				rail = self.addrail()
			self.addpoint(rail, pk.pt[i].x, pk.pt[i].y, pk.pt[i].z,
			              pk.pt[i].post)

