/*************************************************************************/
/*  tl_line_edit.cpp                                                     */
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

#include "controls/tl_line_edit.hpp"
#include "controls/tl_label.hpp"

#ifdef GODOT_MODULE
#include "core/class_db.h"
#include "core/message_queue.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/translation.h"
#include "servers/visual_server.h"
#else
#include <Image.hpp>
#include <InputEventKey.hpp>
#include <InputEventMouseButton.hpp>
#include <InputEventMouseMotion.hpp>
#include <MainLoop.hpp>
#include <OS.hpp>
#include <StyleBox.hpp>
#include <Texture.hpp>
#include <Theme.hpp>
#include <TranslationServer.hpp>
#include <VisualServer.hpp>
#endif

#ifdef TOOLS_ENABLED
#include "editor/editor_scale.h"
#include "editor/editor_settings.h"
#endif

static bool _is_text_char(wchar_t c) {

	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

float TLLineEdit::_get_base_font_height() const {

	if (line->get_base_font().is_valid())
		if (line->get_base_font()->get_face(line->get_base_font_style()).is_valid())
			if (line->get_base_font()->get_face(line->get_base_font_style()).value().is_valid())
				return line->get_base_font()->get_face(line->get_base_font_style()).value()->get_height(line->get_base_font_size());
	return 5.0;
}

#ifdef GODOT_MODULE
void TLLineEdit::_gui_input(Ref<InputEvent> p_event) {
#else
void TLLineEdit::_gui_input(InputEvent *p_event) {
#endif

#ifdef GODOT_MODULE
	Ref<InputEventMouseButton> b = p_event;
	if (b.is_valid()) {
#else
	InputEventMouseButton *b = cast_to<InputEventMouseButton>(p_event);
	if (b) {
#endif

		if (b->is_pressed() && b->get_button_index() == GLOBAL_CONST(BUTTON_RIGHT) && context_menu_enabled) {
			menu->set_position(get_global_transform().xform(get_local_mouse_position()));
			menu->set_size(Vector2(1, 1));
			menu->popup();
			grab_focus();
			return;
		}

		if (b->get_button_index() != GLOBAL_CONST(BUTTON_LEFT))
			return;

		_reset_caret_blink_timer();
		if (b->is_pressed()) {

			if (!text.empty() && is_editable() && _is_over_clear_button(b->get_position())) {
				clear_button_status.press_attempt = true;
				clear_button_status.pressing_inside = true;
				return;
			}

			shift_selection_check_pre(b->get_shift());

			set_cursor_at_pixel_pos(b->get_position().x);

			if (b->get_shift()) {

				selection_fill_at_cursor();
				selection.creating = true;

			} else {

				if (b->is_doubleclick()) {

					selection.enabled = true;
					selection.begin = 0;
					selection.end = text.length();
					selection.doubleclick = true;
				}

				selection.drag_attempt = false;

				if ((cursor_pos < selection.begin) || (cursor_pos > selection.end) || !selection.enabled) {

					deselect();
					selection.cursor_start = cursor_pos;
					selection.creating = true;
				} else if (selection.enabled) {

					selection.drag_attempt = true;
				}
			}

			update();

		} else {

			if (!text.empty() && is_editable() && clear_button_enabled) {
				bool press_attempt = clear_button_status.press_attempt;
				clear_button_status.press_attempt = false;
				if (press_attempt && clear_button_status.pressing_inside && _is_over_clear_button(b->get_position())) {
					clear();
					return;
				}
			}

			if ((!selection.creating) && (!selection.doubleclick)) {
				deselect();
			}
			selection.creating = false;
			selection.doubleclick = false;

			if (OS::get_singleton()->has_virtual_keyboard())
#ifdef GODOT_MODULE
				OS::get_singleton()->show_virtual_keyboard(text, get_global_rect());
#else
				OS::get_singleton()->show_virtual_keyboard(text);
#endif
		}

		update();
	}

#ifdef GODOT_MODULE
	Ref<InputEventMouseMotion> m = p_event;
	if (m.is_valid()) {
#else
	InputEventMouseMotion *m = cast_to<InputEventMouseMotion>(p_event);
	if (m) {
#endif

		if (!text.empty() && is_editable() && clear_button_enabled) {
			bool last_press_inside = clear_button_status.pressing_inside;
			clear_button_status.pressing_inside = clear_button_status.press_attempt && _is_over_clear_button(m->get_position());
			if (last_press_inside != clear_button_status.pressing_inside) {
				update();
			}
		}

		if (m->get_button_mask() & GLOBAL_CONST(BUTTON_LEFT)) {

			if (selection.creating) {
				set_cursor_at_pixel_pos(m->get_position().x);
				selection_fill_at_cursor();
			}
		}
	}

#ifdef GODOT_MODULE
	Ref<InputEventKey> k = p_event;
	if (k.is_valid()) {
#else
	InputEventKey *k = cast_to<InputEventKey>(p_event);
	if (k) {
#endif

		if (!k->is_pressed())
			return;
		unsigned int code = k->get_scancode();

		if (k->get_command()) {

			bool handled = true;

			switch (code) {

				case GLOBAL_CONST(KEY_D): { // Swap curent input direction (primary cursor)

					last_input_direction = (last_input_direction == TEXT_DIRECTION_LTR) ? TEXT_DIRECTION_RTL : TEXT_DIRECTION_LTR;
					update();

				} break;

				case GLOBAL_CONST(KEY_X): { // CUT

					if (editable) {
						cut_text();
					}

				} break;

				case GLOBAL_CONST(KEY_C): { // COPY

					copy_text();

				} break;

				case GLOBAL_CONST(KEY_V): { // PASTE

					if (editable) {

						paste_text();
					}

				} break;

				case GLOBAL_CONST(KEY_Z): { // undo / redo
					if (editable) {
						if (k->get_shift()) {
							redo();
						} else {
							undo();
						}
					}
				} break;

				case GLOBAL_CONST(KEY_U): { // Delete from start to cursor

					if (editable) {

						deselect();
						text = text.substr(cursor_pos, text.length() - cursor_pos);

						set_cursor_position(0);
						_text_reshape();
						_text_changed();
					}

				} break;

				case GLOBAL_CONST(KEY_Y): { // PASTE (Yank for unix users)

					if (editable) {

						paste_text();
					}

				} break;
				case GLOBAL_CONST(KEY_K): { // Delete from cursor_pos to end

					if (editable) {

						deselect();
						text = text.substr(0, cursor_pos);
						_text_reshape();
						_text_changed();
					}

				} break;
				case GLOBAL_CONST(KEY_A): { //Select All
					select();
				} break;
#ifdef APPLE_STYLE_KEYS
				case GLOBAL_CONST(KEY_LEFT): { // Go to start of text - like HOME key
					set_cursor_position(0);
				} break;
				case GLOBAL_CONST(KEY_RIGHT): { // Go to end of text - like END key
					set_cursor_position(text.length());
				} break;
#endif
				default: {
					handled = false;
				}
			}

			if (handled) {
				accept_event();
				return;
			}
		}

		_reset_caret_blink_timer();
		if (!k->get_metakey()) {

			bool handled = true;
			switch (code) {

				case GLOBAL_CONST(KEY_KP_ENTER):
				case GLOBAL_CONST(KEY_ENTER): {

					emit_signal("text_entered", text);
					if (OS::get_singleton()->has_virtual_keyboard())
						OS::get_singleton()->hide_virtual_keyboard();

					return;
				} break;

				case GLOBAL_CONST(KEY_BACKSPACE): {

					if (!editable)
						break;

					if (selection.enabled) {
						selection_delete();
						break;
					}

#ifdef APPLE_STYLE_KEYS
					if (k->get_alt()) {
#else
					if (k->get_alt()) {
						handled = false;
						break;
					} else if (k->get_command()) {
#endif
						int cc = cursor_pos;
						bool prev_char = false;

						while (cc > 0) {
							bool ischar = _is_text_char(text[cc - 1]);

							if (prev_char && !ischar)
								break;

							prev_char = ischar;
							cc--;
						}

						delete_text(cc, cursor_pos);

						set_cursor_position(cc);

					} else {
						delete_char();
					}

				} break;
				case GLOBAL_CONST(KEY_KP_4): {
					if (k->get_unicode() != 0) {
						handled = false;
						break;
					}
					// numlock disabled. fallthrough to key_left
				}
				case GLOBAL_CONST(KEY_LEFT): {

#ifndef APPLE_STYLE_KEYS
					if (!k->get_alt())
#endif
						shift_selection_check_pre(k->get_shift());

#ifdef APPLE_STYLE_KEYS
					if (k->get_command()) {
						set_cursor_position(0);
					} else if (k->get_alt()) {

#else
					if (k->get_alt()) {
						handled = false;
						break;
					} else if (k->get_command()) {
#endif
						bool prev_char = false;
						int cc = cursor_pos;

						while (cc > 0) {
							bool ischar = _is_text_char(text[cc - 1]);

							if (prev_char && !ischar)
								break;

							prev_char = ischar;
							cc--;
						}

						set_cursor_position(cc);

					} else {
						set_cursor_position(get_cursor_position() - 1);
					}

					shift_selection_check_post(k->get_shift());

				} break;
				case GLOBAL_CONST(KEY_KP_6): {
					if (k->get_unicode() != 0) {
						handled = false;
						break;
					}
					// numlock disabled. fallthrough to key_right
				}
				case GLOBAL_CONST(KEY_RIGHT): {

					shift_selection_check_pre(k->get_shift());

#ifdef APPLE_STYLE_KEYS
					if (k->get_command()) {
						set_cursor_position(text.length());
					} else if (k->get_alt()) {
#else
					if (k->get_alt()) {
						handled = false;
						break;
					} else if (k->get_command()) {
#endif
						bool prev_char = false;
						int cc = cursor_pos;

						while (cc < text.length()) {
							bool ischar = _is_text_char(text[cc]);

							if (prev_char && !ischar)
								break;

							prev_char = ischar;
							cc++;
						}

						set_cursor_position(cc);

					} else {
						set_cursor_position(get_cursor_position() + 1);
					}

					shift_selection_check_post(k->get_shift());

				} break;
				case GLOBAL_CONST(KEY_UP): {

					shift_selection_check_pre(k->get_shift());
					if (get_cursor_position() == 0) handled = false;
					set_cursor_position(0);
					shift_selection_check_post(k->get_shift());
				} break;
				case GLOBAL_CONST(KEY_DOWN): {

					shift_selection_check_pre(k->get_shift());
					if (get_cursor_position() == text.length()) handled = false;
					set_cursor_position(text.length());
					shift_selection_check_post(k->get_shift());
				} break;
				case GLOBAL_CONST(KEY_DELETE): {

					if (!editable)
						break;

					if (k->get_shift() && !k->get_command() && !k->get_alt()) {
						cut_text();
						break;
					}

					if (selection.enabled) {
						selection_delete();
						break;
					}

					int text_len = text.length();

					if (cursor_pos == text_len)
						break; // nothing to do

#ifdef APPLE_STYLE_KEYS
					if (k->get_alt()) {
#else
					if (k->get_alt()) {
						handled = false;
						break;
					} else if (k->get_command()) {
#endif
						int cc = cursor_pos;

						bool prev_char = false;

						while (cc < text.length()) {

							bool ischar = _is_text_char(text[cc]);

							if (prev_char && !ischar)
								break;
							prev_char = ischar;
							cc++;
						}

						delete_text(cursor_pos, cc);

					} else {
						set_cursor_position(cursor_pos + 1);
						delete_char();
					}

				} break;
				case GLOBAL_CONST(KEY_KP_7): {
					if (k->get_unicode() != 0) {
						handled = false;
						break;
					}
					// numlock disabled. fallthrough to key_home
				}
				case GLOBAL_CONST(KEY_HOME): {

					shift_selection_check_pre(k->get_shift());
					set_cursor_position(0);
					shift_selection_check_post(k->get_shift());
				} break;
				case GLOBAL_CONST(KEY_KP_1): {
					if (k->get_unicode() != 0) {
						handled = false;
						break;
					}
					// numlock disabled. fallthrough to key_end
				}
				case GLOBAL_CONST(KEY_END): {

					shift_selection_check_pre(k->get_shift());
					set_cursor_position(text.length());
					shift_selection_check_post(k->get_shift());
				} break;

				default: {

					handled = false;
				} break;
			}

			if (handled) {
				accept_event();
			} else if (!k->get_alt() && !k->get_command()) {
				if (k->get_unicode() >= 32 && k->get_scancode() != GLOBAL_CONST(KEY_DELETE)) {

					if (editable) {
						selection_delete();
						wchar_t ucodestr[2] = { (wchar_t)k->get_unicode(), 0 };
						append_at_cursor(ucodestr);
						_text_changed();
						accept_event();
					}

				} else {
					return;
				}
			}

			update();
		}

		return;
	}
}

void TLLineEdit::set_text_direction(int p_text_direction) {

	ERR_FAIL_INDEX((int)p_text_direction, 4);
	if (base_direction != (TextDirection)p_text_direction) {
		base_direction = (TextDirection)p_text_direction;
		_text_reshape();
		update();
	}
}

int TLLineEdit::get_text_direction() const {

	return base_direction;
}

void TLLineEdit::set_ot_features(const String p_features) {

	if (ot_features != p_features) {
		ot_features = p_features;
		_text_reshape();
		update();
	}
}

String TLLineEdit::get_ot_features() const {

	return ot_features;
}

void TLLineEdit::set_language(const String p_language) {

	if (language != p_language) {
		language = p_language;
		_text_reshape();
		update();
	}
}

String TLLineEdit::get_language() const {

	return language;
}

void TLLineEdit::set_align(int p_align) {

	ERR_FAIL_INDEX((int)p_align, 4);
	align = (Align)p_align;
	update();
}

int TLLineEdit::get_align() const {

	return align;
}

Variant TLLineEdit::get_drag_data(const Point2 &p_point) {

	if (selection.drag_attempt && selection.enabled) {
		String t = text.substr(selection.begin, selection.end - selection.begin);
		TLLabel *l = memnew(TLLabel);
		l->set_text(t);
		set_drag_preview(l);
		return t;
	}

	return Variant();
}

bool TLLineEdit::can_drop_data(const Point2 &p_point, const Variant &p_data) const {

	return p_data.get_type() == Variant::STRING;
}

void TLLineEdit::drop_data(const Point2 &p_point, const Variant &p_data) {

	if (p_data.get_type() == Variant::STRING) {
		set_cursor_at_pixel_pos(p_point.x);
		int selected = selection.end - selection.begin;

		text.erase(selection.begin, selected);

		append_at_cursor(p_data);
		selection.begin = cursor_pos - selected;
		selection.end = cursor_pos;
	}
}

Control::CursorShape TLLineEdit::get_cursor_shape(const Point2 &p_pos) const {
	if (!text.empty() && is_editable() && _is_over_clear_button(p_pos)) {
		return CURSOR_ARROW;
	}
	return Control::get_cursor_shape(p_pos);
}

bool TLLineEdit::_is_over_clear_button(const Point2 &p_pos) const {
	if (!clear_button_enabled || !Rect2(Point2(), get_size()).has_point(p_pos)) {
		return false;
	}
	Ref<Texture> icon = Control::get_icon("clear");
#ifdef GODOT_MODULE
	int x_ofs = get_stylebox("normal", "LineEdit")->get_offset().x;
#else
	int x_ofs = 0;
#endif
	if (p_pos.x > get_size().width - icon->get_width() - x_ofs) {
		return true;
	}
	return false;
}

void TLLineEdit::_notification(int p_what) {

	switch (p_what) {
#ifdef TOOLS_ENABLED
		case NOTIFICATION_ENTER_TREE: {
			if (Engine::get_singleton()->is_editor_hint() && !get_tree()->is_node_being_edited(this)) {
				cursor_set_blink_enabled(EDITOR_DEF("text_editor/cursor/caret_blink", false));
				cursor_set_blink_speed(EDITOR_DEF("text_editor/cursor/caret_blink_speed", 0.65));

				if (!EditorSettings::get_singleton()->is_connected("settings_changed", this, "_editor_settings_changed")) {
					EditorSettings::get_singleton()->connect("settings_changed", this, "_editor_settings_changed");
				}
			}
		} break;
#endif
		case NOTIFICATION_RESIZED: {

			if (expand_to_text_length) {
				window_pos = 0; //force scroll back since it's expanding to text length
			}
			set_cursor_position(get_cursor_position());

		} break;
		case MainLoop::NOTIFICATION_WM_FOCUS_IN: {
			window_has_focus = true;
			draw_caret = true;
			update();
		} break;
		case MainLoop::NOTIFICATION_WM_FOCUS_OUT: {
			window_has_focus = false;
			draw_caret = false;
			update();
		} break;
		case NOTIFICATION_DRAW: {

			if ((!has_focus() && !menu->has_focus()) || !window_has_focus) {
				draw_caret = false;
			}

			int width, height;

			Size2 size = get_size();
			width = size.width;
			height = size.height;

			RID ci = get_canvas_item();

#ifdef GODOT_MODULE
			Ref<StyleBox> style = get_stylebox("normal", "LineEdit");
#else
			Ref<Theme> theme = get_theme();
			if (theme.is_null()) {
				theme.instance();
				theme->copy_default_theme();
			}
			Ref<StyleBox> style = theme->get_stylebox("normal", "LineEdit");
#endif
			float disabled_alpha = 1.0; // used to set the disabled input text color
			if (!is_editable()) {
#ifdef GODOT_MODULE
				style = get_stylebox("read_only", "LineEdit");
#else
				style = theme->get_stylebox("read_only", "LineEdit");
#endif
				disabled_alpha = .5;
				draw_caret = false;
			}

			style->draw(ci, Rect2(Point2(), size));

			if (has_focus()) {
				style->draw(ci, Rect2(Point2(), size));
			}

			int x_ofs = 0;
			bool using_placeholder = text.empty();

			switch (align) {

				case ALIGN_FILL:
				case ALIGN_LEFT: {
					x_ofs = style->get_offset().x;
				} break;
				case ALIGN_CENTER: {

					if (window_pos != 0) {
						x_ofs = style->get_offset().x;
					} else {
						x_ofs = MAX(style->get_margin(GLOBAL_CONST(MARGIN_LEFT)), int(size.width - line->get_width()) / 2);
					}
				} break;
				case ALIGN_RIGHT: {
					x_ofs = MAX(style->get_margin(GLOBAL_CONST(MARGIN_LEFT)), int(size.width - style->get_margin(GLOBAL_CONST(MARGIN_RIGHT)) - line->get_width()));
				} break;
			}
			int ofs_max = width - style->get_margin(GLOBAL_CONST(MARGIN_RIGHT));
			int y_area = height - style->get_minimum_size().height;
			int y_ofs = style->get_offset().y + (y_area - MAX(_get_base_font_height(), line->get_height())) / 2;

#ifdef GODOT_MODULE
			Color selection_color = get_color("selection_color", "LineEdit");
			Color font_color = get_color("font_color", "LineEdit");
			Color font_color_selected = get_color("font_color_selected", "LineEdit");
			Color cursor_color = get_color("cursor_color", "LineEdit");
#else
			Color selection_color = theme->get_color("selection_color", "LineEdit");
			Color font_color = theme->get_color("font_color", "LineEdit");
			Color font_color_selected = theme->get_color("font_color_selected", "LineEdit");
			Color cursor_color = theme->get_color("cursor_color", "LineEdit");
#endif

			// draw placeholder color
			if (using_placeholder)
				font_color.a *= placeholder_alpha;
			font_color.a *= disabled_alpha;

			bool display_clear_icon = !using_placeholder && is_editable() && clear_button_enabled;
			if (right_icon.is_valid() || display_clear_icon) {
				Ref<Texture> r_icon = display_clear_icon ? Control::get_icon("clear") : right_icon;
				Color color_icon(1, 1, 1, disabled_alpha * .9);
				if (display_clear_icon) {
					if (clear_button_status.press_attempt && clear_button_status.pressing_inside) {
						color_icon = get_color("clear_button_color_pressed", "LineEdit");
					} else {
						color_icon = get_color("clear_button_color", "LineEdit");
					}
				}
				r_icon->draw(ci, Point2(width - r_icon->get_width() - style->get_margin(GLOBAL_CONST(MARGIN_RIGHT)), height / 2 - r_icon->get_height() / 2), color_icon);

				if (align == ALIGN_CENTER) {
					if (window_pos == 0) {
						x_ofs = MAX(style->get_margin(GLOBAL_CONST(MARGIN_LEFT)), int(size.width - line->get_width() - r_icon->get_width() - style->get_margin(GLOBAL_CONST(MARGIN_RIGHT)) * 2) / 2);
					}
				} else {
					x_ofs = MAX(style->get_margin(GLOBAL_CONST(MARGIN_LEFT)), x_ofs - r_icon->get_width() - style->get_margin(GLOBAL_CONST(MARGIN_RIGHT)));
				}
				ofs_max -= r_icon->get_width();
			}

			int caret_height = MAX(_get_base_font_height(), line->get_height()) > y_area ? y_area : MAX(_get_base_font_height(), line->get_height());

			if (line->length() == 0) {
				//Draw solid caret at the start of empty line
				if ((cursor_pos == 0) && draw_caret) {
					VisualServer::get_singleton()->canvas_item_add_rect(ci, Rect2(Point2(x_ofs, y_ofs), Size2(1, caret_height)), cursor_color);
				}
			} else {
				//Skip offscreen
				Vector2 wpos_ofs;
				for (int i = 0; i < window_pos; i++) {
					wpos_ofs.x += line->get_cluster_width(i);
				}

#ifdef DEBUG_DISPLAY_METRICS
				VisualServer::get_singleton()->canvas_item_add_line(ci, Point2(x_ofs, y_ofs + line->get_ascent()) - wpos_ofs + Point2(0, -line->get_ascent()), Point2(x_ofs, y_ofs + line->get_ascent()) - wpos_ofs + Point2(line->get_width(), -line->get_ascent()), Color(1, 0, 0, 0.5), 1);
				VisualServer::get_singleton()->canvas_item_add_line(ci, Point2(x_ofs, y_ofs + line->get_ascent()) - wpos_ofs + Point2(0, 0), Point2(x_ofs, y_ofs + line->get_ascent()) - wpos_ofs + Point2(line->get_width(), 0), Color(1, 1, 0, 0.5), 1);
				VisualServer::get_singleton()->canvas_item_add_line(ci, Point2(x_ofs, y_ofs + line->get_ascent()) - wpos_ofs + Point2(0, line->get_descent()), Point2(x_ofs, y_ofs + line->get_ascent()) - wpos_ofs + Point2(line->get_width(), line->get_descent()), Color(0, 0, 1, 0.5), 1);
#endif

				//Draw selection rects
				if (selection.enabled) {
					std::vector<Rect2> sranges = line->get_highlight_shapes(selection.begin, selection.end);
					for (int64_t i = 0; i < (int64_t)sranges.size(); i++) {
						float s_st = x_ofs + sranges[i].position.x - wpos_ofs.x >= 0 ? x_ofs + sranges[i].position.x - wpos_ofs.x : x_ofs;
						if (s_st < ofs_max) {
							float s_wdt = s_st + sranges[i].size.x <= ofs_max ? sranges[i].size.x : ofs_max - s_st;
							VisualServer::get_singleton()->canvas_item_add_rect(ci, Rect2(Point2(s_st, y_ofs), Size2(s_wdt, caret_height)), selection_color);
						}
					}
				}

				//Draw clusters
				Vector2 ofs;
				for (int i = 0; i < line->clusters(); i++) {
					if ((ofs.x - wpos_ofs.x + line->get_cluster_width(i) <= ofs_max) && (ofs.x - wpos_ofs.x >= 0)) {
						bool selected = selection.enabled && line->get_cluster_start(i) >= selection.begin && line->get_cluster_end(i) < selection.end;
						Vector2 char_ofs = line->draw_cluster(ci, Point2(x_ofs, y_ofs + line->get_ascent()) + ofs - wpos_ofs, i, selected ? font_color_selected : font_color);
						ofs += char_ofs;
					} else {
						ofs.x += line->get_cluster_width(i);
					}
				}

				//Draw caret
				if (draw_caret) {
					std::vector<float> carets = line->get_cursor_positions(cursor_pos, last_input_direction);
					if (carets.size() == 1) {
						//Draw solid caret (leading or midcluster)
#ifdef TOOLS_ENABLED
						VisualServer::get_singleton()->canvas_item_add_rect(ci, Rect2(Point2(x_ofs + carets[0], y_ofs) - wpos_ofs, Size2(Math::round(EDSCALE), caret_height)), cursor_color);
#else
						VisualServer::get_singleton()->canvas_item_add_rect(ci, Rect2(Point2(x_ofs + carets[0], y_ofs) - wpos_ofs, Size2(1, caret_height)), cursor_color);
#endif
					} else if (carets.size() == 2) {
						//Draw split caret (leading and trailing)
						float inv_caret_circle_size = 3.0;
#ifdef TOOLS_ENABLED
						VisualServer::get_singleton()->canvas_item_add_rect(ci, Rect2(Point2(x_ofs + carets[0], y_ofs) - wpos_ofs, Size2(Math::round(EDSCALE), caret_height / 2)), cursor_color);
						if (x_ofs + carets[1] - wpos_ofs.x < 0) {
							VisualServer::get_singleton()->canvas_item_add_circle(ci, Point2(style->get_margin(GLOBAL_CONST(MARGIN_LEFT)), style->get_offset().y), inv_caret_circle_size, cursor_color);
						} else if (x_ofs + carets[1] - wpos_ofs.x > ofs_max) {
							VisualServer::get_singleton()->canvas_item_add_circle(ci, Point2(ofs_max, style->get_offset().y), inv_caret_circle_size, cursor_color);
						} else {
							VisualServer::get_singleton()->canvas_item_add_rect(ci, Rect2(Point2(x_ofs + carets[1], y_ofs + caret_height / 2) - wpos_ofs, Size2(Math::round(EDSCALE), caret_height / 2)), cursor_color);
						}
#else
						VisualServer::get_singleton()->canvas_item_add_rect(ci, Rect2(Point2(x_ofs + carets[0], y_ofs) - wpos_ofs, Size2(1, caret_height / 2)), cursor_color);
						if (x_ofs + carets[1] - wpos_ofs.x < 0) {
							VisualServer::get_singleton()->canvas_item_add_circle(ci, Point2(style->get_margin(GLOBAL_CONST(MARGIN_LEFT)), style->get_offset().y), inv_caret_circle_size, cursor_color);
						} else if (x_ofs + carets[1] - wpos_ofs.x > ofs_max) {
							VisualServer::get_singleton()->canvas_item_add_circle(ci, Point2(ofs_max, style->get_offset().y), inv_caret_circle_size, cursor_color);
						} else {
							VisualServer::get_singleton()->canvas_item_add_rect(ci, Rect2(Point2(x_ofs + carets[1], y_ofs + caret_height / 2) - wpos_ofs, Size2(1, caret_height / 2)), cursor_color);
						}
#endif
					}
				}
			}

			if (has_focus()) {

				OS::get_singleton()->set_ime_active(true);
				OS::get_singleton()->set_ime_position(get_global_position() + Point2(using_placeholder ? 0 : x_ofs, y_ofs + caret_height));
			}
		} break;
		case NOTIFICATION_FOCUS_ENTER: {

			if (!caret_blink_enabled) {
				draw_caret = true;
			}

			OS::get_singleton()->set_ime_active(true);
			Point2 cursor_pos = Point2(get_cursor_position(), 1) * get_minimum_size().height;
			OS::get_singleton()->set_ime_position(get_global_position() + cursor_pos);

			if (OS::get_singleton()->has_virtual_keyboard())
#ifdef GODOT_MODULE
				OS::get_singleton()->show_virtual_keyboard(text, get_global_rect());
#else
				OS::get_singleton()->show_virtual_keyboard(text);
#endif

		} break;
		case NOTIFICATION_FOCUS_EXIT: {

			OS::get_singleton()->set_ime_position(Point2());
			OS::get_singleton()->set_ime_active(false);
			ime_text = "";
			ime_selection = Point2();

			_text_reshape();

			if (OS::get_singleton()->has_virtual_keyboard())
				OS::get_singleton()->hide_virtual_keyboard();

		} break;
		case MainLoop::NOTIFICATION_OS_IME_UPDATE: {

			if (has_focus()) {
				ime_text = OS::get_singleton()->get_ime_text();
				ime_selection = OS::get_singleton()->get_ime_selection();

				_text_reshape();
				update();
			}

		} break;
		case NOTIFICATION_THEME_CHANGED: {

			_text_reshape();
		} break;
	}
}

void TLLineEdit::copy_text() {

	if (selection.enabled && !pass) {
		OS::get_singleton()->set_clipboard(text.substr(selection.begin, selection.end - selection.begin));
	}
}

void TLLineEdit::cut_text() {

	if (selection.enabled && !pass) {
		OS::get_singleton()->set_clipboard(text.substr(selection.begin, selection.end - selection.begin));
		selection_delete();
	}
}

void TLLineEdit::paste_text() {

	String paste_buffer = OS::get_singleton()->get_clipboard();

	if (paste_buffer != "") {

		if (selection.enabled) selection_delete();
		append_at_cursor(paste_buffer);

		if (!text_changed_dirty) {
			if (is_inside_tree()) {
#ifdef GODOT_MODULE
				MessageQueue::get_singleton()->push_call(this, "_text_changed");
#endif
			}
			text_changed_dirty = true;
		}
	}
}

void TLLineEdit::undo() {
	if (undo_stack_pos == undo_stack.end()) {
		if (undo_stack.size() <= 1) {
			return;
		}
		undo_stack_pos = undo_stack.end();
	} else if (undo_stack_pos == undo_stack.begin()) {
		return;
	}
	undo_stack_pos--;
	TextOperation op = (*undo_stack_pos);
	text = op.text;
	_text_reshape();

	set_cursor_position(op.cursor_pos);
	_emit_text_change();
}

void TLLineEdit::redo() {
	if (undo_stack_pos == undo_stack.end()) {
		return;
	}
	undo_stack_pos++;
	TextOperation op = (*undo_stack_pos);
	text = op.text;
	_text_reshape();

	set_cursor_position(op.cursor_pos);
	_emit_text_change();
}

void TLLineEdit::shift_selection_check_pre(bool p_shift) {

	if (!selection.enabled && p_shift) {
		selection.cursor_start = cursor_pos;
	}
	if (!p_shift)
		deselect();
}

void TLLineEdit::shift_selection_check_post(bool p_shift) {

	if (p_shift)
		selection_fill_at_cursor();
}

void TLLineEdit::set_cursor_at_pixel_pos(int p_x) {

#ifdef GODOT_MODULE
	Ref<StyleBox> style = get_stylebox("normal", "LineEdit");
#else
	Ref<Theme> theme = get_theme();
	if (theme.is_null()) {
		theme.instance();
		theme->copy_default_theme();
	}
	Ref<StyleBox> style = theme->get_stylebox("normal", "LineEdit");
#endif
	Size2 size = get_size();

	//Calc offscreen offset
	float wpos_ofs = 0.0;
	for (int i = 0; i < window_pos; i++) {
		wpos_ofs += line->get_cluster_width(i);
	}

	float x_ofs = 0.0;
	switch (align) {

		case ALIGN_FILL:
		case ALIGN_LEFT: {
			x_ofs = style->get_offset().x;
		} break;
		case ALIGN_CENTER: {

			if (window_pos != 0) {
				x_ofs = style->get_offset().x;
			} else {
				x_ofs = MAX(style->get_margin(GLOBAL_CONST(MARGIN_LEFT)), int(size.width - line->get_width()) / 2);
			}
		} break;
		case ALIGN_RIGHT: {
			x_ofs = MAX(style->get_margin(GLOBAL_CONST(MARGIN_LEFT)), int(size.width - style->get_margin(GLOBAL_CONST(MARGIN_RIGHT)) - line->get_width()));
		} break;
	}

	set_cursor_position(line->hit_test(p_x - x_ofs - wpos_ofs));
}

bool TLLineEdit::cursor_get_blink_enabled() const {
	return caret_blink_enabled;
}

void TLLineEdit::cursor_set_blink_enabled(const bool p_enabled) {
	caret_blink_enabled = p_enabled;
	if (p_enabled) {
		caret_blink_timer->start();
	} else {
		caret_blink_timer->stop();
	}
	draw_caret = true;
}

float TLLineEdit::cursor_get_blink_speed() const {
	return caret_blink_timer->get_wait_time();
}

void TLLineEdit::cursor_set_blink_speed(const float p_speed) {
	ERR_FAIL_COND(p_speed <= 0);
	caret_blink_timer->set_wait_time(p_speed);
}

void TLLineEdit::_reset_caret_blink_timer() {
	if (caret_blink_enabled) {
		caret_blink_timer->stop();
		caret_blink_timer->start();
		draw_caret = true;
		update();
	}
}

void TLLineEdit::_toggle_draw_caret() {
	draw_caret = !draw_caret;
	if (is_visible_in_tree() && has_focus() && window_has_focus) {
		update();
	}
}

void TLLineEdit::delete_char() {

	if ((text.length() <= 0) || (cursor_pos == 0)) return;

	text.erase(cursor_pos - 1, 1);

	_text_reshape();

	set_cursor_position(get_cursor_position() - 1);

	_text_changed();
}

void TLLineEdit::delete_text(int p_from_column, int p_to_column) {

	text.erase(p_from_column, p_to_column - p_from_column);

	_text_reshape();

	cursor_pos -= CLAMP(cursor_pos - p_from_column, 0, p_to_column - p_from_column);

	if (cursor_pos >= text.length()) {

		cursor_pos = text.length();
	}

	if (!text_changed_dirty) {
		if (is_inside_tree()) {
#ifdef GODOT_MODULE
			MessageQueue::get_singleton()->push_call(this, "_text_changed");
#endif
		}
		text_changed_dirty = true;
	}
}

void TLLineEdit::set_text(String p_text) {

	clear_internal();
	append_at_cursor(p_text);
	update();
	cursor_pos = 0;
	window_pos = 0;
}

Ref<TLFontFamily> TLLineEdit::get_base_font() const {

	return line->get_base_font();
}

void TLLineEdit::_font_changed() {
	update();
}

void TLLineEdit::set_base_font(const Ref<TLFontFamily> p_font) {

	line->set_base_font(p_font);
	_text_reshape();
	update();
}

String TLLineEdit::get_base_font_style() const {

	return line->get_base_font_style();
}

void TLLineEdit::set_base_font_style(const String p_style) {

	line->set_base_font_style(p_style);
	_text_reshape();
	update();
}

int TLLineEdit::get_base_font_size() const {

	return line->get_base_font_size();
}

void TLLineEdit::set_base_font_size(int p_size) {

	line->set_base_font_size(p_size);
	_text_reshape();
	update();
}

void TLLineEdit::clear() {

	clear_internal();
	_text_changed();
}

String TLLineEdit::get_text() const {

	return text;
}

void TLLineEdit::set_placeholder(String p_text) {

	placeholder = tr(p_text);

	_text_reshape();

	update();
}

String TLLineEdit::get_placeholder() const {

	return placeholder;
}

void TLLineEdit::set_placeholder_alpha(float p_alpha) {

	placeholder_alpha = p_alpha;
	update();
}

float TLLineEdit::get_placeholder_alpha() const {

	return placeholder_alpha;
}

void TLLineEdit::set_cursor_position(int p_pos) {

#ifdef GODOT_MODULE
	Ref<StyleBox> style = get_stylebox("normal", "LineEdit");
#else
	Ref<Theme> theme = get_theme();
	if (theme.is_null()) {
		theme.instance();
		theme->copy_default_theme();
	}
	Ref<StyleBox> style = theme->get_stylebox("normal", "LineEdit");
#endif
	float window_width = get_size().width - style->get_minimum_size().width;

	if (p_pos > (int)text.length())
		p_pos = text.length();

	if (p_pos < 0)
		p_pos = 0;

	//Save last direction
	last_input_direction = line->get_char_direction(cursor_pos);

	//Set new cursor position
	cursor_pos = p_pos;

	if (!is_inside_tree()) {

		window_pos = 0;
		return;
	}

	bool display_clear_icon = !text.empty() && is_editable() && clear_button_enabled;
	if (right_icon.is_valid() || display_clear_icon) {
		Ref<Texture> r_icon = display_clear_icon ? Control::get_icon("clear") : right_icon;
		window_width -= r_icon->get_width();
	}

	//Calc window_pos offsets in pixels
	float wpos_ofs = 0.0f;
	for (int i = 0; i < window_pos; i++) {
		wpos_ofs += line->get_cluster_width(i);
	}

	std::vector<float> carets = line->get_cursor_positions(cursor_pos, last_input_direction);
	if (carets.size() > 0) {
		int wp = window_pos;
		// Adjust window if cursor goes too much to the left
		while ((carets[0] - wpos_ofs < 0) && (wp > 0) && (wp <= line->clusters())) {
			wp--;
			wpos_ofs -= line->get_cluster_width(wp);
		}
		// Adjust window if cursor goes too much to the right
		while ((carets[0] - wpos_ofs > window_width) && (wp >= 0) && (wp < line->clusters())) {
			wpos_ofs += line->get_cluster_width(wp);
			wp++;
		}
		//Set window pos
		if ((wp != window_pos) && (wp >= 0) && (wp < line->clusters())) {
			set_window_pos(wp);
		}
	}

	update();
}

int TLLineEdit::get_cursor_position() const {

	return cursor_pos;
}

void TLLineEdit::set_window_pos(int p_pos) {

	window_pos = p_pos;
	if (window_pos < 0) window_pos = 0;
	if (window_pos >= line->clusters()) window_pos = line->clusters() - 1;
}

void TLLineEdit::append_at_cursor(String p_text) {

	if ((max_length <= 0) || (text.length() + p_text.length() <= max_length)) {

		String pre = text.substr(0, cursor_pos);
		String post = text.substr(cursor_pos, text.length() - cursor_pos);
		text = pre + p_text + post;
		_text_reshape();

		set_cursor_position(cursor_pos + p_text.length());
	}
}

void TLLineEdit::clear_internal() {

	_clear_undo_stack();

	cursor_pos = 0;
	window_pos = 0;
	undo_text = "";
	text = "";
	_text_reshape();

	update();
}

Size2 TLLineEdit::get_minimum_size() const {

#ifdef GODOT_MODULE
	Ref<StyleBox> style = get_stylebox("normal", "LineEdit");
#else
	Ref<Theme> theme = get_theme();
	if (theme.is_null()) {
		theme.instance();
		theme->copy_default_theme();
	}
	Ref<StyleBox> style = theme->get_stylebox("normal", "LineEdit");
#endif
	Size2 min = style->get_minimum_size();
	min.height += MAX(_get_base_font_height(), line->get_ascent() + line->get_descent());

	//minimum size of text
	int space_size = 10;
	int mstext = get_constant("minimum_spaces", "LineEdit") * space_size;

	if (expand_to_text_length) {
		mstext = line->get_width();
	}

	min.width += mstext;

	return min;
}

/* selection */

void TLLineEdit::deselect() {

	selection.begin = 0;
	selection.end = 0;
	selection.cursor_start = 0;
	selection.enabled = false;
	selection.creating = false;
	selection.doubleclick = false;
	update();
}

void TLLineEdit::selection_delete() {

	if (selection.enabled)
		delete_text(selection.begin, selection.end);

	deselect();
}

void TLLineEdit::set_max_length(int p_max_length) {

	ERR_FAIL_COND(p_max_length < 0);
	max_length = p_max_length;
	set_text(text);
}

int TLLineEdit::get_max_length() const {

	return max_length;
}

void TLLineEdit::selection_fill_at_cursor() {

	selection.begin = cursor_pos;
	selection.end = selection.cursor_start;

	if (selection.end < selection.begin) {
		int aux = selection.end;
		selection.end = selection.begin;
		selection.begin = aux;
	}

	selection.enabled = (selection.begin != selection.end);
}

void TLLineEdit::select_all() {

	if (!text.length())
		return;

	selection.begin = 0;
	selection.end = text.length();
	selection.enabled = true;
	update();
}

void TLLineEdit::set_editable(bool p_editable) {

	editable = p_editable;
	update();
}

bool TLLineEdit::is_editable() const {

	return editable;
}

void TLLineEdit::set_secret(bool p_secret) {

	pass = p_secret;

	_text_reshape();
	update();
}

bool TLLineEdit::is_secret() const {

	return pass;
}

void TLLineEdit::set_secret_character(const String p_string) {

	// An empty string as the secret character would crash the engine
	// It also wouldn't make sense to use multiple characters as the secret character
	if (p_string.length() != 1)
		return;

	secret_character = p_string;

	_text_reshape();
	update();
}

String TLLineEdit::get_secret_character() const {
	return secret_character;
}

void TLLineEdit::select(int p_from, int p_to) {

	if (p_from == 0 && p_to == 0) {
		deselect();
		return;
	}

	int len = text.length();
	if (p_from < 0)
		p_from = 0;
	if (p_from > len)
		p_from = len;
	if (p_to < 0 || p_to > len)
		p_to = len;

	if (p_from >= p_to)
		return;

	selection.enabled = true;
	selection.begin = p_from;
	selection.end = p_to;
	selection.creating = false;
	selection.doubleclick = false;
	update();
}

bool TLLineEdit::is_text_field() const {

	return true;
}

void TLLineEdit::menu_option(int p_option) {

	switch (p_option) {
		case MENU_CUT: {
			if (editable) {
				cut_text();
			}
		} break;
		case MENU_COPY: {

			copy_text();
		} break;
		case MENU_PASTE: {
			if (editable) {
				paste_text();
			}
		} break;
		case MENU_CLEAR: {
			if (editable) {
				clear();
			}
		} break;
		case MENU_SELECT_ALL: {
			select_all();
		} break;
		case MENU_UNDO: {
			if (editable) {
				undo();
			}
		} break;
		case MENU_REDO: {
			if (editable) {
				redo();
			}
		}
	}
}

void TLLineEdit::set_context_menu_enabled(bool p_enable) {
	context_menu_enabled = p_enable;
}

bool TLLineEdit::is_context_menu_enabled() {
	return context_menu_enabled;
}

PopupMenu *TLLineEdit::get_menu() const {
	return menu;
}

void TLLineEdit::_editor_settings_changed() {
#ifdef TOOLS_ENABLED
	cursor_set_blink_enabled(EDITOR_DEF("text_editor/cursor/caret_blink", false));
	cursor_set_blink_speed(EDITOR_DEF("text_editor/cursor/caret_blink_speed", 0.65));
#endif
}

void TLLineEdit::set_expand_to_text_length(bool p_enabled) {

	expand_to_text_length = p_enabled;
	minimum_size_changed();
	set_window_pos(0);
}

bool TLLineEdit::get_expand_to_text_length() const {
	return expand_to_text_length;
}

void TLLineEdit::set_clear_button_enabled(bool p_enabled) {
	clear_button_enabled = p_enabled;
	update();
}

bool TLLineEdit::is_clear_button_enabled() const {
	return clear_button_enabled;
}

void TLLineEdit::set_right_icon(const Ref<Texture> &p_icon) {
	if (right_icon == p_icon) {
		return;
	}
	right_icon = p_icon;
	update();
}

void TLLineEdit::_text_reshape() {

	String text_to_draw;
	if (ime_text.length() > 0) {
		text_to_draw = text.substr(0, cursor_pos) + ime_text + text.substr(cursor_pos, text.length());
	} else {
		text_to_draw = text.empty() ? placeholder : text;
	}
	if (pass && !text_to_draw.empty()) {
		int count = text_to_draw.length();
		text_to_draw = String();
		for (int i = 0; i < count; i++) {
			text_to_draw += secret_character[0];
		}
	}

	line->set_base_direction(base_direction);
	line->set_text(text_to_draw);
	line->set_language(language);
	line->set_features(ot_features);

	if (line->clusters() > 0) {
		if (window_pos >= line->clusters()) window_pos = line->clusters() - 1;
	} else {
		window_pos = 0;
	}
}

void TLLineEdit::_text_changed() {

	if (expand_to_text_length)
		minimum_size_changed();

	_emit_text_change();
	_clear_redo();
}

void TLLineEdit::_emit_text_change() {
	emit_signal("text_changed", text);
#ifdef GODOT_MODULE
	_change_notify("text");
#else
	property_list_changed_notify();
#endif
	text_changed_dirty = false;
}

void TLLineEdit::_clear_redo() {
	_create_undo_state();
	if (undo_stack_pos == undo_stack.end()) {
		return;
	}

	undo_stack_pos++;
	while (undo_stack_pos != undo_stack.end()) {
		std::list<TextOperation>::iterator elem = undo_stack_pos;
		undo_stack_pos++;
		undo_stack.erase(elem);
	}
	_create_undo_state();
}

void TLLineEdit::_clear_undo_stack() {
	undo_stack.clear();
	undo_stack_pos = undo_stack.end();
	_create_undo_state();
}

void TLLineEdit::_create_undo_state() {
	TextOperation op;
	op.text = text;
	op.cursor_pos = cursor_pos;
	undo_stack.push_back(op);
}

#ifdef GODOT_MODULE

void TLLineEdit::_bind_methods() {

	ClassDB::bind_method(D_METHOD("_font_changed"), &TLLineEdit::_font_changed);
	ClassDB::bind_method(D_METHOD("_text_changed"), &TLLineEdit::_text_changed);
	ClassDB::bind_method(D_METHOD("_toggle_draw_caret"), &TLLineEdit::_toggle_draw_caret);

	ClassDB::bind_method("_editor_settings_changed", &TLLineEdit::_editor_settings_changed);

	ClassDB::bind_method(D_METHOD("set_align", "align"), &TLLineEdit::set_align);
	ClassDB::bind_method(D_METHOD("get_align"), &TLLineEdit::get_align);
	ClassDB::bind_method(D_METHOD("set_text_direction", "direction"), &TLLineEdit::set_text_direction);
	ClassDB::bind_method(D_METHOD("get_text_direction"), &TLLineEdit::get_text_direction);
	ClassDB::bind_method(D_METHOD("set_ot_features", "features"), &TLLineEdit::set_ot_features);
	ClassDB::bind_method(D_METHOD("get_ot_features"), &TLLineEdit::get_ot_features);
	ClassDB::bind_method(D_METHOD("set_language", "language"), &TLLineEdit::set_language);
	ClassDB::bind_method(D_METHOD("get_language"), &TLLineEdit::get_language);

	ClassDB::bind_method(D_METHOD("set_base_font", "ref"), &TLLineEdit::set_base_font);
	ClassDB::bind_method(D_METHOD("get_base_font"), &TLLineEdit::get_base_font);

	ClassDB::bind_method(D_METHOD("set_base_font_style", "style"), &TLLineEdit::set_base_font_style);
	ClassDB::bind_method(D_METHOD("get_base_font_style"), &TLLineEdit::get_base_font_style);

	ClassDB::bind_method(D_METHOD("set_base_font_size", "size"), &TLLineEdit::set_base_font_size);
	ClassDB::bind_method(D_METHOD("get_base_font_size"), &TLLineEdit::get_base_font_size);

	ClassDB::bind_method(D_METHOD("_gui_input"), &TLLineEdit::_gui_input);
	ClassDB::bind_method(D_METHOD("clear"), &TLLineEdit::clear);
	ClassDB::bind_method(D_METHOD("select", "from", "to"), &TLLineEdit::select, DEFVAL(0), DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("select_all"), &TLLineEdit::select_all);
	ClassDB::bind_method(D_METHOD("deselect"), &TLLineEdit::deselect);
	ClassDB::bind_method(D_METHOD("set_text", "text"), &TLLineEdit::set_text);
	ClassDB::bind_method(D_METHOD("get_text"), &TLLineEdit::get_text);
	ClassDB::bind_method(D_METHOD("set_placeholder", "text"), &TLLineEdit::set_placeholder);
	ClassDB::bind_method(D_METHOD("get_placeholder"), &TLLineEdit::get_placeholder);
	ClassDB::bind_method(D_METHOD("set_placeholder_alpha", "alpha"), &TLLineEdit::set_placeholder_alpha);
	ClassDB::bind_method(D_METHOD("get_placeholder_alpha"), &TLLineEdit::get_placeholder_alpha);
	ClassDB::bind_method(D_METHOD("set_cursor_position", "position"), &TLLineEdit::set_cursor_position);
	ClassDB::bind_method(D_METHOD("get_cursor_position"), &TLLineEdit::get_cursor_position);
	ClassDB::bind_method(D_METHOD("set_expand_to_text_length", "enabled"), &TLLineEdit::set_expand_to_text_length);
	ClassDB::bind_method(D_METHOD("get_expand_to_text_length"), &TLLineEdit::get_expand_to_text_length);
	ClassDB::bind_method(D_METHOD("cursor_set_blink_enabled", "enabled"), &TLLineEdit::cursor_set_blink_enabled);
	ClassDB::bind_method(D_METHOD("cursor_get_blink_enabled"), &TLLineEdit::cursor_get_blink_enabled);
	ClassDB::bind_method(D_METHOD("cursor_set_blink_speed", "blink_speed"), &TLLineEdit::cursor_set_blink_speed);
	ClassDB::bind_method(D_METHOD("cursor_get_blink_speed"), &TLLineEdit::cursor_get_blink_speed);
	ClassDB::bind_method(D_METHOD("set_max_length", "chars"), &TLLineEdit::set_max_length);
	ClassDB::bind_method(D_METHOD("get_max_length"), &TLLineEdit::get_max_length);
	ClassDB::bind_method(D_METHOD("append_at_cursor", "text"), &TLLineEdit::append_at_cursor);
	ClassDB::bind_method(D_METHOD("set_editable", "enabled"), &TLLineEdit::set_editable);
	ClassDB::bind_method(D_METHOD("is_editable"), &TLLineEdit::is_editable);
	ClassDB::bind_method(D_METHOD("set_secret", "enabled"), &TLLineEdit::set_secret);
	ClassDB::bind_method(D_METHOD("is_secret"), &TLLineEdit::is_secret);
	ClassDB::bind_method(D_METHOD("set_secret_character", "character"), &TLLineEdit::set_secret_character);
	ClassDB::bind_method(D_METHOD("get_secret_character"), &TLLineEdit::get_secret_character);
	ClassDB::bind_method(D_METHOD("menu_option", "option"), &TLLineEdit::menu_option);
	ClassDB::bind_method(D_METHOD("get_menu"), &TLLineEdit::get_menu);
	ClassDB::bind_method(D_METHOD("set_context_menu_enabled", "enable"), &TLLineEdit::set_context_menu_enabled);
	ClassDB::bind_method(D_METHOD("is_context_menu_enabled"), &TLLineEdit::is_context_menu_enabled);
	ClassDB::bind_method(D_METHOD("set_clear_button_enabled", "enable"), &TLLineEdit::set_clear_button_enabled);
	ClassDB::bind_method(D_METHOD("is_clear_button_enabled"), &TLLineEdit::is_clear_button_enabled);

	ADD_SIGNAL(MethodInfo("text_changed", PropertyInfo(Variant::STRING, "new_text")));
	ADD_SIGNAL(MethodInfo("text_entered", PropertyInfo(Variant::STRING, "new_text")));

	BIND_ENUM_CONSTANT(ALIGN_LEFT);
	BIND_ENUM_CONSTANT(ALIGN_CENTER);
	BIND_ENUM_CONSTANT(ALIGN_RIGHT);
	BIND_ENUM_CONSTANT(ALIGN_FILL);

	BIND_ENUM_CONSTANT(MENU_CUT);
	BIND_ENUM_CONSTANT(MENU_COPY);
	BIND_ENUM_CONSTANT(MENU_PASTE);
	BIND_ENUM_CONSTANT(MENU_CLEAR);
	BIND_ENUM_CONSTANT(MENU_SELECT_ALL);
	BIND_ENUM_CONSTANT(MENU_UNDO);
	BIND_ENUM_CONSTANT(MENU_REDO);
	BIND_ENUM_CONSTANT(MENU_MAX);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "text"), "set_text", "get_text");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "align", PROPERTY_HINT_ENUM, "Left,Center,Right,Fill"), "set_align", "get_align");

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "base_font", PROPERTY_HINT_RESOURCE_TYPE, "TLFontFamily"), "set_base_font", "get_base_font");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "base_font_size"), "set_base_font_size", "get_base_font_size");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "base_font_style"), "set_base_font_style", "get_base_font_style");

	ADD_PROPERTY(PropertyInfo(Variant::INT, "text_direction", PROPERTY_HINT_ENUM, "LTR,RTL,Locale,Auto"), "set_text_direction", "get_text_direction");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "ot_features"), "set_ot_features", "get_ot_features");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "language"), "set_language", "get_language");

	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_length"), "set_max_length", "get_max_length");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "editable"), "set_editable", "is_editable");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "secret"), "set_secret", "is_secret");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "secret_character"), "set_secret_character", "get_secret_character");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "expand_to_text_length"), "set_expand_to_text_length", "get_expand_to_text_length");

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "context_menu_enabled"), "set_context_menu_enabled", "is_context_menu_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "clear_button_enabled"), "set_clear_button_enabled", "is_clear_button_enabled");
	ADD_GROUP("Placeholder", "placeholder_");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "placeholder_text"), "set_placeholder", "get_placeholder");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "placeholder_alpha", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_placeholder_alpha", "get_placeholder_alpha");
	ADD_GROUP("Caret", "caret_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "caret_blink"), "cursor_set_blink_enabled", "cursor_get_blink_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "caret_blink_speed", PROPERTY_HINT_RANGE, "0.1,10,0.01"), "cursor_set_blink_speed", "cursor_get_blink_speed");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "caret_position"), "set_cursor_position", "get_cursor_position");
}

