project('zlib', 'c')
library_version = '1.2.11'

zlib = static_library(
	'zlib',
	[
		'src/adler32.c',
		'src/compress.c',
		'src/crc32.c',
		'src/deflate.c',
		'src/infback.c',
		'src/inffast.c',
		'src/inflate.c',
		'src/inftrees.c',
		'src/trees.c',
		'src/uncompr.c',
		'src/zutil.c',
	],
	include_directories : include_directories('src'),
	pic : true
)

zlib_dep = declare_dependency(
	include_directories : include_directories('src'),
	link_with : zlib
)
