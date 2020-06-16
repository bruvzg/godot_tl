/*************************************************************************/
/*  tl_gd_font_wrapper.hpp                                               */
/*************************************************************************/

#ifndef TL_GD_FONT_WRAPPER_HPP
#define TL_GD_FONT_WRAPPER_HPP

#ifdef GODOT_MODULE
#include "scene/resources/font.h"
#else
#include <Font.hpp>
#endif

#include <list>
#include <map>

#include "resources/tl_font_family.hpp"
#include "resources/tl_shaped_string.hpp"

using namespace godot;

class TLGDFontWrapper : public Font {
	GODOT_CLASS(TLGDFontWrapper, Font);

	mutable std::map<int64_t, Ref<TLShapedString>> sstr;
	mutable std::list<int64_t> sids;

	int scdepth;

	Ref<TLFontFamily> family;
	String fstyle;
	int fsize;

public:
#ifdef GODOT_MODULE
	static void _bind_methods();
#else
	static void _register_methods();
#endif
	void _init();

	Ref<TLFontFamily> get_base_font() const;
	void set_base_font(const Ref<TLFontFamily> p_font);

	String get_base_font_style() const;
	void set_base_font_style(const String p_style);

	int get_base_font_size() const;
	void set_base_font_size(int p_size);

	int get_cache_depth() const;
	void set_cache_depth(int p_cache_depth);

	virtual float get_height() const;

	virtual float get_ascent() const;
	virtual float get_descent() const;
	virtual float get_underline_position() const { return 2; };
	virtual float get_underline_thickness() const { return 1; };

	virtual Size2 get_char_size(CharType p_char, CharType p_next = 0) const;
	virtual Size2 get_string_size(const String &p_text) const;

	virtual bool is_distance_field_hint() const;

	virtual void draw(RID p_canvas_item, const Point2 &p_pos, const String &p_text, const Color &p_modulate = Color(1, 1, 1), int p_clip_w = -1, const Color &p_outline_modulate = Color(1, 1, 1)) const;

	virtual float draw_char(RID p_canvas_item, const Point2 &p_pos, CharType p_char, CharType p_next = 0, const Color &p_modulate = Color(1, 1, 1), bool p_outline = false) const;

	TLGDFontWrapper();
	virtual ~TLGDFontWrapper();
};

#endif /*TL_GD_FONT_WRAPPER_HPP*/