#else
bool TLLineEdit::_set(String p_name, Variant p_value) {
	if (p_name == "base_font") {
		Object *p_obj = p_value;
		Ref<TLFontFamily> font = Ref<TLFontFamily>(Object::cast_to<TLFontFamily>(p_obj));
		if (font.is_valid()) {
			set_base_font(font);
			return true;
		}
	} else if (p_name == "focus_mode") {
		int64_t fm = p_value;
		set_focus_mode(fm);
		return true;
	}
	return false;
}

Variant TLLineEdit::_get(String p_name) const {
	if (p_name == "base_font") {
		return get_base_font();
	} else if (p_name == "focus_mode") {
		return get_focus_mode();
	}
	return Variant();
}

Array TLLineEdit::_get_property_list() const {
	Array ret;
	{
		Dictionary prop;
		prop["name"] = "base_font";
		prop["type"] = GlobalConstants::TYPE_OBJECT;
		prop["hint"] = GlobalConstants::PROPERTY_HINT_RESOURCE_TYPE;
		prop["hint_string"] = "TLFontFamily";
		prop["usage"] = GlobalConstants::PROPERTY_USAGE_EDITOR | GODOT_PROPERTY_USAGE_STORAGE;
		ret.push_back(prop);
	}
	{
		Dictionary prop;
		prop["name"] = "focus_mode";
		prop["type"] = GlobalConstants::TYPE_INT;
		prop["hint"] = GlobalConstants::PROPERTY_HINT_ENUM;
		prop["hint_string"] = String("None,Click,All");
		prop["usage"] = GlobalConstants::PROPERTY_USAGE_DEFAULT;
		ret.push_back(prop);
	}

	return ret;
}

