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
		new_btn->set_ff(ff);
		new_btn->set_ctl(new_name);
		new_btn->set_text("Add style");
		hbox->add_child(new_btn);
		add_custom_control(hbox);
		return true;
	}
	Vector<String> tokens = p_path.split("/");
	if (tokens.size() == 2) {
		if (tokens[1] == "_remove_style") {
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
			new_name->set_placeholder("ISO script code");
			hbox->add_child(new_name);
			ButtonAddLang *new_btn = memnew(ButtonAddLang);
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
			new_name->set_placeholder("ISO language code");
			hbox->add_child(new_name);
			ButtonAddScript *new_btn = memnew(ButtonAddScript);
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
}
