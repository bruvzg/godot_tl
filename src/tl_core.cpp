/*************************************************************************/
/*  tl_core.cpp                                                          */
/*************************************************************************/

#include "tl_core.hpp"

#include "resources/tl_bitmap_font.hpp"
#include "resources/tl_dynamic_font.hpp"
#include "resources/tl_font.hpp"
#include "resources/tl_font_family.hpp"

#include "resources/tl_shaped_attributed_string.hpp"
#include "resources/tl_shaped_string.hpp"

#include "resources/tl_shaped_paragraph.hpp"

#include "resources/tl_icu_data_loader.hpp"

#include "controls/tl_proto_control.hpp"

#include <unicode/uclean.h>
#include <unicode/udata.h>

using namespace godot;

#ifdef GODOT_MODULE

void register_godot_tl_types() {

	TLFontFace::initialize_hex_font();

	ClassDB::register_class<TLICUDataLoader>();
	ClassDB::register_class<TLFontFace>();
	ClassDB::register_class<TLBitmapFontFace>();
	ClassDB::register_class<TLDynamicFontFace>();
	ClassDB::register_class<TLFontFamily>();
	ClassDB::register_class<TLShapedString>();
	ClassDB::register_class<TLShapedAttributedString>();
	ClassDB::register_class<TLShapedParagraph>();
	ClassDB::register_class<TLProtoControl>();
	ClassDB::register_class<TLProtoControlSelection>();
}

void unregister_godot_tl_types() {

	TLFontFace::finish_hex_font();
	TLICUDataLoader::unload();

	//Finish ICU
	u_cleanup();
}

#else

extern "C" void GDN_EXPORT gdtl_gdnative_init(godot_gdnative_init_options *p_options) {

	//Init GDNative
	Godot::gdnative_init(p_options);
}

extern "C" void GDN_EXPORT gdtl_gdnative_terminate(godot_gdnative_terminate_options *p_options) {

	TLFontFace::finish_hex_font();
	TLICUDataLoader::unload();

	//Finish ICU
	u_cleanup();

	//Finish GDNative
	Godot::gdnative_terminate(p_options);
}

extern "C" void GDN_EXPORT gdtl_nativescript_init(void *p_handle) {

	Godot::nativescript_init(p_handle);

	TLFontFace::initialize_hex_font();

	register_class<TLICUDataLoader>();
	register_class<TLFontFace>();
	register_class<TLBitmapFontFace>();
	register_class<TLDynamicFontFace>();
	register_class<TLFontFamily>();
	register_class<TLShapedString>();
	register_class<TLShapedAttributedString>();
	register_class<TLShapedParagraph>();
	register_class<TLProtoControl>();
	register_class<TLProtoControlSelection>();

	register_tool_class<TLICUDataLoader>();
	register_tool_class<TLFontFace>();
	register_tool_class<TLBitmapFontFace>();
	register_tool_class<TLDynamicFontFace>();
	register_tool_class<TLFontFamily>();
	register_tool_class<TLShapedString>();
	register_tool_class<TLShapedAttributedString>();
	register_tool_class<TLShapedParagraph>();
	register_tool_class<TLProtoControl>();
	register_tool_class<TLProtoControlSelection>();
}

#endif
