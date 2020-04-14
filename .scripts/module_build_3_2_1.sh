#!/bin/bash

set -ev

scons --version
clang --version

cd ..
git clone --branch 3.2.1-stable --depth 1 https://github.com/godotengine/godot godot

if [ ! -d $(pwd)/godot/modules/godot_tl ]; then
	ln -s $(pwd)/godot_tl $(pwd)/godot/modules/godot_tl
fi

cd godot

git apply ./modules/godot_tl/patch_font_3_2_x.diff
git apply ./modules/godot_tl/patch_editor_fonts_3_2_x.diff
git apply ./modules/godot_tl/patch_dialog_3_2_x.diff
git apply ./modules/godot_tl/patch_lbl_3_2_x.diff
git apply ./modules/godot_tl/patch_le_3_2_x.diff

scons -j2 platform=x11 bits=64 target=debug tools=yes warnings=extra module_hdr_enabled=no module_bmp_enabled=no module_bullet_enabled=no module_jpg_enabled=no module_squish_enabled=no module_csg_enabled=no module_stb_vorbis_enabled=no module_cvtt_enabled=no module_mbedtls_enabled=no module_dds_enabled=no module_tga_enabled=no module_enet_enabled=no module_thekla_unwrap_enabled=no module_etc_enabled=no module_mobile_vr_enabled=no module_theora_enabled=no module_ogg_enabled=no module_upnp_enabled=no module_gdnative_enabled=no module_opensimplex_enabled=no module_opus_enabled=no module_vorbis_enabled=no module_pvr_enabled=no module_webm_enabled=no module_recast_enabled=no module_webp_enabled=no module_websocket_enabled=no module_gridmap_enabled=no module_xatlas_unwrap_enabled=no builtin_freetype=yes builtin_libpng=yes builtin_zlib=yes use_llvm=yes use_font_wrapper=true