void TLLineEdit::_register_methods() {

	register_method("_font_changed", &TLLineEdit::_font_changed);
	register_method("_text_changed", &TLLineEdit::_text_changed);
	register_method("_toggle_draw_caret", &TLLineEdit::_toggle_draw_caret);

	register_method("_editor_settings_changed", &TLLineEdit::_editor_settings_changed);

	register_method("set_align", &TLLineEdit::set_align);
	register_method("get_align", &TLLineEdit::get_align);
	register_method("set_text_direction", &TLLineEdit::set_text_direction);
	register_method("get_text_direction", &TLLineEdit::get_text_direction);
	register_method("set_ot_features", &TLLineEdit::set_ot_features);
	register_method("get_ot_features", &TLLineEdit::get_ot_features);
	register_method("set_language", &TLLineEdit::set_language);
	register_method("get_language", &TLLineEdit::get_language);

	register_method("set_base_font", &TLLineEdit::set_base_font);
	register_method("get_base_font", &TLLineEdit::get_base_font);

	register_method("set_base_font_style", &TLLineEdit::set_base_font_style);
	register_method("get_base_font_style", &TLLineEdit::get_base_font_style);

	register_method("set_base_font_size", &TLLineEdit::set_base_font_size);
	register_method("get_base_font_size", &TLLineEdit::get_base_font_size);

	register_method("_gui_input", &TLLineEdit::_gui_input);
	register_method("clear", &TLLineEdit::clear);
	register_method("select", &TLLineEdit::select);
	register_method("select_all", &TLLineEdit::select_all);
	register_method("deselect", &TLLineEdit::deselect);
	register_method("set_text", &TLLineEdit::set_text);
	register_method("get_text", &TLLineEdit::get_text);
	register_method("set_placeholder", &TLLineEdit::set_placeholder);
	register_method("get_placeholder", &TLLineEdit::get_placeholder);
	register_method("set_placeholder_alpha", &TLLineEdit::set_placeholder_alpha);
	register_method("get_placeholder_alpha", &TLLineEdit::get_placeholder_alpha);
	register_method("set_cursor_position", &TLLineEdit::set_cursor_position);
	register_method("get_cursor_position", &TLLineEdit::get_cursor_position);
	register_method("set_expand_to_text_length", &TLLineEdit::set_expand_to_text_length);
	register_method("get_expand_to_text_length", &TLLineEdit::get_expand_to_text_length);
	register_method("cursor_set_blink_enabled", &TLLineEdit::cursor_set_blink_enabled);
	register_method("cursor_get_blink_enabled", &TLLineEdit::cursor_get_blink_enabled);
	register_method("cursor_set_blink_speed", &TLLineEdit::cursor_set_blink_speed);
	register_method("cursor_get_blink_speed", &TLLineEdit::cursor_get_blink_speed);
	register_method("set_max_length", &TLLineEdit::set_max_length);
	register_method("get_max_length", &TLLineEdit::get_max_length);
	register_method("append_at_cursor", &TLLineEdit::append_at_cursor);
	register_method("set_editable", &TLLineEdit::set_editable);
	register_method("is_editable", &TLLineEdit::is_editable);
	register_method("set_secret", &TLLineEdit::set_secret);
	register_method("is_secret", &TLLineEdit::is_secret);
	register_method("set_secret_character", &TLLineEdit::set_secret_character);
	register_method("get_secret_character", &TLLineEdit::get_secret_character);
	register_method("menu_option", &TLLineEdit::menu_option);
	register_method("get_menu", &TLLineEdit::get_menu);
	register_method("set_context_menu_enabled", &TLLineEdit::set_context_menu_enabled);
	register_method("is_context_menu_enabled", &TLLineEdit::is_context_menu_enabled);
	register_method("set_clear_button_enabled", &TLLineEdit::set_clear_button_enabled);
	register_method("is_clear_button_enabled", &TLLineEdit::is_clear_button_enabled);

	register_signal<TLLineEdit>("text_changed");
	register_signal<TLLineEdit>("text_entered");

	register_property<TLLineEdit, String>("text", &TLLineEdit::set_text, &TLLineEdit::get_text, String(), GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_MULTILINE_TEXT, String(""));
	register_property<TLLineEdit, int>("align", &TLLineEdit::set_align, &TLLineEdit::get_align, ALIGN_LEFT, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM, String("Left,Center,Right,Fill"));

	register_property<TLLineEdit, String>("base_font_style", &TLLineEdit::set_base_font_style, &TLLineEdit::get_base_font_style, String("Regular"));
	register_property<TLLineEdit, int>("base_font_size", &TLLineEdit::set_base_font_size, &TLLineEdit::get_base_font_size, 12);

	register_property<TLLineEdit, int>("text_direction", &TLLineEdit::set_text_direction, &TLLineEdit::get_text_direction, TEXT_DIRECTION_AUTO, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM, String("LTR,RTL,Locale,Auto"));
	register_property<TLLineEdit, String>("ot_features", &TLLineEdit::set_ot_features, &TLLineEdit::get_ot_features, String());
	register_property<TLLineEdit, String>("language", &TLLineEdit::set_language, &TLLineEdit::get_language, String());

	register_property<TLLineEdit, int>("max_length", &TLLineEdit::set_max_length, &TLLineEdit::get_max_length, 0);
	register_property<TLLineEdit, bool>("editable", &TLLineEdit::set_editable, &TLLineEdit::is_editable, true);
	register_property<TLLineEdit, bool>("secret", &TLLineEdit::set_secret, &TLLineEdit::is_secret, false);

	register_property<TLLineEdit, String>("secret_character", &TLLineEdit::set_secret_character, &TLLineEdit::get_secret_character, String());
	register_property<TLLineEdit, bool>("expand_to_text_length", &TLLineEdit::set_expand_to_text_length, &TLLineEdit::get_expand_to_text_length, false);

	register_property<TLLineEdit, bool>("context_menu_enabled", &TLLineEdit::set_context_menu_enabled, &TLLineEdit::is_context_menu_enabled, true);
	register_property<TLLineEdit, bool>("clear_button_enabled", &TLLineEdit::set_clear_button_enabled, &TLLineEdit::is_clear_button_enabled, false);

	register_property<TLLineEdit, String>("placeholder_text", &TLLineEdit::set_placeholder, &TLLineEdit::get_placeholder, String());
	register_property<TLLineEdit, float>("placeholder_alpha", &TLLineEdit::set_placeholder_alpha, &TLLineEdit::get_placeholder_alpha, 0.6, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RANGE, String("0,1,0.001"));

	register_property<TLLineEdit, bool>("caret_blink", &TLLineEdit::cursor_set_blink_enabled, &TLLineEdit::cursor_get_blink_enabled, false);
	register_property<TLLineEdit, float>("caret_blink_speed", &TLLineEdit::cursor_set_blink_speed, &TLLineEdit::cursor_get_blink_speed, 0.65, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RANGE, String("0,10,0.001"));
	register_property<TLLineEdit, int>("caret_position", &TLLineEdit::set_cursor_position, &TLLineEdit::get_cursor_position, 0);

	register_method("_notification", &TLLineEdit::_notification);

	register_method("_get_property_list", &TLLineEdit::_get_property_list);
	register_method("_get", &TLLineEdit::_get);
	register_method("_set", &TLLineEdit::_set);
}

