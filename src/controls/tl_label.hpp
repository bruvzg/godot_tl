/*************************************************************************/
/*  tl_label.hpp                                                         */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2019 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2019 Godot Engine contributors (cf. AUTHORS.md)    */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef TL_LABEL_H
#define TL_LABEL_H

#include "resources/tl_font_family.hpp"
#include "resources/tl_shaped_string.hpp"

#ifdef GODOT_MODULE
#include "scene/gui/control.h"
#else
#include <Control.hpp>
#include <InputEvent.hpp>
#endif

using namespace godot;

/**
	@author Juan Linietsky <reduzio@gmail.com>
*/
class TLLabel : public Control {

	GODOT_CLASS(TLLabel, Control);

public:
	enum Align {

		ALIGN_LEFT,
		ALIGN_CENTER,
		ALIGN_RIGHT,
		ALIGN_FILL
	};

	enum VAlign {

		VALIGN_TOP,
		VALIGN_CENTER,
		VALIGN_BOTTOM,
		VALIGN_FILL
	};

private:
	Align align;
	VAlign valign;
	String text;
	String xl_text;
	bool autowrap;
	bool clip;
	Size2 minsize;
	bool uppercase;

	TextDirection base_direction;
	String ot_features;
	String language;

	Ref<TLShapedString> s_paragraph;
	std::vector<Ref<TLShapedString> > s_lines;

	void _reshape_lines();
	bool _lines_dirty;

	float percent_visible;

	int visible_chars;
	int lines_skipped;
	int max_lines_visible;

	float _get_base_font_height() const;

public:
	void _notification(int p_what);

#ifdef GODOT_MODULE
	static void _bind_methods();
#else
	static void _register_methods();
#endif
	void _init();

	virtual Size2 get_minimum_size() const;

	void set_align(int p_align);
	int get_align() const;

	void set_valign(int p_align);
	int get_valign() const;

	void set_text_direction(int p_text_direction);
	int get_text_direction() const;

	void set_ot_features(const String p_features);
	String get_ot_features() const;

	void set_language(const String p_language);
	String get_language() const;

	void set_text(const String p_string);
	String get_text() const;

	void set_autowrap(bool p_autowrap);
	bool has_autowrap() const;

	void set_uppercase(bool p_uppercase);
	bool is_uppercase() const;

	void set_visible_characters(int p_amount);
	int get_visible_characters() const;
	int get_total_character_count() const;

	void set_clip_text(bool p_clip);
	bool is_clipping_text() const;

	void set_percent_visible(float p_percent);
	float get_percent_visible() const;

	void set_lines_skipped(int p_lines);
	int get_lines_skipped() const;

	void set_max_lines_visible(int p_lines);
	int get_max_lines_visible() const;

	int get_line_height() const;
	int get_line_count() const;
	int get_visible_line_count() const;

	Ref<TLFontFamily> get_base_font() const;
	void set_base_font(const Ref<TLFontFamily> p_font);

	String get_base_font_style() const;
	void set_base_font_style(const String p_style);

	int get_base_font_size() const;
	void set_base_font_size(int p_size);

	TLLabel();

	virtual ~TLLabel(){};
};

#ifdef GODOT_MODULE
VARIANT_ENUM_CAST(TLLabel::Align);
VARIANT_ENUM_CAST(TLLabel::VAlign);
#endif

#endif
