/*************************************************************************/
/*  tl_gd_font_wrapper.cpp                                               */
/*************************************************************************/

#include "tl_gd_font_wrapper.hpp"

#ifdef GODOT_MODULE
void TLGDFontWrapper::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_base_font", "ref"), &TLGDFontWrapper::set_base_font);
	ClassDB::bind_method(D_METHOD("get_base_font"), &TLGDFontWrapper::get_base_font);

	ClassDB::bind_method(D_METHOD("set_base_font_style", "style"), &TLGDFontWrapper::set_base_font_style);
	ClassDB::bind_method(D_METHOD("get_base_font_style"), &TLGDFontWrapper::get_base_font_style);

	ClassDB::bind_method(D_METHOD("set_base_font_size", "size"), &TLGDFontWrapper::set_base_font_size);
	ClassDB::bind_method(D_METHOD("get_base_font_size"), &TLGDFontWrapper::get_base_font_size);

	ClassDB::bind_method(D_METHOD("set_cache_depth", "cache_depth"), &TLGDFontWrapper::set_cache_depth);
	ClassDB::bind_method(D_METHOD("get_cache_depth"), &TLGDFontWrapper::get_cache_depth);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "base_font", PROPERTY_HINT_RESOURCE_TYPE, "TLFontFamily"), "set_base_font", "get_base_font");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "base_font_size"), "set_base_font_size", "get_base_font_size");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "base_font_style"), "set_base_font_style", "get_base_font_style");

	ADD_PROPERTY(PropertyInfo(Variant::INT, "cache_depth"), "set_cache_depth", "get_cache_depth");
}
#else
void TLGDFontWrapper::_register_methods() {
	register_method("set_base_font", &TLGDFontWrapper::set_base_font);
	register_method("get_base_font", &TLGDFontWrapper::get_base_font);

	register_method("set_base_font_style", &TLGDFontWrapper::set_base_font_style);
	register_method("get_base_font_style", &TLGDFontWrapper::get_base_font_style);

	register_method("set_base_font_size", &TLGDFontWrapper::set_base_font_size);
	register_method("get_base_font_size", &TLGDFontWrapper::get_base_font_size);

	register_method("set_cache_depth", &TLGDFontWrapper::set_cache_depth);
	register_method("get_cache_depth", &TLGDFontWrapper::get_cache_depth);

	register_property<TLGDFontWrapper, Ref<TLFontFamily>>("base_font", &TLGDFontWrapper::set_base_font, &TLGDFontWrapper::get_base_font, Ref<TLFontFamily>(), GODOT_METHOD_RPC_MODE_DISABLED, (godot_property_usage_flags)(GODOT_PROPERTY_USAGE_NOEDITOR | GODOT_PROPERTY_USAGE_STORAGE), GODOT_PROPERTY_HINT_RESOURCE_TYPE, String("TLFontFamily"));
	register_property<TLGDFontWrapper, String>("base_font_style", &TLGDFontWrapper::set_base_font_style, &TLGDFontWrapper::get_base_font_style, String("Regular"));
	register_property<TLGDFontWrapper, int>("base_font_size", &TLGDFontWrapper::set_base_font_size, &TLGDFontWrapper::get_base_font_size, 12);

	register_property<TLGDFontWrapper, int>("cache_depth", &TLGDFontWrapper::set_cache_depth, &TLGDFontWrapper::get_cache_depth, 100);
}
#endif

Ref<TLFontFamily> TLGDFontWrapper::get_base_font() const {
	return family;
}

void TLGDFontWrapper::set_base_font(const Ref<TLFontFamily> p_font) {
	if (family != p_font) {
		family = p_font;
		sstr.clear();
		sids.clear();
	}
}

String TLGDFontWrapper::get_base_font_style() const {
	return fstyle;
}

void TLGDFontWrapper::set_base_font_style(const String p_style) {
	if (fstyle != p_style) {
		fstyle = p_style;
		sstr.clear();
		sids.clear();
	}
}

int TLGDFontWrapper::get_base_font_size() const {
	return fsize;
}

void TLGDFontWrapper::set_base_font_size(int p_size) {
	if (fsize != p_size) {
		fsize = p_size;
		sstr.clear();
		sids.clear();
	}
}

int TLGDFontWrapper::get_cache_depth() const {
	return scdepth;
}

