/*************************************************************************/
/*  tl_label.cpp                                                         */
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

#include "controls/tl_label.hpp"

#ifdef GODOT_MODULE
#include "core/translation.h"
#else
#include <StyleBox.hpp>
#include <Theme.hpp>
#include <TranslationServer.hpp>
#include <VisualServer.hpp>
#endif

float TLLabel::_get_base_font_height() const {

	if (s_paragraph->get_base_font().is_valid())
		if (s_paragraph->get_base_font()->get_face(s_paragraph->get_base_font_style()).is_valid())
			if (s_paragraph->get_base_font()->get_face(s_paragraph->get_base_font_style()).value().is_valid())
				return s_paragraph->get_base_font()->get_face(s_paragraph->get_base_font_style()).value()->get_height(s_paragraph->get_base_font_size());
	return 5.0;
}

void TLLabel::set_autowrap(bool p_autowrap) {

	if (autowrap != p_autowrap) {
		autowrap = p_autowrap;
		_lines_dirty = true;
	}
	update();
}

bool TLLabel::has_autowrap() const {

	return autowrap;
}

void TLLabel::set_uppercase(bool p_uppercase) {

	if (uppercase != p_uppercase) {
		uppercase = p_uppercase;
		s_paragraph->set_text((uppercase) ? xl_text.to_upper() : xl_text);
		_lines_dirty = true;
	}
	update();
}

bool TLLabel::is_uppercase() const {

	return uppercase;
}

int TLLabel::get_line_height() const {

	float total_h = 0.0;
	for (int i = 0; i < s_lines.size(); i++) {
		total_h += s_lines[i]->get_height();
	}

	return MAX(_get_base_font_height(), total_h / s_lines.size());
}

void TLLabel::_notification(int p_what) {

	if (p_what == NOTIFICATION_TRANSLATION_CHANGED) {

		String new_text = tr(text);
		if (new_text == xl_text)
			return; //nothing new
		xl_text = new_text;

		s_paragraph->set_text(xl_text);
		_reshape_lines();

		update();
	}

	if (p_what == NOTIFICATION_DRAW) {

		if (clip) {
			VisualServer::get_singleton()->canvas_item_set_clip(get_canvas_item(), true);
		}

		if (_lines_dirty)
			_reshape_lines();

		RID ci = get_canvas_item();

		Size2 string_size;
		Size2 size = get_size();
#ifdef GODOT_MODULE
		Ref<StyleBox> style = get_stylebox("normal", "Label");
		Color font_color = get_color("font_color", "Label");
		Color font_color_shadow = get_color("font_color_shadow", "Label");
		bool use_outline = get_constant("shadow_as_outline", "Label");
		Point2 shadow_ofs(get_constant("shadow_offset_x", "Label"), get_constant("shadow_offset_y", "Label"));
		int line_spacing = get_constant("line_spacing", "Label");
#else
		Ref<Theme> theme = get_theme();
		if (theme.is_null()) {
			theme.instance();
			theme->copy_default_theme();
			set_theme(theme);
		}
		Ref<StyleBox> style = theme->get_stylebox("normal", "Label");
		Color font_color = theme->get_color("font_color", "Label");
		Color font_color_shadow = theme->get_color("font_color_shadow", "Label");
		bool use_outline = theme->get_constant("shadow_as_outline", "Label");
		Point2 shadow_ofs(theme->get_constant("shadow_offset_x", "Label"), theme->get_constant("shadow_offset_y", "Label"));
		int line_spacing = theme->get_constant("line_spacing", "Label");
#endif
		style->draw(ci, Rect2(Point2(0, 0), get_size()));

		int vbegin = 0, vsep = 0;

		float total_h = 0.0;
		int lines_visible = 0;
		for (int i = lines_skipped; i < s_lines.size(); i++) {
			total_h += s_lines[i]->get_height() + line_spacing;
			if (total_h > (get_size().height - style->get_minimum_size().height + line_spacing)) {
				break;
			}
			lines_visible++;
		}

		if (lines_visible > s_lines.size())
			lines_visible = s_lines.size();

		if (max_lines_visible >= 0 && lines_visible > max_lines_visible)
			lines_visible = max_lines_visible;

		if (lines_visible > 0) {

			switch (valign) {

				case VALIGN_TOP: {
					//nothing
				} break;
				case VALIGN_CENTER: {
					vbegin = (size.y - (total_h - line_spacing)) / 2;
					vsep = 0;

				} break;
				case VALIGN_BOTTOM: {
					vbegin = size.y - (total_h - line_spacing);
					vsep = 0;

				} break;
				case VALIGN_FILL: {
					vbegin = 0;
					if (lines_visible > 1) {
						vsep = (size.y - (total_h - line_spacing)) / (lines_visible - 1);
					} else {
						vsep = 0;
					}

				} break;
			}
		}

		Vector2 ofs;
		ofs.y = style->get_offset().y + vbegin;
		for (int j = lines_skipped; j < s_lines.size(); j++) {
			ofs.y += s_lines[j]->get_ascent();
			switch (align) {
				case ALIGN_FILL:
				case ALIGN_LEFT: {
					ofs.x = style->get_offset().x;
				} break;
				case ALIGN_CENTER: {

					ofs.x = int(size.width - s_lines[j]->get_width()) / 2;
				} break;
				case ALIGN_RIGHT: {
					ofs.x = int(size.width - style->get_margin(GLOBAL_CONST(MARGIN_RIGHT)) - s_lines[j]->get_width());
				} break;
			}
			if (font_color_shadow.a > 0) {
				s_lines[j]->draw(ci, ofs + shadow_ofs, font_color_shadow);
				if (use_outline) {
					//draw shadow
					s_lines[j]->draw(ci, ofs + Vector2(-shadow_ofs.x, shadow_ofs.y), font_color_shadow);
					s_lines[j]->draw(ci, ofs + Vector2(shadow_ofs.x, -shadow_ofs.y), font_color_shadow);
					s_lines[j]->draw(ci, ofs + Vector2(-shadow_ofs.x, -shadow_ofs.y), font_color_shadow);
				}
			}

			s_lines[j]->draw(ci, ofs, font_color);

			ofs.y += s_lines[j]->get_descent() + vsep + line_spacing;
		}
	}
	if (p_what == NOTIFICATION_THEME_CHANGED) {

		_lines_dirty = true;
		update();
	}
	if (p_what == NOTIFICATION_RESIZED) {

		_lines_dirty = true;
	}
}

