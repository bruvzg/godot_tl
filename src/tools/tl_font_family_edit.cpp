/*************************************************************************/
/*  tl_font_family_edit.cpp                                              */
/*************************************************************************/

#include "tl_font_family_edit.hpp"

void EditorInspectorPluginTLFontFamily::_new_style(Object *p_object, Object *p_ctl) {
	Ref<TLFontFamily> object = Ref<TLFontFamily>(Object::cast_to<TLFontFamily>(p_object));
	LineEdit *ctl = Object::cast_to<LineEdit>(p_ctl);
	if (object.is_valid() && ctl && ctl->get_text() != "")
		object->add_style(ctl->get_text());
}

void EditorInspectorPluginTLFontFamily::_remove_style(Object *p_object, String p_style) {
	Ref<TLFontFamily> object = Ref<TLFontFamily>(Object::cast_to<TLFontFamily>(p_object));
	if (object.is_valid())
		object->remove_style(p_style);
}

void EditorInspectorPluginTLFontFamily::_new_lang(Object *p_object, String p_style, Object *p_ctl) {
	Ref<TLFontFamily> object = Ref<TLFontFamily>(Object::cast_to<TLFontFamily>(p_object));
	LineEdit *ctl = Object::cast_to<LineEdit>(p_ctl);
	if (object.is_valid() && ctl && ctl->get_text() != "")
		object->add_language(p_style, ctl->get_text());
}

void EditorInspectorPluginTLFontFamily::_new_script(Object *p_object, String p_style, Object *p_ctl) {
	Ref<TLFontFamily> object = Ref<TLFontFamily>(Object::cast_to<TLFontFamily>(p_object));
	LineEdit *ctl = Object::cast_to<LineEdit>(p_ctl);
	if (object.is_valid() && ctl && ctl->get_text() != "")
		object->add_script(p_style, ctl->get_text());
}

void EditorInspectorPluginTLFontFamily::_remove_lang(Object *p_object, String p_style, String p_lang) {
	Ref<TLFontFamily> object = Ref<TLFontFamily>(Object::cast_to<TLFontFamily>(p_object));
	if (object.is_valid())
		object->remove_language(p_style, p_lang);
}

void EditorInspectorPluginTLFontFamily::_remove_script(Object *p_object, String p_style, String p_script) {
	Ref<TLFontFamily> object = Ref<TLFontFamily>(Object::cast_to<TLFontFamily>(p_object));
	if (object.is_valid())
		object->remove_script(p_style, p_script);
}

void EditorInspectorPluginTLFontFamily::_clear(Object *p_object) {
	Ref<TLShapedAttributedString> object = Ref<TLShapedAttributedString>(Object::cast_to<TLShapedAttributedString>(p_object));
	if (object.is_valid())
		object->clear_attributes();
}

void EditorInspectorPluginTLFontFamily::_commit(Object *p_object) {
	Ref<TLShapedAttributedString> object = Ref<TLShapedAttributedString>(Object::cast_to<TLShapedAttributedString>(p_object));
	if (object.is_valid())
		object->commit_attribute();
}

void EditorInspectorPluginTLFontFamily::_reject(Object *p_object) {
	Ref<TLShapedAttributedString> object = Ref<TLShapedAttributedString>(Object::cast_to<TLShapedAttributedString>(p_object));
	if (object.is_valid())
		object->reject_attribute();
}

void EditorInspectorPluginTLFontFamily::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_new_style", "object", "ctl"), &EditorInspectorPluginTLFontFamily::_new_style);
	ClassDB::bind_method(D_METHOD("_remove_style", "object", "style"), &EditorInspectorPluginTLFontFamily::_remove_style);
	ClassDB::bind_method(D_METHOD("_new_lang", "object", "style", "ctl"), &EditorInspectorPluginTLFontFamily::_new_lang);
	ClassDB::bind_method(D_METHOD("_new_script", "object", "style", "ctl"), &EditorInspectorPluginTLFontFamily::_new_script);
	ClassDB::bind_method(D_METHOD("_remove_lang", "object", "style", "lang"), &EditorInspectorPluginTLFontFamily::_remove_lang);
	ClassDB::bind_method(D_METHOD("_remove_script", "object", "style", "script"), &EditorInspectorPluginTLFontFamily::_remove_script);
	ClassDB::bind_method(D_METHOD("_clear"), &EditorInspectorPluginTLFontFamily::_clear);
	ClassDB::bind_method(D_METHOD("_commit"), &EditorInspectorPluginTLFontFamily::_commit);
	ClassDB::bind_method(D_METHOD("_reject"), &EditorInspectorPluginTLFontFamily::_reject);
}

bool EditorInspectorPluginTLFontFamily::can_handle(Object *p_object) {
	return (Object::cast_to<TLFontFamily>(p_object) != NULL) || (Object::cast_to<TLShapedAttributedString>(p_object) != NULL);
}

