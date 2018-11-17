/*************************************************************************/
/*  tl_font_family.cpp                                                   */
/*************************************************************************/

#include "resources/tl_font_family.hpp"

TLFontFamily::TLFontFamily() {

#ifdef GODOT_MODULE
	_init();
#endif
}

void TLFontFamily::_init() {
	//NOP
}

TLFontFamily::~TLFontFamily() {
	//NOP
}

void TLFontFamily::remove_style(String p_style) {

	styles.erase(p_style);
}

bool TLFontFamily::has_style(String p_style) const {

	return styles.count(p_style) > 0;
}

void TLFontFamily::set_face(String p_style, Ref<TLFontFace> p_ref) {

	if (p_ref.is_valid() && !cast_to<TLFontFace>(*p_ref)) {
		ERR_PRINTS("Type mismatch");
		return;
	}

	if (!p_ref.is_valid()) {
		if (styles.count(p_style) == 0)
			return;
	}

	styles[p_style].main = p_ref;
#ifdef GODOT_MODULE
	_change_notify();
#endif
}

Ref<TLFontFace> TLFontFamily::get_face(String p_style) const {

	if (styles.count(p_style) > 0)
		return styles.at(p_style).main;

	return Ref<TLFontFace>();
}

void TLFontFamily::set_liked_face_for_script(String p_style, String p_script, Ref<TLFontFace> p_ref) {

	if (p_ref.is_valid() && !cast_to<TLFontFace>(*p_ref)) {
		ERR_PRINTS("Type mismatch");
		return;
	}

	hb_script_t scr = hb_script_from_string(p_script.ascii().get_data(), -1);

	if (!p_ref.is_valid()) {
		if (styles.count(p_style) == 0)
			return;
		if (styles[p_style].linked.count(scr) == 0)
			return;
	}

	styles[p_style].linked[scr] = p_ref;
#ifdef GODOT_MODULE
	_change_notify();
#endif
}

Ref<TLFontFace> TLFontFamily::_get_liked_face_for_script(String p_style, hb_script_t p_script) const {

	if (styles.count(p_style) > 0)
		if (styles.at(p_style).linked.count(p_script) > 0)
			return styles.at(p_style).linked.at(p_script);

	return Ref<TLFontFace>();
}

Ref<TLFontFace> TLFontFamily::get_liked_face_for_script(String p_style, String p_script) const {

	hb_script_t scr = hb_script_from_string(p_script.ascii().get_data(), -1);
	if (styles.count(p_style) > 0)
		if (styles.at(p_style).linked.count(scr) > 0)
			return styles.at(p_style).linked.at(scr);

	return Ref<TLFontFace>();
}

#ifdef GODOT_MODULE

void TLFontFamily::_bind_methods() {

	ClassDB::bind_method(D_METHOD("remove_style", "style"), &TLFontFamily::remove_style);
	ClassDB::bind_method(D_METHOD("has_style", "style"), &TLFontFamily::has_style);

	ClassDB::bind_method(D_METHOD("set_face", "style", "ref"), &TLFontFamily::set_face);

	ClassDB::bind_method(D_METHOD("get_face", "style"), &TLFontFamily::get_face);

	ClassDB::bind_method(D_METHOD("set_liked_face_for_script", "style", "script", "ref"), &TLFontFamily::set_liked_face_for_script);
	ClassDB::bind_method(D_METHOD("get_liked_face_for_script", "style", "script"), &TLFontFamily::get_liked_face_for_script);
}

#else

void TLFontFamily::_register_methods() {

	register_method("remove_style", &TLFontFamily::remove_style);
	register_method("has_style", &TLFontFamily::has_style);

	register_method("set_face", &TLFontFamily::set_face);

	register_method("get_face", &TLFontFamily::get_face);

	register_method("set_liked_face_for_script", &TLFontFamily::set_liked_face_for_script);
	register_method("get_liked_face_for_script", &TLFontFamily::get_liked_face_for_script);
}

#endif