Size2 TLLabel::get_minimum_size() const {
#ifdef GODOT_MODULE
	Size2 min_style = get_stylebox("normal", "Label")->get_minimum_size();
#else
	Ref<Theme> theme = get_theme();
	if (theme.is_null()) {
		theme.instance();
		theme->copy_default_theme();
		const_cast<TLLabel *>(this)->set_theme(theme);
	}
	Size2 min_style = theme->get_stylebox("normal", "Label")->get_minimum_size();
#endif
	// don't want to mutable everything
	if (_lines_dirty)
		const_cast<TLLabel *>(this)->_reshape_lines();

	if (autowrap)
		return Size2(1, clip ? 1 : minsize.height) + min_style;
	else {
		Size2 ms = minsize;
		if (clip)
			ms.width = 1;
		return ms + min_style;
	}
}

int TLLabel::get_line_count() const {

	if (!is_inside_tree())
		return 1;

	if (_lines_dirty)
		const_cast<TLLabel *>(this)->_reshape_lines();

	return s_lines.size();
}

int TLLabel::get_visible_line_count() const {

	int line_spacing = get_constant("line_spacing", "Label");

	if (_lines_dirty)
		const_cast<TLLabel *>(this)->_reshape_lines();

	float total_h = 0.0;
	int lines_visible = 0;
	for (int i = lines_skipped; i < s_lines.size(); i++) {
		total_h += s_lines[i]->get_height() + line_spacing;
#ifdef GODOT_MODULE
		if (total_h > (get_size().height - get_stylebox("normal", "Label")->get_minimum_size().height + line_spacing)) {
			break;
		}
#else
		Ref<Theme> theme = get_theme();
		if (theme.is_null()) {
			theme.instance();
			theme->copy_default_theme();
			const_cast<TLLabel *>(this)->set_theme(theme);
		}
		if (total_h > (get_size().height - theme->get_stylebox("normal", "Label")->get_minimum_size().height + line_spacing)) {
			break;
		}
#endif
		lines_visible++;
	}

	if (lines_visible > s_lines.size())
		lines_visible = s_lines.size();

	if (max_lines_visible >= 0 && lines_visible > max_lines_visible)
		lines_visible = max_lines_visible;

	return lines_visible;
}