#endif

TLLineEdit::TLLineEdit() {
#ifdef GODOT_MODULE
	_init();
#endif
}

void TLLineEdit::_init() {
	line.instance();
	line->connect("string_changed", this, "_font_changed");

	base_direction = TEXT_DIRECTION_AUTO;
	last_input_direction = TEXT_DIRECTION_AUTO;
	ot_features = "";
	language = "";

	undo_stack_pos = undo_stack.end();
	_create_undo_state();
	align = ALIGN_LEFT;

	cursor_pos = 0;
	window_pos = 0;
	window_has_focus = true;
	max_length = 0;
	pass = false;
	secret_character = "*";
	text_changed_dirty = false;
	placeholder_alpha = 0.6;
	clear_button_enabled = false;
	clear_button_status.press_attempt = false;
	clear_button_status.pressing_inside = false;

	deselect();
	set_focus_mode(FOCUS_ALL);
	editable = true;
	set_default_cursor_shape(CURSOR_IBEAM);
	set_mouse_filter(MOUSE_FILTER_STOP);

	draw_caret = true;
	caret_blink_enabled = false;
	caret_blink_timer = memnew(Timer);
	add_child(caret_blink_timer);
	caret_blink_timer->set_wait_time(0.65);
	caret_blink_timer->connect("timeout", this, "_toggle_draw_caret");
	cursor_set_blink_enabled(false);

	context_menu_enabled = true;
	menu = memnew(PopupMenu);
	add_child(menu);
	menu->add_item(TranslationServer::get_singleton()->translate("Cut"), MENU_CUT, GLOBAL_CONST(KEY_MASK_CMD) | GLOBAL_CONST(KEY_X));
	menu->add_item(TranslationServer::get_singleton()->translate("Copy"), MENU_COPY, GLOBAL_CONST(KEY_MASK_CMD) | GLOBAL_CONST(KEY_C));
	menu->add_item(TranslationServer::get_singleton()->translate("Paste"), MENU_PASTE, GLOBAL_CONST(KEY_MASK_CMD) | GLOBAL_CONST(KEY_V));
	menu->add_separator();
	menu->add_item(TranslationServer::get_singleton()->translate("Select All"), MENU_SELECT_ALL, GLOBAL_CONST(KEY_MASK_CMD) | GLOBAL_CONST(KEY_A));
	menu->add_item(TranslationServer::get_singleton()->translate("Clear"), MENU_CLEAR);
	menu->add_separator();
	menu->add_item(TranslationServer::get_singleton()->translate("Undo"), MENU_UNDO, GLOBAL_CONST(KEY_MASK_CMD) | GLOBAL_CONST(KEY_Z));
	menu->add_item(TranslationServer::get_singleton()->translate("Redo"), MENU_REDO, GLOBAL_CONST(KEY_MASK_CMD) | GLOBAL_CONST(KEY_MASK_SHIFT) | GLOBAL_CONST(KEY_Z));
	menu->connect("id_pressed", this, "menu_option");
	expand_to_text_length = false;
}
