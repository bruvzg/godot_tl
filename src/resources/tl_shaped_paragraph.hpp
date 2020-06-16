/*************************************************************************/
/*  tl_shaped_paragraph.hpp                                              */
/*************************************************************************/

#ifndef TL_SHAPED_PARAGRAPH_HPP
#define TL_SHAPED_PARAGRAPH_HPP

#include "resources/tl_shaped_attributed_string.hpp"

using namespace godot;

enum ParaHAlign {

	PARA_HALIGN_LEFT = 0,
	PARA_HALIGN_CENTER = 1,
	PARA_HALIGN_RIGHT = 2,
	PARA_HALIGN_FILL = 3
};

class TLShapedParagraph : public Resource {
	GODOT_CLASS(TLShapedParagraph, Resource);

protected:
	//persistent data
	Ref<TLShapedAttributedString> ctx;
	float width;

	TextBreak brk_flags;
	TextJustification jst_flags;
	ParaHAlign halign;
	Color back_color;
	float line_spacing;
	float indent;

	//temp data
	float max_line_width;
	float height;

	std::vector<int> word_bounds;
	std::vector<int> line_bounds;
	std::vector<Ref<TLShapedAttributedString> > line_ctx;

public:
	TLShapedParagraph();
	virtual ~TLShapedParagraph();

	void _line_change_warning();
	void _update_paragraph();

	void _init();

	void copy_properties(Ref<TLShapedParagraph> p_source);

	int get_lines() const;
	Ref<TLShapedAttributedString> get_line(int p_index) const; //NOTE: read only, changes to line will be discarded on paragraph update!

	void set_width(float p_width);
	float get_width() const;

	void set_indent(float p_indent);
	float get_indent() const;

	void set_back_color(Color p_bcolor);
	Color get_back_color() const;

	void set_line_spacing(float p_line_spacing);
	float get_line_spacing() const;

	void set_string(Ref<TLShapedAttributedString> p_string);
	Ref<TLShapedAttributedString> get_string() const;

	void set_brk_flags(int p_flags);
	int get_brk_flags() const;

	void set_jst_flags(int p_flags);
	int get_jst_flags() const;

	void set_halign(int p_haligh);
	int get_halign() const;

	Size2 get_size() const;
	std::vector<int> get_word_bounds() const;
	std::vector<int> get_line_bounds() const;

	Array _get_word_bounds() const;
	Array _get_line_bounds() const;

#ifdef GODOT_MODULE
	static void _bind_methods();
#else
	bool _set(String p_name, Variant p_value);
	Variant _get(String p_name) const;
	Array _get_property_list() const;
	static void _register_methods();
#endif
};

#ifdef GODOT_MODULE
VARIANT_ENUM_CAST(ParaHAlign);
#endif

#endif /*TL_SHAPED_PARAGRAPH_HPP*/
