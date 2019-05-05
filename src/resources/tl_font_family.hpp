/*************************************************************************/
/*  tl_font_family.hpp                                                   */
/*************************************************************************/

#ifndef TL_FONT_FAMILY_HPP
#define TL_FONT_FAMILY_HPP

#include "tl_font.hpp"

#include <cmath>
#include <map>
#include <vector>

#include <hb.h>

using namespace godot;

struct StyleData {
	//Ref<TLFontFace> main;
	//std::map<hb_script_t, Ref<TLFontFace> > linked;

	std::vector<Ref<TLFontFace> > main_chain;
	std::map<hb_script_t, std::vector<Ref<TLFontFace> > > linked_src_chain;
	std::map<hb_language_t, std::vector<Ref<TLFontFace> > > linked_lang_chain;
};

class TLFontFallbackIterator {

protected:
	enum ChainID {
		LANG_CHAIN,
		SCRIPT_CHAIN,
		MAIN_CHAIN,
		SINGLE_FONT,
		INVALID_CHAIN
	};

	Ref<TLFontFace> font;
	const StyleData *data;

	hb_language_t lang;
	hb_script_t script;
	ChainID chain_id;

	int64_t index;

public:
	Ref<TLFontFace> value();

	bool is_valid();
	bool is_linked();
	TLFontFallbackIterator next();

	TLFontFallbackIterator() {
		data = NULL;

		script = HB_SCRIPT_INVALID;
		lang = HB_LANGUAGE_INVALID;
		chain_id = INVALID_CHAIN;
		index = -1;
	}
	TLFontFallbackIterator(Ref<TLFontFace> p_font);
	TLFontFallbackIterator(const StyleData *p_data);
	TLFontFallbackIterator(const StyleData *p_data, hb_script_t p_script);
	TLFontFallbackIterator(const StyleData *p_data, hb_language_t p_lang);
};

class TLFontFamily : public Resource {
	GODOT_CLASS(TLFontFamily, Resource);

protected:
	std::map<String, StyleData> styles;

public:
	TLFontFamily();
	~TLFontFamily();

	void _init();

	//Ref<TLFontFace> _get_liked_face_for_script(String p_style, hb_script_t p_script) const;

	//GDNative
	void remove_style(String p_style);
	bool has_style(String p_style) const;

	TLFontFallbackIterator get_face(String p_style) const;
	TLFontFallbackIterator get_face_for_script(String p_style, hb_script_t p_script) const;
	TLFontFallbackIterator get_face_for_language(String p_style, hb_language_t p_lang) const;

	//void set_face(String p_style, Ref<TLFontFace> p_ref); //remove
	//Ref<TLFontFace> get_face(String p_style) const; //remove

	void add_face(String p_style, Ref<TLFontFace> p_ref);
	void add_face_for_script(String p_style, Ref<TLFontFace> p_ref, String p_script);
	void add_face_for_language(String p_style, Ref<TLFontFace> p_ref, String p_lang);

	//void set_liked_face_for_script(String p_style, String p_script, Ref<TLFontFace> p_ref); //remove
	//Ref<TLFontFace> get_liked_face_for_script(String p_style, String p_script) const; //remove

#ifdef GODOT_MODULE
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

	static void _bind_methods();
#else
	bool _set(String p_name, Variant p_value);
	Variant _get(String p_name) const;
	Array _get_property_list() const;

	static void _register_methods();
#endif
};

#endif /*TL_FONT_FAMILY_HPP*/
