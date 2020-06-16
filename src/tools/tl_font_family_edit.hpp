/*************************************************************************/
/*  tl_font_family_edit.hpp                                              */
/*************************************************************************/

#ifndef TL_FONT_FAMILY_EDIT_HPP
#define TL_FONT_FAMILY_EDIT_HPP

#include "resources/tl_font_family.hpp"
#include "resources/tl_shaped_attributed_string.hpp"
#include "resources/tl_shaped_string.hpp"

#include "editor/editor_node.h"
#include "editor/editor_plugin.h"
#include "editor/editor_scale.h"

/*************************************************************************/

class TLFontFamilyPreview : public VBoxContainer {
	GDCLASS(TLFontFamilyPreview, VBoxContainer);

	Control *preview;
	Ref<TLShapedString> str;
	LineEdit *ctl;

	void _ff_changed(const String &p_text);
	void _redraw();

protected:
	static void _bind_methods();

public:
	void set_ff(const Ref<TLFontFamily> &p_ff, const String &p_style);

	TLFontFamilyPreview();
};

/*************************************************************************/

class EditorInspectorPluginTLFontFamily : public EditorInspectorPlugin {
	GDCLASS(EditorInspectorPluginTLFontFamily, EditorInspectorPlugin);

	void _new_style(Object *p_object, Object *p_ctl);
	void _remove_style(Object *p_object, String p_style);
	void _new_lang(Object *p_object, String p_style, Object *p_ctl);
	void _new_script(Object *p_object, String p_style, Object *p_ctl);
	void _remove_lang(Object *p_object, String p_style, String p_lang);
	void _remove_script(Object *p_object, String p_style, String p_script);
	void _clear(Object *p_object);
	void _commit(Object *p_object);
	void _reject(Object *p_object);

protected:
	static void _bind_methods();

public:
	virtual bool can_handle(Object *p_object);
	virtual void parse_begin(Object *p_object);
	virtual bool parse_property(Object *p_object, Variant::Type p_type, const String &p_path, PropertyHint p_hint, const String &p_hint_text, int p_usage, bool p_wide = false);
	virtual void parse_end();
};

#endif /*TL_FONT_FAMILY_EDIT_HPP*/
