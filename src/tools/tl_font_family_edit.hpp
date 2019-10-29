/*************************************************************************/
/*  tl_font_family_edit.hpp                                              */
/*************************************************************************/

#ifndef TL_FONT_FAMILY_EDIT_HPP
#define TL_FONT_FAMILY_EDIT_HPP

#include "resources/tl_font_family.hpp"

#include "editor/editor_node.h"
#include "editor/editor_plugin.h"

/*************************************************************************/

class ButtonAddStyle : public Button {
	GDCLASS(ButtonAddStyle, Button);

	Ref<TLFontFamily> ff;
	LineEdit *ctl;

protected:
	virtual void pressed() {
		if (ff.is_valid() && ctl && ctl->get_text() != String()) {
			ff->add_style(ctl->get_text());
		}
	}

public:
	void set_ff(const Ref<TLFontFamily> &p_ff) { ff = p_ff; };
	void set_ctl(LineEdit *p_clt) { ctl = p_clt; };

	ButtonAddStyle() {
		ctl = NULL;
	}
};

/*************************************************************************/

class ButtonDelStyle : public Button {
	GDCLASS(ButtonDelStyle, Button);

	Ref<TLFontFamily> ff;
	String sname;

protected:
	virtual void pressed() {
		if (ff.is_valid()) {
			ff->remove_style(sname);
		}
	}

public:
	void set_ff(const Ref<TLFontFamily> &p_ff) { ff = p_ff; };
	void set_sname(const String &p_sname) { sname = p_sname; };
};

/*************************************************************************/

class ButtonAddScript : public Button {
	GDCLASS(ButtonAddScript, Button);

	Ref<TLFontFamily> ff;
	String sname;
	LineEdit *ctl;

protected:
	virtual void pressed() {
		if (ff.is_valid() && ctl && ctl->get_text() != String()) {
			ff->add_script(sname, ctl->get_text());
		}
	}

public:
	void set_ff(const Ref<TLFontFamily> &p_ff) { ff = p_ff; };
	void set_sname(const String &p_sname) { sname = p_sname; };
	void set_ctl(LineEdit *p_clt) { ctl = p_clt; };

	ButtonAddScript() {
		ctl = NULL;
	}
};

/*************************************************************************/

class ButtonDelScript : public Button {
	GDCLASS(ButtonDelScript, Button);

	Ref<TLFontFamily> ff;
	String sname;
	String scr;

protected:
	virtual void pressed() {
		if (ff.is_valid()) {
			ff->remove_script(sname, scr);
		}
	}

public:
	void set_ff(const Ref<TLFontFamily> &p_ff) { ff = p_ff; };
	void set_sname(const String &p_sname) { sname = p_sname; };
	void set_scr(const String &p_scr) { scr = p_scr; };
};

/*************************************************************************/

class ButtonAddLang : public Button {
	GDCLASS(ButtonAddLang, Button);

	Ref<TLFontFamily> ff;
	String sname;
	LineEdit *ctl;

protected:
	virtual void pressed() {
		if (ff.is_valid() && ctl && ctl->get_text() != String()) {
			ff->add_language(sname, ctl->get_text());
		}
	}

public:
	void set_ff(const Ref<TLFontFamily> &p_ff) { ff = p_ff; };
	void set_sname(const String &p_sname) { sname = p_sname; };
	void set_ctl(LineEdit *p_clt) { ctl = p_clt; };

	ButtonAddLang() {
		ctl = NULL;
	}
};

/*************************************************************************/

class ButtonDelLang : public Button {
	GDCLASS(ButtonDelLang, Button);

	Ref<TLFontFamily> ff;
	String sname;
	String lang;

protected:
	virtual void pressed() {
		if (ff.is_valid()) {
			ff->remove_language(sname, lang);
		}
	}

public:
	void set_ff(const Ref<TLFontFamily> &p_ff) { ff = p_ff; };
	void set_sname(const String &p_sname) { sname = p_sname; };
	void set_lang(const String &p_lang) { lang = p_lang; };
};

/*************************************************************************/

class EditorInspectorPluginTLFontFamily : public EditorInspectorPlugin {
	GDCLASS(EditorInspectorPluginTLFontFamily, EditorInspectorPlugin);

public:
	virtual bool can_handle(Object *p_object);
	virtual void parse_begin(Object *p_object);
	virtual bool parse_property(Object *p_object, Variant::Type p_type, const String &p_path, PropertyHint p_hint, const String &p_hint_text, int p_usage);
	virtual void parse_end();
};

#endif /*TL_FONT_FAMILY_EDIT_HPP*/