void TLLabel::_reshape_lines() {

#ifdef GODOT_MODULE
	Ref<StyleBox> style = get_stylebox("normal", "Label");
	int line_spacing = get_constant("line_spacing", "Label");
#else
	Ref<Theme> theme = get_theme();
	if (theme.is_null()) {
		theme.instance();
		theme->copy_default_theme();
		set_theme(theme);
	}
	Ref<StyleBox> style = theme->get_stylebox("normal", "Label");
	int line_spacing = theme->get_constant("line_spacing", "Label");
#endif
	s_lines.clear();
	int width = (get_size().width - style->get_minimum_size().width);

	if (xl_text.length() == 0) {
		minsize = Size2(width, _get_base_font_height());
		return;
	}

	std::vector<int> l_lines = s_paragraph->break_lines(width, (autowrap) ? TEXT_BREAK_MANDATORY_AND_WORD_BOUND : TEXT_BREAK_MANDATORY);

	int line_start = 0;
	for (int i = 0; i < l_lines.size(); i++) {
		Ref<TLShapedString> _ln = s_paragraph->substr(line_start, l_lines[i], TEXT_TRIM_BREAK_AND_WHITESPACE);
		if (!_ln.is_null()) s_lines.push_back(_ln);
		line_start = l_lines[i];
	}

	if (!autowrap) {
		minsize.width = 0.0f;
		for (int i = 0; i < s_lines.size(); i++) {
			if (minsize.width < s_lines[i]->get_width()) {
				minsize.width = s_lines[i]->get_width();
			}
		}
	}

	if (max_lines_visible > 0 && s_lines.size() > max_lines_visible) {
		minsize.height = (_get_base_font_height() * max_lines_visible) + (line_spacing * (max_lines_visible - 1));
	} else {
		minsize.height = (_get_base_font_height() * s_lines.size()) + (line_spacing * (s_lines.size() - 1));
	}

	if (align == ALIGN_FILL) {
		for (int i = 0; i < s_lines.size(); i++) {
			s_lines[i]->extend_to_width(width, TEXT_JUSTIFICATION_KASHIDA_AND_WHITESPACE);
		}
	}

	if (!autowrap || !clip) {
		//helps speed up some TLLabels that may change a lot, as no resizing is requested. Do not change.
		minimum_size_changed();
	}

	_lines_dirty = false;
}

void TLLabel::set_text_direction(int p_text_direction) {

	ERR_FAIL_INDEX((int)p_text_direction, 4);
	if (base_direction != (TextDirection)p_text_direction) {
		base_direction = (TextDirection)p_text_direction;
		s_paragraph->set_base_direction(base_direction);
		_lines_dirty = true;
		update();
	}
}

int TLLabel::get_text_direction() const {

	return base_direction;
}

void TLLabel::set_ot_features(const String p_features) {

	if (ot_features != p_features) {
		ot_features = p_features;
		s_paragraph->set_features(ot_features);
		_lines_dirty = true;
		update();
	}
}

String TLLabel::get_ot_features() const {

	return ot_features;
}

void TLLabel::set_language(const String p_language) {

	if (language != p_language) {
		language = p_language;
		s_paragraph->set_language(language);
		_lines_dirty = true;
		update();
	}
}

String TLLabel::get_language() const {

	return language;
}

Ref<TLFontFamily> TLLabel::get_base_font() const {

	return s_paragraph->get_base_font();
}

void TLLabel::_font_changed() {
	_lines_dirty = true;
	update();
}

void TLLabel::set_base_font(const Ref<TLFontFamily> p_font) {

	s_paragraph->set_base_font(p_font);
	_lines_dirty = true;
	update();
}

String TLLabel::get_base_font_style() const {

	return s_paragraph->get_base_font_style();
}

void TLLabel::set_base_font_style(const String p_style) {

	s_paragraph->set_base_font_style(p_style);
	_lines_dirty = true;
	update();
}

int TLLabel::get_base_font_size() const {

	return s_paragraph->get_base_font_size();
}

void TLLabel::set_base_font_size(int p_size) {

	s_paragraph->set_base_font_size(p_size);
	_lines_dirty = true;
	update();
}

void TLLabel::set_align(int p_align) {

	ERR_FAIL_INDEX((int)p_align, 4);

	if (align != (Align)p_align) {
		align = (Align)p_align;
		_lines_dirty = true;
	}
	update();
}

int TLLabel::get_align() const {

	return align;
}

void TLLabel::set_valign(int p_align) {

	ERR_FAIL_INDEX((int)p_align, 4);
	valign = (VAlign)p_align;
	update();
}