void EditorInspectorPluginTLFontFamily::parse_begin(Object *p_object) {
	//NOP
}

bool EditorInspectorPluginTLFontFamily::parse_property(Object *p_object, Variant::Type p_type, const String &p_path, PropertyHint p_hint, const String &p_hint_text, int p_usage) {
	if (p_path == "attribute/_commit") {
		HBoxContainer *hbox = memnew(HBoxContainer);
		Button *rem_btn = memnew(Button);
		rem_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		rem_btn->connect("pressed", callable_mp(this, &EditorInspectorPluginTLFontFamily::_clear), varray(p_object));
		rem_btn->set_text("Clear");
		hbox->add_child(rem_btn);
		Button *cln_btn = memnew(Button);
		cln_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		cln_btn->connect("pressed", callable_mp(this, &EditorInspectorPluginTLFontFamily::_reject), varray(p_object));
		cln_btn->set_text("Remove");
		hbox->add_child(cln_btn);
		Button *new_btn = memnew(Button);
		new_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		new_btn->connect("pressed", callable_mp(this, &EditorInspectorPluginTLFontFamily::_commit), varray(p_object));
		new_btn->set_text("Add");
		hbox->add_child(new_btn);
		add_custom_control(hbox);
		return true;
	}
	if (p_path == "_new_style") {
		HBoxContainer *hbox = memnew(HBoxContainer);
		LineEdit *new_name = memnew(LineEdit);
		new_name->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		new_name->set_placeholder("Style name");
		hbox->add_child(new_name);
		Button *new_btn = memnew(Button);
		new_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		new_btn->connect("pressed", callable_mp(this, &EditorInspectorPluginTLFontFamily::_new_style), varray(p_object, new_name));
		new_btn->set_text("Add style");
		hbox->add_child(new_btn);
		add_custom_control(hbox);
		return true;
	}
	Vector<String> tokens = p_path.split("/");
	if (tokens.size() == 2) {
		if (tokens[1] == "_prev_style") {
			TLFontFamilyPreview *prev = memnew(TLFontFamilyPreview);
			Ref<TLFontFamily> object = Ref<TLFontFamily>(Object::cast_to<TLFontFamily>(p_object));
			prev->set_ff(object, tokens[0]);
			add_custom_control(prev);
			return true;
		} else if (tokens[1] == "_remove_style") {
			Button *rem_btn = memnew(Button);
			rem_btn->set_text("Remove \"" + tokens[0] + "\" style");
			rem_btn->connect("pressed", callable_mp(this, &EditorInspectorPluginTLFontFamily::_remove_style), varray(p_object, tokens[0]));
			add_custom_control(rem_btn);
			return true;
		} else if (tokens[1] == "_add_lang") {
			HBoxContainer *hbox = memnew(HBoxContainer);
			LineEdit *new_name = memnew(LineEdit);
			new_name->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			new_name->set_max_length(4);
			new_name->set_placeholder("ISO language code");
			hbox->add_child(new_name);
			Button *new_btn = memnew(Button);
			new_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			new_btn->connect("pressed", callable_mp(this, &EditorInspectorPluginTLFontFamily::_new_lang), varray(p_object, tokens[0], new_name));
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
			Button *new_btn = memnew(Button);
			new_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			new_btn->connect("pressed", callable_mp(this, &EditorInspectorPluginTLFontFamily::_new_script), varray(p_object, tokens[0], new_name));
			new_btn->set_text("Add script");
			hbox->add_child(new_btn);
			add_custom_control(hbox);
			return true;
		}
	} else if (tokens.size() == 4) {
		if (tokens[3] == "_remove_script" && tokens[1] == "script") {
			Button *rem_btn = memnew(Button);
			rem_btn->set_text("Remove \"" + tokens[2] + "\" script");
			rem_btn->connect("pressed", callable_mp(this, &EditorInspectorPluginTLFontFamily::_remove_script), varray(p_object, tokens[0], tokens[2]));
			add_custom_control(rem_btn);
			return true;
		}
		if (tokens[3] == "_remove_lang" && tokens[1] == "lang") {
			Button *rem_btn = memnew(Button);
			rem_btn->set_text("Remove \"" + tokens[2] + "\" language");
			rem_btn->connect("pressed", callable_mp(this, &EditorInspectorPluginTLFontFamily::_remove_lang), varray(p_object, tokens[0], tokens[2]));
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
	RenderingServer::get_singleton()->canvas_item_add_rect(preview->get_canvas_item(), Rect2(Point2(), preview->get_size()), Color(0.8, 0.8, 0.8, 1));
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
	preview->connect("draw", callable_mp(this, &TLFontFamilyPreview::_redraw));
	add_child(preview);
	ctl = memnew(LineEdit);
	ctl->set_text("Etaoin shrdlu");
	ctl->connect("text_changed", callable_mp(this, &TLFontFamilyPreview::_ff_changed));
	add_child(ctl);
}