void TLGDFontWrapper::set_cache_depth(int p_cache_depth) {
	if (scdepth != p_cache_depth) {
		scdepth = p_cache_depth;
		while (sids.size() > (size_t)scdepth) {
			sstr.erase(sstr.find(sids.front()));
			sids.pop_front();
		}
	}
}

float TLGDFontWrapper::get_height() const {
	if (family.is_valid()) {
		TLFontFallbackIterator font_iter = family->get_face(fstyle);
		if (font_iter.is_valid()) {
			Ref<TLFontFace> _font = font_iter.value();
			if (_font.is_null()) {
				return 0.0f;
			}
			return _font->get_height(fsize);
		}
	}
	return 15.0f;
}

float TLGDFontWrapper::get_ascent() const {
	if (family.is_valid()) {
		TLFontFallbackIterator font_iter = family->get_face(fstyle);
		if (font_iter.is_valid()) {
			Ref<TLFontFace> _font = font_iter.value();
			if (_font.is_null()) {
				return 0.0f;
			}
			return _font->get_ascent(fsize);
		}
	}
	return 10.0f;
}

float TLGDFontWrapper::get_descent() const {
	if (family.is_valid()) {
		TLFontFallbackIterator font_iter = family->get_face(fstyle);
		if (font_iter.is_valid()) {
			Ref<TLFontFace> _font = font_iter.value();
			if (_font.is_null()) {
				return 0.0f;
			}
			return _font->get_ascent(fsize);
		}
	}
	return 5.0f;
}

Size2 TLGDFontWrapper::get_char_size(CharType p_char, CharType p_next) const {
	CharType ucodestr[2] = { (CharType)p_char, 0 };
	return get_string_size(ucodestr);
}

Size2 TLGDFontWrapper::get_string_size(const String &p_text) const {
	Ref<TLShapedString> str;
	int64_t hash = p_text.hash();
	if (sstr.count(hash) > 0) {
		str = sstr[hash];
	} else {
#ifdef GODOT_MODULE
		str.instance();
#else
		str = Ref<TLShapedString>::__internal_constructor(TLShapedString::_new());
#endif
		str->set_base_font(family);
		str->set_base_font_style(fstyle);
		str->set_base_font_size(fsize);
		str->set_text(p_text);
		sids.push_back(hash);
		sstr[hash] = str;
		while (sids.size() > (size_t)scdepth) {
			sstr.erase(sstr.find(sids.front()));
			sids.pop_front();
		}
	}
	return Size2(str->get_width(), MAX(str->get_height(), get_height()));
}

void TLGDFontWrapper::draw(RID p_canvas_item, const Point2 &p_pos, const String &p_text, const Color &p_modulate, int p_clip_w, const Color &p_outline_modulate) const {
	Ref<TLShapedString> str;
	int64_t hash = p_text.hash();
	if (sstr.count(hash) > 0) {
		str = sstr[hash];
	} else {
#ifdef GODOT_MODULE
		str.instance();
#else
		str = Ref<TLShapedString>::__internal_constructor(TLShapedString::_new());
#endif
		str->set_base_font(family);
		str->set_base_font_style(fstyle);
		str->set_base_font_size(fsize);
		str->set_text(p_text);
		sids.push_back(hash);
		sstr[hash] = str;
		while (sids.size() > (size_t)scdepth) {
			sstr.erase(sstr.find(sids.front()));
			sids.pop_front();
		}
	}
	float w = 0.f;
	for (int i = 0; i < str->clusters(); i++) {
		if (p_clip_w <= 0.f || (w + str->get_cluster_width(i)) <= p_clip_w) {
			w += str->draw_cluster(p_canvas_item, Point2(p_pos.x + w, p_pos.y), i, p_modulate).x;
		}
	}
}

float TLGDFontWrapper::draw_char(RID p_canvas_item, const Point2 &p_pos, CharType p_char, CharType p_next, const Color &p_modulate, bool p_outline) const {
	CharType ucodestr[2] = { (CharType)p_char, 0 };
	draw(p_canvas_item, p_pos, ucodestr, p_modulate);
	return get_string_size(ucodestr).width;
}

bool TLGDFontWrapper::is_distance_field_hint() const {
	return false;
}

void TLGDFontWrapper::_init() {
	fstyle = "Regular";
	fsize = 12;
	scdepth = 100;
}

TLGDFontWrapper::TLGDFontWrapper() {
#ifdef GODOT_MODULE
	_init();
#endif
}

TLGDFontWrapper::~TLGDFontWrapper() {}
