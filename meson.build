project(
	'libgdtl',
	'cpp',
	meson_version : '>= 0.37'
)
library_version = '0.1.0'

godot_cpp = meson.get_compiler('cpp').find_library(get_option('godot-cpp-lib-name'), dirs: join_paths(meson.current_source_dir(), 'godot-cpp', 'bin'))

if (get_option('builtin-runtime'))
	ft = subproject('freetype2').get_variable('freetype2_dep')
	png = subproject('libpng').get_variable('libpng_dep')
	zlib = subproject('zlib').get_variable('zlib_dep')
	hb = subproject('harfbuzz', default_options : ['use-graphite2=' + get_option('use-graphite2').to_string()]).get_variable('harfbuzz_dep')
	icu = subproject('icu4c').get_variable('icu4c_dep')
else
	ft = dependency('freetype2', version : '>=2.9.1', static : get_option('static-runtime'))
	png = dependency('libpng', version : '>=1.6.36', static : get_option('static-runtime'))
	zlib = dependency('zlib', version : '>=1.2.11', static : get_option('static-runtime'))
	hb = dependency('harfbuzz-icu', version : '>=2.1.1', static : get_option('static-runtime'))
	icu = dependency('icu-uc', version : '>=62.1', static : get_option('static-runtime'))
endif

prefix = ''
cpp_args = []
link_args = []
dependencies = [
	godot_cpp,
	hb,
	icu,
	ft,
	png,
	zlib
]

sources = [
	'src/tl_core.cpp',
	'src/resources/tl_font.cpp',
	'src/resources/tl_bitmap_font.cpp',
	'src/resources/tl_dynamic_font.cpp',
	'src/resources/tl_font_family.cpp',
	'src/resources/tl_shaped_string.cpp',
	'src/resources/tl_shaped_attributed_string.cpp',
	'src/resources/tl_shaped_paragraph.cpp',
	'src/resources/tl_icu_data_loader.cpp',
	'src/controls/tl_proto_control.cpp'
]

if get_option('use-graphite2')
	if (get_option('builtin-runtime'))
		gr = subproject('graphite2').get_variable('graphite2_dep')
	else
		gr = dependency('graphite2', version : '>=1.3.12', static : get_option('static-runtime'))
	endif
	dependencies += [gr]
	cpp_args += ['-DHAVE_GRAPHITE2']
endif

if host_machine.system() == 'linux'
	cpp_args += [
		'-std=c++14',
		'-Wno-unused-parameter',
		'-Wno-sign-compare'
	]
	if get_option('static-runtime')
		link_args += [
			'-static-libgcc',
			'-static-libstdc++'
		]
	endif
	link_args += [
		'-ldl',
		'-Wl,--no-undefined',
		'-Wl,-R,\'\$ORIGIN\'',
	]
elif host_machine.system() == 'windows'
	if meson.get_compiler('cpp').get_id() == 'msvc'
		prefix = 'lib'
		cpp_args += [
			'/MD',
			'/TP',
			'/EHsc'
		]
		link_args += [
			'/WX'
		]
	else
		cpp_args += [
			'-std=c++14',
			'-Wno-unused-parameter',
			'-Wno-sign-compare'
		]
		if get_option('static-runtime')
			link_args += [
				'-static',
				'-static-libgcc',
				'-static-libstdc++'
			]
		endif
		link_args += [
			'-Wl,--no-undefined'
		]
	endif
elif host_machine.system() == 'darwin'
	cpp_args += [
		'-std=c++14',
		'-Wno-unused-parameter',
		'-Wno-sign-compare'
	]
	link_args += [
		'-framework', 'Cocoa',
		'-Wl,-undefined,dynamic_lookup'
	]
endif

shared_library(
	prefix + 'gdtl',
	sources,
	dependencies : dependencies,
	link_args : link_args,
	cpp_args : cpp_args,
	include_directories : include_directories('src', 'godot-cpp/include', 'godot-cpp/include/core', 'godot-cpp/include/gen', 'godot-cpp/godot_headers')
)
