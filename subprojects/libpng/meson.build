project('libpng', 'c')
library_version = '1.6.37'

libpng = static_library(
	'libpng',
	[
		'src/png.c',
		'src/pngerror.c',
		'src/pngget.c',
		'src/pngmem.c',
		'src/pngpread.c',
		'src/pngread.c',
		'src/pngrio.c',
		'src/pngrtran.c',
		'src/pngrutil.c',
		'src/pngset.c',
		'src/pngtrans.c',
		'src/pngwio.c',
		'src/pngwrite.c',
		'src/pngwtran.c',
		'src/pngwutil.c'
	],
	c_args : [
		'-DPNG_ARM_NEON_OPT=0'
	],
	include_directories : include_directories('src', '../zlib/src'),
	pic : true
)

libpng_dep = declare_dependency(
	include_directories : include_directories('src'),
	link_with : libpng
)