int TLLabel::get_valign() const {

	return valign;
}

void TLLabel::set_text(const String p_string) {

	if (text == p_string)
		return;
	text = p_string;
	xl_text = tr(p_string);
	s_paragraph->set_text(xl_text);
	_lines_dirty = true;
	if (percent_visible < 1)
		visible_chars = get_total_character_count() * percent_visible;
	update();
}

void TLLabel::set_clip_text(bool p_clip) {

	clip = p_clip;
	update();
	minimum_size_changed();
}

bool TLLabel::is_clipping_text() const {

	return clip;
}

String TLLabel::get_text() const {

	return text;
}

void TLLabel::set_visible_characters(int p_amount) {

	visible_chars = p_amount;
	if (get_total_character_count() > 0) {
		percent_visible = (float)p_amount / (float)get_total_character_count();
	}
#ifdef GODOT_MODULE
	_change_notify("percent_visible");
#endif
	update();
}

int TLLabel::get_visible_characters() const {

	return visible_chars;
}

void TLLabel::set_percent_visible(float p_percent) {

	if (p_percent < 0 || p_percent >= 1) {

		visible_chars = -1;
		percent_visible = 1;

	} else {

		visible_chars = get_total_character_count() * p_percent;
		percent_visible = p_percent;
	}
#ifdef GODOT_MODULE
	_change_notify("visible_chars");
#endif
	update();
}

float TLLabel::get_percent_visible() const {

	return percent_visible;
}

void TLLabel::set_lines_skipped(int p_lines) {

	lines_skipped = p_lines;
	update();
}

int TLLabel::get_lines_skipped() const {

	return lines_skipped;
}

void TLLabel::set_max_lines_visible(int p_lines) {

	max_lines_visible = p_lines;
	update();
}

int TLLabel::get_max_lines_visible() const {

	return max_lines_visible;
}

int TLLabel::get_total_character_count() const {

	/* ??? */
	return xl_text.length();
}

#ifdef GODOT_MODULE

