/*************************************************************************/
/*  tl_font_family.hpp                                                   */
/*************************************************************************/

#ifndef TL_FONT_FAMILY_HPP
#define TL_FONT_FAMILY_HPP

#include "tl_font.hpp"

#include <cmath>
#include <map>

#include <hb.h>

using namespace godot;

class TLFontFamily : public Resource {
	GODOT_CLASS(TLFontFamily, Resource);

protected:
	struct StyleData {
		Ref<TLFontFace> main;
		std::map<hb_script_t, Ref<TLFontFace> > linked;
	};

	std::map<String, StyleData> styles;

public:
	TLFontFamily();
	~TLFontFamily();

	void _init();

	//Internal
	Ref<TLFontFace> _get_liked_face_for_script(String p_style, hb_script_t p_script) const;

	//GDNative
	void remove_style(String p_style);
	bool has_style(String p_style) const;

	void set_face(String p_style, Ref<TLFontFace> p_ref);
	Ref<TLFontFace> get_face(String p_style) const;

	void set_liked_face_for_script(String p_style, String p_script, Ref<TLFontFace> p_ref);
	Ref<TLFontFace> get_liked_face_for_script(String p_style, String p_script) const;

#ifdef GODOT_MODULE
	static void _bind_methods();
#else
	static void _register_methods();
#endif
};

//TODO add EditorInspectorPlugin fro styles

#endif /*TL_FONT_FAMILY_HPP*/
