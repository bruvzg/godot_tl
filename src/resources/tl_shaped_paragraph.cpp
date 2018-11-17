/*************************************************************************/
/*  tl_shaped_paragraph.cpp                                              */
/*************************************************************************/

#include "resources/tl_shaped_paragraph.hpp"

TLShapedParagraph::TLShapedParagraph() {

#ifdef GODOT_MODULE
	_init();
#endif
}

TLShapedParagraph::~TLShapedParagraph() {

	for (unsigned int i = 0; i < line_ctx.size(); i++) {
		line_ctx[i]->disconnect("string_changed", this, "_line_change_warning");
	}
}

void TLShapedParagraph::_init() {

#ifdef GODOT_MODULE
	ctx.instance();
#else
	ctx = Ref<TLShapedAttributedString>::__internal_constructor(TLShapedAttributedString::_new());
#endif

	ctx->connect("string_changed", this, "_update_paragraph");

	width = -1.0f;
	max_line_width = 0.0f;
	height = 0.0f;
	brk_flags = TEXT_BREAK_MANDATORY_AND_WORD_BOUND;
	jst_flags = TEXT_JUSTIFICATION_KASHIDA_AND_WHITESPACE;
	halign = PARA_HALIGN_LEFT;
	back_color = Color(1, 1, 1);
	line_spacing = 1.0f;
	indent = 0.0f;

	_update_paragraph();
}

void TLShapedParagraph::copy_properties(Ref<TLShapedParagraph> p_source) {

	ctx->copy_properties(p_source->ctx);

	width = p_source->width;
	brk_flags = p_source->brk_flags;
	jst_flags = p_source->jst_flags;
	halign = p_source->halign;
	back_color = p_source->back_color;
	line_spacing = p_source->line_spacing;
	indent = p_source->indent;

	_update_paragraph();
}

void TLShapedParagraph::_line_change_warning() {

	WARN_PRINTS("DO NOT EDIT")
	_update_paragraph(); //regenerate changed lines
}

void TLShapedParagraph::_update_paragraph() {

	for (unsigned int i = 0; i < line_ctx.size(); i++) {
		line_ctx[i]->disconnect("string_changed", this, "_line_change_warning");
	}
	line_ctx.clear();
	word_bounds = ctx->break_words();
	if (width > 0.0f) {
		line_bounds = ctx->break_lines(width, brk_flags);
	} else {
		line_bounds = ctx->break_lines(-1, TEXT_BREAK_MANDATORY);
	}

	max_line_width = width;
	height = 0.0f;
	if (line_bounds.size() == 0) {
		height = ctx->get_height();
		if (max_line_width < min_paragraph_width)
			max_line_width = min_paragraph_width;
	} else {
		unsigned int prev = 0;
		for (unsigned int i = 0; i < line_bounds.size(); i++) {
			Ref<TLShapedAttributedString> line = ctx->substr(prev, line_bounds[i], TEXT_TRIM_BREAK);
			line->shape();
			line->connect("string_changed", this, "_line_change_warning");
			if ((halign == PARA_HALIGN_FILL) && (width > 0.0f)) {
				line->extend_to_width(width, jst_flags);
			}
			if (max_line_width < line->get_width())
				max_line_width = line->get_width();

			height += line->get_height() * line_spacing;

			line_ctx.push_back(line);
			prev = line_bounds[i];
		}
	}
	emit_signal("paragraph_changed");
}

Size2 TLShapedParagraph::get_size() const {

	return Size2(max_line_width, height);
}

std::vector<int> TLShapedParagraph::get_word_bounds() const {

	return word_bounds;
}

std::vector<int> TLShapedParagraph::get_line_bounds() const {

	return line_bounds;
}

Array TLShapedParagraph::_get_word_bounds() const {

	Array ret;
	for (int i = 0; i < word_bounds.size(); i++) {
		ret.push_back(word_bounds[i]);
	}
	return ret;
}

Array TLShapedParagraph::_get_line_bounds() const {

	Array ret;
	for (int i = 0; i < line_bounds.size(); i++) {
		ret.push_back(line_bounds[i]);
	}
	return ret;
}

void TLShapedParagraph::set_width(float p_width) {

	if (width != p_width) {
		width = p_width;
		_update_paragraph();
	}
}

float TLShapedParagraph::get_width() const {

	return width;
}

void TLShapedParagraph::set_indent(float p_indent) {

	if (indent != p_indent) {
		indent = p_indent;
		_update_paragraph();
	}
}
float TLShapedParagraph::get_indent() const {

	return indent;
}

void TLShapedParagraph::set_back_color(Color p_bcolor) {

	if (back_color != p_bcolor) {
		back_color = p_bcolor;
		_update_paragraph();
	}
}

Color TLShapedParagraph::get_back_color() const {

	return back_color;
}

