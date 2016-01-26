from Cython.Distutils import build_ext
from distutils.core import setup
from distutils.extension import Extension

files = ['pyprk.pyx', '../lib/bit.c', '../lib/read.c', '../lib/write.c']

setup(
	name = 'prk',
	version = '1',
	description = 'A THUG2 PRK reader and writer.',
	url = 'github.com/wizfiz/prktext.git',
	cmdclass = {'build_ext': build_ext},
	ext_modules = [
		Extension('prk', files,
		          extra_compile_args = [
		                '-I..',
		                '-Dbitput=prk_out',
		                '-Dbitget=prk_in'
		          ],
		)
	]
)