void TLLabel::_bind_methods() {

	ClassDB::bind_method(D_METHOD("_font_changed"), &TLLabel::_font_changed);

	ClassDB::bind_method(D_METHOD("set_align", "align"), &TLLabel::set_align);
	ClassDB::bind_method(D_METHOD("get_align"), &TLLabel::get_align);
	ClassDB::bind_method(D_METHOD("set_valign", "valign"), &TLLabel::set_valign);
	ClassDB::bind_method(D_METHOD("get_valign"), &TLLabel::get_valign);
	ClassDB::bind_method(D_METHOD("set_text_direction", "direction"), &TLLabel::set_text_direction);
	ClassDB::bind_method(D_METHOD("get_text_direction"), &TLLabel::get_text_direction);
	ClassDB::bind_method(D_METHOD("set_ot_features", "features"), &TLLabel::set_ot_features);
	ClassDB::bind_method(D_METHOD("get_ot_features"), &TLLabel::get_ot_features);
	ClassDB::bind_method(D_METHOD("set_language", "language"), &TLLabel::set_language);
	ClassDB::bind_method(D_METHOD("get_language"), &TLLabel::get_language);

	ClassDB::bind_method(D_METHOD("set_base_font", "ref"), &TLLabel::set_base_font);
	ClassDB::bind_method(D_METHOD("get_base_font"), &TLLabel::get_base_font);

	ClassDB::bind_method(D_METHOD("set_base_font_style", "style"), &TLLabel::set_base_font_style);
	ClassDB::bind_method(D_METHOD("get_base_font_style"), &TLLabel::get_base_font_style);

	ClassDB::bind_method(D_METHOD("set_base_font_size", "size"), &TLLabel::set_base_font_size);
	ClassDB::bind_method(D_METHOD("get_base_font_size"), &TLLabel::get_base_font_size);

	ClassDB::bind_method(D_METHOD("set_text", "text"), &TLLabel::set_text);
	ClassDB::bind_method(D_METHOD("get_text"), &TLLabel::get_text);
	ClassDB::bind_method(D_METHOD("set_autowrap", "enable"), &TLLabel::set_autowrap);
	ClassDB::bind_method(D_METHOD("has_autowrap"), &TLLabel::has_autowrap);
	ClassDB::bind_method(D_METHOD("set_clip_text", "enable"), &TLLabel::set_clip_text);
	ClassDB::bind_method(D_METHOD("is_clipping_text"), &TLLabel::is_clipping_text);
	ClassDB::bind_method(D_METHOD("set_uppercase", "enable"), &TLLabel::set_uppercase);
	ClassDB::bind_method(D_METHOD("is_uppercase"), &TLLabel::is_uppercase);
	ClassDB::bind_method(D_METHOD("get_line_height"), &TLLabel::get_line_height);
	ClassDB::bind_method(D_METHOD("get_line_count"), &TLLabel::get_line_count);
	ClassDB::bind_method(D_METHOD("get_visible_line_count"), &TLLabel::get_visible_line_count);
	ClassDB::bind_method(D_METHOD("get_total_character_count"), &TLLabel::get_total_character_count);
	ClassDB::bind_method(D_METHOD("set_visible_characters", "amount"), &TLLabel::set_visible_characters);
	ClassDB::bind_method(D_METHOD("get_visible_characters"), &TLLabel::get_visible_characters);
	ClassDB::bind_method(D_METHOD("set_percent_visible", "percent_visible"), &TLLabel::set_percent_visible);
	ClassDB::bind_method(D_METHOD("get_percent_visible"), &TLLabel::get_percent_visible);
	ClassDB::bind_method(D_METHOD("set_lines_skipped", "lines_skipped"), &TLLabel::set_lines_skipped);
	ClassDB::bind_method(D_METHOD("get_lines_skipped"), &TLLabel::get_lines_skipped);
	ClassDB::bind_method(D_METHOD("set_max_lines_visible", "lines_visible"), &TLLabel::set_max_lines_visible);
	ClassDB::bind_method(D_METHOD("get_max_lines_visible"), &TLLabel::get_max_lines_visible);

	BIND_ENUM_CONSTANT(ALIGN_LEFT);
	BIND_ENUM_CONSTANT(ALIGN_CENTER);
	BIND_ENUM_CONSTANT(ALIGN_RIGHT);
	BIND_ENUM_CONSTANT(ALIGN_FILL);

	BIND_ENUM_CONSTANT(VALIGN_TOP);
	BIND_ENUM_CONSTANT(VALIGN_CENTER);
	BIND_ENUM_CONSTANT(VALIGN_BOTTOM);
	BIND_ENUM_CONSTANT(VALIGN_FILL);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "text", PROPERTY_HINT_MULTILINE_TEXT, "", PROPERTY_USAGE_DEFAULT_INTL), "set_text", "get_text");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "align", PROPERTY_HINT_ENUM, "Left,Center,Right,Fill"), "set_align", "get_align");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "valign", PROPERTY_HINT_ENUM, "Top,Center,Bottom,Fill"), "set_valign", "get_valign");

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "base_font", PROPERTY_HINT_RESOURCE_TYPE, "TLFontFamily"), "set_base_font", "get_base_font");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "base_font_size"), "set_base_font_size", "get_base_font_size");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "base_font_style"), "set_base_font_style", "get_base_font_style");

	ADD_PROPERTY(PropertyInfo(Variant::INT, "text_direction", PROPERTY_HINT_ENUM, "LTR,RTL,Locale,Auto"), "set_text_direction", "get_text_direction");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "ot_features"), "set_ot_features", "get_ot_features");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "language"), "set_language", "get_language");

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autowrap"), "set_autowrap", "has_autowrap");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "clip_text"), "set_clip_text", "is_clipping_text");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "uppercase"), "set_uppercase", "is_uppercase");
}

#else