void TLShapedParagraph::set_line_spacing(float p_line_spacing) {

	if (line_spacing != p_line_spacing) {
		line_spacing = p_line_spacing;
		_update_paragraph();
	}
}

float TLShapedParagraph::get_line_spacing() const {

	return line_spacing;
}

void TLShapedParagraph::set_string(Ref<TLShapedAttributedString> p_string) {

	if (ctx != p_string) {
		ctx->disconnect("string_changed", this, "_update_paragraph");
		ctx = p_string;
		ctx->connect("string_changed", this, "_update_paragraph");
		_update_paragraph();
	}
}

Ref<TLShapedAttributedString> TLShapedParagraph::get_string() const {

	return ctx;
}

void TLShapedParagraph::set_brk_flags(int p_flags) {

	if (brk_flags != p_flags) {
		brk_flags = (TextBreak)p_flags;
		_update_paragraph();
	}
}

int TLShapedParagraph::get_brk_flags() const {

	return brk_flags;
}

void TLShapedParagraph::set_jst_flags(int p_flags) {

	if (jst_flags != p_flags) {
		jst_flags = (TextJustification)p_flags;
		_update_paragraph();
	}
}

int TLShapedParagraph::get_jst_flags() const {

	return jst_flags;
}

void TLShapedParagraph::set_halign(int p_haligh) {

	if (p_haligh != halign) {
		halign = (ParaHAlign)p_haligh;
		_update_paragraph();
	}
}

int TLShapedParagraph::get_halign() const {

	return halign;
}

int TLShapedParagraph::get_lines() const {

	return line_ctx.size();
}

Ref<TLShapedAttributedString> TLShapedParagraph::get_line(int p_index) const {

	if ((p_index < 0) || (p_index >= line_ctx.size()))
		return Ref<TLShapedAttributedString>();

	return line_ctx[p_index];
}

#ifdef GODOT_MODULE

void TLShapedParagraph::_bind_methods() {

	ClassDB::bind_method(D_METHOD("copy_properties", "source"), &TLShapedParagraph::copy_properties);

	ClassDB::bind_method(D_METHOD("_update_paragraph"), &TLShapedParagraph::_update_paragraph);
	ClassDB::bind_method(D_METHOD("_line_change_warning"), &TLShapedParagraph::_line_change_warning);

	ClassDB::bind_method(D_METHOD("get_lines"), &TLShapedParagraph::get_lines);
	ClassDB::bind_method(D_METHOD("get_line", "index"), &TLShapedParagraph::get_line);

	ClassDB::bind_method(D_METHOD("set_width", "width"), &TLShapedParagraph::set_width);
	ClassDB::bind_method(D_METHOD("get_width"), &TLShapedParagraph::get_width);
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "width"), "set_width", "get_width");

	ClassDB::bind_method(D_METHOD("set_indent", "indent"), &TLShapedParagraph::set_indent);
	ClassDB::bind_method(D_METHOD("get_indent"), &TLShapedParagraph::get_indent);
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "indent"), "set_indent", "get_indent");

	ClassDB::bind_method(D_METHOD("set_back_color", "bcolor"), &TLShapedParagraph::set_back_color);
	ClassDB::bind_method(D_METHOD("get_back_color"), &TLShapedParagraph::get_back_color);
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "back_color"), "set_back_color", "get_back_color");

	ClassDB::bind_method(D_METHOD("set_line_spacing", "line_spacing"), &TLShapedParagraph::set_line_spacing);
	ClassDB::bind_method(D_METHOD("get_line_spacing"), &TLShapedParagraph::get_line_spacing);
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "line_spacing"), "set_line_spacing", "get_line_spacing");

	ClassDB::bind_method(D_METHOD("set_string", "string"), &TLShapedParagraph::set_string);
	ClassDB::bind_method(D_METHOD("get_string"), &TLShapedParagraph::get_string);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "string", PROPERTY_HINT_RESOURCE_TYPE, "TLShapedAttributedString"), "set_string", "get_string");

	ClassDB::bind_method(D_METHOD("set_brk_flags", "flags"), &TLShapedParagraph::set_brk_flags);
	ClassDB::bind_method(D_METHOD("get_brk_flags"), &TLShapedParagraph::get_brk_flags);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "brk_flags", PROPERTY_HINT_ENUM, "None,Mandatory,Mandatory and word bounds,Mandatory and cluster bounds"), "set_brk_flags", "get_brk_flags");

	ClassDB::bind_method(D_METHOD("set_jst_flags", "flags"), &TLShapedParagraph::set_jst_flags);
	ClassDB::bind_method(D_METHOD("get_jst_flags"), &TLShapedParagraph::get_jst_flags);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "jst_flags", PROPERTY_HINT_ENUM, "None,Kashida and whitespace,Kashida only,Whitespace only"), "set_jst_flags", "get_jst_flags");

	ClassDB::bind_method(D_METHOD("set_halign", "haligh"), &TLShapedParagraph::set_halign);
	ClassDB::bind_method(D_METHOD("get_halign"), &TLShapedParagraph::get_halign);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "halign", PROPERTY_HINT_ENUM, "Left,Center,Right,Fill"), "set_halign", "get_halign");

	ClassDB::bind_method(D_METHOD("get_size"), &TLShapedParagraph::get_size);
	ClassDB::bind_method(D_METHOD("get_word_bounds"), &TLShapedParagraph::_get_word_bounds);
	ClassDB::bind_method(D_METHOD("get_line_bounds"), &TLShapedParagraph::_get_line_bounds);

	ADD_SIGNAL(MethodInfo("paragraph_changed"));

	BIND_ENUM_CONSTANT(PARA_HALIGN_LEFT);
	BIND_ENUM_CONSTANT(PARA_HALIGN_CENTER);
	BIND_ENUM_CONSTANT(PARA_HALIGN_RIGHT);
	BIND_ENUM_CONSTANT(PARA_HALIGN_FILL);
}

