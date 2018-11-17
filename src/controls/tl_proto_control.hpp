/*************************************************************************/
/*  tl_proto_control.hpp                                                 */
/*************************************************************************/

#ifndef TL_PROTO_CONTROL_HPP
#define TL_PROTO_CONTROL_HPP

#include "resources/tl_dynamic_font.hpp"
#include "resources/tl_font_family.hpp"
#include "resources/tl_shaped_attributed_string.hpp"
#include "resources/tl_shaped_paragraph.hpp"

#ifdef GODOT_MODULE
#include "scene/gui/control.h"
#else
#include <Control.hpp>
#include <InputEvent.hpp>
#endif

using namespace godot;

class TLProtoControl;

class TLProtoControlSelection : public Reference {
	GODOT_CLASS(TLProtoControlSelection, Reference);

	friend class TLProtoControl;

protected:
	struct Cursor {
		int64_t p;
		int64_t o;

		Cursor() {
			p = 0;
			o = 0;
		}

		Cursor(int64_t p_para, int64_t p_offset) {
			p = p_para;
			o = p_offset;
		}

		bool operator==(const Cursor &p_b) const {
			return (p == p_b.p) && (o == p_b.o);
		}

		bool operator!=(const Cursor &p_b) const {
			return (p != p_b.p) || (o != p_b.o);
		}

		bool operator<(const Cursor &p_b) const {
			return (p == p_b.p) ? (o < p_b.o) : (p < p_b.p);
		}

		bool operator>(const Cursor &p_b) const {
			return (p == p_b.p) ? (o > p_b.o) : (p > p_b.p);
		}
	};

	Cursor start;
	Cursor end;
	Cursor caret;

public:
	TLProtoControlSelection();
	~TLProtoControlSelection();

	void _init();

	int get_caret_para();
	void set_caret_para(int p_value);

	int get_caret_offset();
	void set_caret_offset(int p_value);

	int get_start_para();
	void set_start_para(int p_value);

	int get_start_offset();
	void set_start_offset(int p_value);

	int get_end_para();
	void set_end_para(int p_value);

	int get_end_offset();
	void set_end_offset(int p_value);

#ifdef GODOT_MODULE
	static void _bind_methods();
#else
	static void _register_methods();
#endif
};

class TLProtoControl : public Control {
	GODOT_CLASS(TLProtoControl, Control);

protected:
	Color cursor_color;
	Color selection_color;
	Color ime_selection_color;
	Color bg_color;

	bool readonly;
	bool in_focus;
	bool selectable;

	std::vector<Ref<TLShapedParagraph> > paragraphs;
	Ref<TLShapedParagraph> ime_temp_para;

	void _fix_selection();
	void _update_ctx_rect();
	float _draw_paragraph(Ref<TLShapedParagraph> p_para, int p_index, float p_offset);
	Size2 get_minimum_size() const;

	enum Margin {
		MARGIN_LEFT,
		MARGIN_RIGHT,
		MARGIN_TOP,
		MARGIN_BOTTOM
	};

	float margin[4];
	Size2 content_size;

	enum AdvDir {
		FWD_CHAR = 0,
		REV_CHAR = 1,
		FWD_WORD = 2,
		REV_WORD = 3,
		FWD_LINE = 4,
		REV_LINE = 5,
		PARA_END = 6,
		PARA_START = 7,
		LINE_END = 8,
		LINE_START = 9,
		TEXT_END = 10,
		TEXT_START = 11
	};

	void _change_selection(bool p_shift, AdvDir p_dir, TextDirection p_last_imp_dir);
	void _hit_test(Point2 p_position, bool p_shift);

	Ref<TLProtoControlSelection> selection;
	Point2 caret_pos;
	uint64_t ime_cursor;
	uint64_t ime_selection_len;
	uint64_t ime_len;
	TextDirection last_inp_dir;

	float para_spacing;

public:
	TLProtoControl();
	~TLProtoControl();

	void _init();

	void clear();

	int get_paragraphs();
	Ref<TLShapedParagraph> get_paragraph(int p_index) const;
	void set_paragraph(Ref<TLShapedParagraph> p_para, int p_index);

	int insert_paragraph(Ref<TLShapedParagraph> p_para, int p_index);
	void remove_paragraph(int p_index);

	Point2 get_caret_position();

	void replace_text(Ref<TLProtoControlSelection> p_selection, const String p_text);
	void replace_utf8(Ref<TLProtoControlSelection> p_selection, const PoolByteArray p_text);
	void replace_utf16(Ref<TLProtoControlSelection> p_selection, const PoolByteArray p_text);
	void replace_utf32(Ref<TLProtoControlSelection> p_selection, const PoolByteArray p_text);
	void replace_sstring(Ref<TLProtoControlSelection> p_selection, Ref<TLShapedString> p_text);

	void add_attribute(Ref<TLProtoControlSelection> p_selection, int p_attribute, Variant p_value);
	void remove_attribute(Ref<TLProtoControlSelection> p_selection, int p_attribute);
	void remove_attributes(Ref<TLProtoControlSelection> p_selection);
	void set_paragraph_width(Ref<TLProtoControlSelection> p_selection, float p_width);
	void set_paragraph_indent(Ref<TLProtoControlSelection> p_selection, float p_indent);
	void set_paragraph_back_color(Ref<TLProtoControlSelection> p_selection, Color p_bcolor);
	void set_paragraph_line_spacing(Ref<TLProtoControlSelection> p_selection, float p_line_spacing);
	void set_paragraph_brk_flags(Ref<TLProtoControlSelection> p_selection, int p_flags);
	void set_paragraph_jst_flags(Ref<TLProtoControlSelection> p_selection, int p_flags);
	void set_paragraph_halign(Ref<TLProtoControlSelection> p_selection, int p_halign);

	Ref<TLProtoControlSelection> get_selection() const;
	void set_selection(Ref<TLProtoControlSelection> p_selection);

	void set_back_color(Color p_color);
	Color get_back_color() const;

	void set_paragraph_spacing(float p_value);
	float get_paragraph_spacing() const;

	void set_readonly(bool p_value);
	bool get_readonly() const;

	void set_selectable(bool p_value);
	bool get_selectable() const;

#ifdef GODOT_MODULE
	void _gui_input(const Ref<InputEvent> &p_event);
#else
	void _gui_input(InputEvent *p_event);
#endif
	void _notification(int p_what);

#ifdef GODOT_MODULE
	static void _bind_methods();
#else
	static void _register_methods();
#endif
};

#endif /*TL_PROTO_CONTROL_HPP*/
