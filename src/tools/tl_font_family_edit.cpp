/*************************************************************************/
/*  tl_font_family_edit.cpp                                              */
/*************************************************************************/

#include "tl_font_family_edit.hpp"

bool EditorInspectorPluginTLFontFamily::can_handle(Object *p_object) {

	return Object::cast_to<TLFontFamily>(p_object) != NULL;
}

void EditorInspectorPluginTLFontFamily::parse_begin(Object *p_object) {
	//NOP
}

bool EditorInspectorPluginTLFontFamily::parse_property(Object *p_object, Variant::Type p_type, const String &p_path, PropertyHint p_hint, const String &p_hint_text, int p_usage) {
	Ref<TLFontFamily> ff = Ref<TLFontFamily>(Object::cast_to<TLFontFamily>(p_object));
	if (p_path == "_new_style") {
		HBoxContainer *hbox = memnew(HBoxContainer);
		LineEdit *new_name = memnew(LineEdit);
		new_name->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		new_name->set_placeholder("Style name");
		hbox->add_child(new_name);
		ButtonAddStyle *new_btn = memnew(ButtonAddStyle);
		new_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		new_btn->set_ff(ff);
		new_btn->set_ctl(new_name);
		new_btn->set_text("Add style");
		hbox->add_child(new_btn);
		add_custom_control(hbox);
		return true;
	}
	Vector<String> tokens = p_path.split("/");
	if (tokens.size() == 2) {
		if (tokens[1] == "_prev_style") {
			TLFontFamilyPreview *prev = memnew(TLFontFamilyPreview);
			prev->set_ff(ff, tokens[0]);
			add_custom_control(prev);
			return true;
		} else if (tokens[1] == "_remove_style") {
			ButtonDelStyle *rem_btn = memnew(ButtonDelStyle);
			rem_btn->set_text("Remove \"" + tokens[0] + "\" style");
			rem_btn->set_ff(ff);
			rem_btn->set_sname(tokens[0]);
			add_custom_control(rem_btn);
			return true;
		} else if (tokens[1] == "_add_lang") {
			HBoxContainer *hbox = memnew(HBoxContainer);
			LineEdit *new_name = memnew(LineEdit);
			new_name->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			new_name->set_max_length(4);
			new_name->set_placeholder("ISO language code");
			hbox->add_child(new_name);
			ButtonAddLang *new_btn = memnew(ButtonAddLang);
			new_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			new_btn->set_ff(ff);
			new_btn->set_sname(tokens[0]);
			new_btn->set_ctl(new_name);
			new_btn->set_text("Add language");
			hbox->add_child(new_btn);
			add_custom_control(hbox);
			return true;
		} else if (tokens[1] == "_add_script") {
			HBoxContainer *hbox = memnew(HBoxContainer);
			LineEdit *new_name = memnew(LineEdit);
			new_name->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			new_name->set_max_length(4);
			new_name->set_placeholder("ISO script code");
			hbox->add_child(new_name);
			ButtonAddScript *new_btn = memnew(ButtonAddScript);
			new_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			new_btn->set_ff(ff);
			new_btn->set_sname(tokens[0]);
			new_btn->set_ctl(new_name);
			new_btn->set_text("Add script");
			hbox->add_child(new_btn);
			add_custom_control(hbox);
			return true;
		}
	} else if (tokens.size() == 4) {
		if (tokens[3] == "_remove_script" && tokens[1] == "script") {
			ButtonDelScript *rem_btn = memnew(ButtonDelScript);
			rem_btn->set_text("Remove \"" + tokens[2] + "\" script");
			rem_btn->set_ff(ff);
			rem_btn->set_sname(tokens[0]);
			rem_btn->set_scr(tokens[2]);
			add_custom_control(rem_btn);
			return true;
		}
		if (tokens[3] == "_remove_lang" && tokens[1] == "lang") {
			ButtonDelLang *rem_btn = memnew(ButtonDelLang);
			rem_btn->set_text("Remove \"" + tokens[2] + "\" language");
			rem_btn->set_ff(ff);
			rem_btn->set_sname(tokens[0]);
			rem_btn->set_lang(tokens[2]);
			add_custom_control(rem_btn);
			return true;
		}
	}
	return false; //do not handle
}

void EditorInspectorPluginTLFontFamily::parse_end() {
	//NOP
}

/*************************************************************************/

void TLFontFamilyPreview::_ff_changed(const String &p_text) {
	str->set_text(p_text);
	preview->update();
}

void TLFontFamilyPreview::_redraw() {
	VisualServer::get_singleton()->canvas_item_add_rect(preview->get_canvas_item(), Rect2(Point2(), preview->get_size()), Color(0.8, 0.8, 0.8, 1));
	str->draw(preview->get_canvas_item(), Point2((preview->get_rect().size.x - str->get_width()) / 2, (preview->get_rect().size.y - str->get_height()) / 2 + str->get_ascent()), Color(0, 0, 0, 1));
}

void TLFontFamilyPreview::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_ff_changed", "text"), &TLFontFamilyPreview::_ff_changed);
	ClassDB::bind_method(D_METHOD("_redraw"), &TLFontFamilyPreview::_redraw);
}

void TLFontFamilyPreview::set_ff(const Ref<TLFontFamily> &p_ff, const String &p_style) {
	str->set_base_font(p_ff);
	str->set_base_font_style(p_style);
}

TLFontFamilyPreview::TLFontFamilyPreview() {
	str.instance();
	str->set_base_font_size(18.0);
	str->set_text("Etaoin shrdlu");
	preview = memnew(Control);
	preview->set_custom_minimum_size(Size2(0, 50 * EDSCALE));
	preview->connect("draw", this, "_redraw");
	add_margin_child(TTR("Preview:"), preview);
	ctl = memnew(LineEdit);
	ctl->set_text("Etaoin shrdlu");
	ctl->connect("text_changed", this, "_ff_changed");
	add_child(ctl);
}