#else

void TLShapedParagraph::_register_methods() {

	register_method("copy_properties", &TLShapedParagraph::copy_properties);

	register_method("_update_paragraph", &TLShapedParagraph::_update_paragraph);
	register_method("_line_change_warning", &TLShapedParagraph::_line_change_warning);

	register_method("get_lines", &TLShapedParagraph::get_lines);
	register_method("get_line", &TLShapedParagraph::get_line);

	register_method("set_width", &TLShapedParagraph::set_width);
	register_method("get_width", &TLShapedParagraph::get_width);
	register_property<TLShapedParagraph, float>("width", &TLShapedParagraph::set_width, &TLShapedParagraph::get_width, -1.0f);

	register_method("set_indent", &TLShapedParagraph::set_indent);
	register_method("get_indent", &TLShapedParagraph::get_indent);
	register_property<TLShapedParagraph, float>("indent", &TLShapedParagraph::set_indent, &TLShapedParagraph::get_indent, 0.0f);

	register_method("set_back_color", &TLShapedParagraph::set_back_color);
	register_method("get_back_color", &TLShapedParagraph::get_back_color);
	register_property<TLShapedParagraph, Color>("back_color", &TLShapedParagraph::set_back_color, &TLShapedParagraph::get_back_color, Color(1, 1, 1));

	register_method("set_line_spacing", &TLShapedParagraph::set_line_spacing);
	register_method("get_line_spacing", &TLShapedParagraph::get_line_spacing);
	register_property<TLShapedParagraph, float>("line_spacing", &TLShapedParagraph::set_line_spacing, &TLShapedParagraph::get_line_spacing, 1.0f);

	register_method("set_string", &TLShapedParagraph::set_string);
	register_method("get_string", &TLShapedParagraph::get_string);
	register_property<TLShapedParagraph, Ref<TLShapedAttributedString> >("string", &TLShapedParagraph::set_string, &TLShapedParagraph::get_string, Ref<TLShapedAttributedString>(), GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RESOURCE_TYPE, String("TLShapedAttributedString"));

	register_method("set_brk_flags", &TLShapedParagraph::set_brk_flags);
	register_method("get_brk_flags", &TLShapedParagraph::get_brk_flags);
	register_property<TLShapedParagraph, int>("brk_flags", &TLShapedParagraph::set_brk_flags, &TLShapedParagraph::get_brk_flags, TEXT_BREAK_NONE, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM, String("None,Mandatory,Mandatory and word bounds,Mandatory and cluster bounds"));

	register_method("set_jst_flags", &TLShapedParagraph::set_jst_flags);
	register_method("get_jst_flags", &TLShapedParagraph::get_jst_flags);
	register_property<TLShapedParagraph, int>("jst_flags", &TLShapedParagraph::set_jst_flags, &TLShapedParagraph::get_jst_flags, TEXT_JUSTIFICATION_NONE, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM, String("None,Kashida and whitespace,Kashida only,Whitespace only"));

	register_method("set_halign", &TLShapedParagraph::set_halign);
	register_method("get_halign", &TLShapedParagraph::get_halign);
	register_property<TLShapedParagraph, int>("halign", &TLShapedParagraph::set_halign, &TLShapedParagraph::get_halign, PARA_HALIGN_LEFT, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM, String("Left,Center,Right,Fill"));

	register_method("get_size", &TLShapedParagraph::get_size);
	register_method("get_word_bounds", &TLShapedParagraph::_get_word_bounds);
	register_method("get_line_bounds", &TLShapedParagraph::_get_line_bounds);

	register_signal<TLShapedParagraph>("paragraph_changed");
}

#endif