void TLLabel::_register_methods() {

	register_method("_font_changed", &TLLabel::_font_changed);

	register_method("set_align", &TLLabel::set_align);
	register_method("get_align", &TLLabel::get_align);
	register_method("set_valign", &TLLabel::set_valign);
	register_method("get_valign", &TLLabel::get_valign);
	register_method("set_text_direction", &TLLabel::set_text_direction);
	register_method("get_text_direction", &TLLabel::get_text_direction);
	register_method("set_ot_features", &TLLabel::set_ot_features);
	register_method("get_ot_features", &TLLabel::get_ot_features);
	register_method("set_language", &TLLabel::set_language);
	register_method("get_language", &TLLabel::get_language);

	register_method("set_base_font", &TLLabel::set_base_font);
	register_method("get_base_font", &TLLabel::get_base_font);

	register_method("set_base_font_style", &TLLabel::set_base_font_style);
	register_method("get_base_font_style", &TLLabel::get_base_font_style);

	register_method("set_base_font_size", &TLLabel::set_base_font_size);
	register_method("get_base_font_size", &TLLabel::get_base_font_size);

	register_method("set_text", &TLLabel::set_text);
	register_method("get_text", &TLLabel::get_text);
	register_method("set_autowrap", &TLLabel::set_autowrap);
	register_method("has_autowrap", &TLLabel::has_autowrap);
	register_method("set_clip_text", &TLLabel::set_clip_text);
	register_method("is_clipping_text", &TLLabel::is_clipping_text);
	register_method("set_uppercase", &TLLabel::set_uppercase);
	register_method("is_uppercase", &TLLabel::is_uppercase);
	register_method("get_line_height", &TLLabel::get_line_height);
	register_method("get_line_count", &TLLabel::get_line_count);
	register_method("get_visible_line_count", &TLLabel::get_visible_line_count);
	register_method("get_total_character_count", &TLLabel::get_total_character_count);
	register_method("set_visible_characters", &TLLabel::set_visible_characters);
	register_method("get_visible_characters", &TLLabel::get_visible_characters);
	register_method("set_percent_visible", &TLLabel::set_percent_visible);
	register_method("get_percent_visible", &TLLabel::get_percent_visible);
	register_method("set_lines_skipped", &TLLabel::set_lines_skipped);
	register_method("get_lines_skipped", &TLLabel::get_lines_skipped);
	register_method("set_max_lines_visible", &TLLabel::set_max_lines_visible);
	register_method("get_max_lines_visible", &TLLabel::get_max_lines_visible);

	register_property<TLLabel, String>("text", &TLLabel::set_text, &TLLabel::get_text, String(), GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_MULTILINE_TEXT, String());
	register_property<TLLabel, int>("align", &TLLabel::set_align, &TLLabel::get_align, ALIGN_LEFT, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM, String("Left,Center,Right,Fill"));
	register_property<TLLabel, int>("valign", &TLLabel::set_valign, &TLLabel::get_valign, VALIGN_TOP, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM, String("Top,Center,Bottom,Fill"));

	register_property<TLLabel, Ref<TLFontFamily> >("base_font", &TLLabel::set_base_font, &TLLabel::get_base_font, Ref<TLFontFamily>(), GODOT_METHOD_RPC_MODE_DISABLED, (godot_property_usage_flags)(GODOT_PROPERTY_USAGE_NOEDITOR | GODOT_PROPERTY_USAGE_STORAGE), GODOT_PROPERTY_HINT_RESOURCE_TYPE, String("TLFontFamily"));
	register_property<TLLabel, String>("base_font_style", &TLLabel::set_base_font_style, &TLLabel::get_base_font_style, String("Regular"));
	register_property<TLLabel, int>("base_font_size", &TLLabel::set_base_font_size, &TLLabel::get_base_font_size, 12);

	register_property<TLLabel, int>("text_direction", &TLLabel::set_text_direction, &TLLabel::get_text_direction, TEXT_DIRECTION_AUTO, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM, String("LTR,RTL,Locale,Auto"));
	register_property<TLLabel, String>("ot_features", &TLLabel::set_ot_features, &TLLabel::get_ot_features, String());
	register_property<TLLabel, String>("language", &TLLabel::set_language, &TLLabel::get_language, String());

	register_property<TLLabel, bool>("autowrap", &TLLabel::set_autowrap, &TLLabel::has_autowrap, false);
	register_property<TLLabel, bool>("clip_text", &TLLabel::set_clip_text, &TLLabel::is_clipping_text, false);
	register_property<TLLabel, bool>("uppercase", &TLLabel::set_uppercase, &TLLabel::is_uppercase, false);

	register_method("_notification", &TLLabel::_notification);
}

#endif

TLLabel::TLLabel() {
#ifdef GODOT_MODULE
	_init();
#endif
}

void TLLabel::_init() {

	s_paragraph.instance();
	s_paragraph->connect("string_changed", this, "_font_changed");

	base_direction = TEXT_DIRECTION_AUTO;
	ot_features = "";
	language = "";

	align = ALIGN_LEFT;
	valign = VALIGN_TOP;
	xl_text = "";

	_lines_dirty = true;

	autowrap = false;

	set_v_size_flags(0);
	clip = false;
	set_mouse_filter(MOUSE_FILTER_IGNORE);

	visible_chars = -1;
	percent_visible = 1;
	lines_skipped = 0;
	max_lines_visible = -1;
	uppercase = false;
	set_v_size_flags(SIZE_SHRINK_CENTER);
}
