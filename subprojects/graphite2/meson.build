project('graphite2', 'cpp')
library_version = '1.3.12'

src = [
	'src/gr_char_info.cpp',
	'src/gr_face.cpp',
	'src/gr_features.cpp',
	'src/gr_font.cpp',
	'src/gr_logging.cpp',
	'src/gr_segment.cpp',
	'src/gr_slot.cpp',
	'src/CmapCache.cpp',
	'src/Code.cpp',
	'src/Collider.cpp',
	'src/Decompressor.cpp',
	'src/Face.cpp',
	'src/FeatureMap.cpp',
	'src/Font.cpp',
	'src/GlyphCache.cpp',
	'src/GlyphFace.cpp',
	'src/Intervals.cpp',
	'src/Justifier.cpp',
	'src/NameTable.cpp',
	'src/Pass.cpp',
	'src/Position.cpp',
	'src/Segment.cpp',
	'src/Silf.cpp',
	'src/Slot.cpp',
	'src/Sparse.cpp',
	'src/TtfUtil.cpp',
	'src/UtfCodec.cpp',
	'src/FileFace.cpp',
	'src/json.cpp'
]

cpp_args = [
	'-DGRAPHITE2_STATIC',
	'-DGRAPHITE2_NTRACING',
	'-DGRAPHITE2_NFILEFACE'
]

if meson.get_compiler('cpp').get_id() != 'msvc'
	cpp_args += [
		'-std=c++11',
		'-Wno-unknown-pragmas',
		'-fno-rtti',
		'-fno-exceptions'
	]
	src += ['src/direct_machine.cpp']
else
	src += ['src/call_machine.cpp']
endif

graphite2 = static_library(
	'graphite2',
	src,
	cpp_args : cpp_args,
	include_directories : include_directories('src', 'include'),
	pic : true
)

graphite2_dep = declare_dependency(
	include_directories : include_directories('include'),
	link_with : graphite2
)
