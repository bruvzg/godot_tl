/*************************************************************************/
/*  tl_shaped_attributed_string.hpp                                      */
/*************************************************************************/

#ifndef TL_SHAPED_ATTRIBUTED_STRING_HPP
#define TL_SHAPED_ATTRIBUTED_STRING_HPP

#include "tl_shaped_string.hpp"

#ifdef GODOT_MODULE
#include "core/map.h"
#else
#include "Map.hpp"
#endif

enum TextAttribute {

	TEXT_ATTRIBUTE_FONT = 1, //Ref<TLFontFamily>
	TEXT_ATTRIBUTE_FONT_STYLE = 2, //String
	TEXT_ATTRIBUTE_FONT_SIZE = 3, //int
	TEXT_ATTRIBUTE_FONT_FEATURES = 4, //String - 4 letter tag comma separated list
	TEXT_ATTRIBUTE_LANGUAGE = 5, //String - 4 letter tag + css mode
	TEXT_ATTRIBUTE_REPLACEMENT_IMAGE = 6, //Ref<Texture2D>
	TEXT_ATTRIBUTE_REPLACEMENT_RECT = 7, //Size2
	TEXT_ATTRIBUTE_REPLACEMENT_ID = 8, //Variant
	TEXT_ATTRIBUTE_REPLACEMENT_VALIGN = 9, //int (TextVAlign)

	TEXT_ATTRIBUTE_MAX_FORMAT_ATTRIBUTE = 30,

	TEXT_ATTRIBUTE_COLOR = 31, //Color
	TEXT_ATTRIBUTE_OUTLINE_COLOR = 32, //Color

	TEXT_ATTRIBUTE_UNDERLINE_COLOR = 41, //Color
	TEXT_ATTRIBUTE_UNDERLINE_WIDTH = 42, //int
	TEXT_ATTRIBUTE_STRIKETHROUGH_COLOR = 51, //Color
	TEXT_ATTRIBUTE_STRIKETHROUGH_WIDTH = 52, //int
	TEXT_ATTRIBUTE_OVERLINE_COLOR = 61, //Color
	TEXT_ATTRIBUTE_OVERLINE_WIDTH = 62, //int

	TEXT_ATTRIBUTE_HIGHLIGHT_COLOR = 71, //Color

	TEXT_ATTRIBUTE_META = 100 //Variant
};

enum TextVAlign {

	TEXT_VALIGN_TOP = 0,
	TEXT_VALIGN_CENTER = 1,
	TEXT_VALIGN_BOTTOM = 2
};

class TLShapedAttributedString : public TLShapedString {
	GODOT_SUBCLASS(TLShapedAttributedString, TLShapedString);

	TextAttribute _edited_attrib = TEXT_ATTRIBUTE_COLOR;
	Variant _edited_attrib_value;
	int64_t _edited_attrib_start = 0;
	int64_t _edited_attrib_end = 0;

protected:
	enum {
		_CLUSTER_TYPE_IMAGE = 11, //Embedded image
		_CLUSTER_TYPE_RECT = 12 //Reserved rect for embedded object
	};

	Map<int, Map<TextAttribute, Variant>> format_attributes;
	Map<int, Map<TextAttribute, Variant>> style_attributes;

	virtual void _disconnect_fonts();
	virtual void _reconnect_fonts();

	virtual bool _compare_attributes(const Map<TextAttribute, Variant> &p_first, const Map<TextAttribute, Variant> &p_second) const;
	virtual void _ensure_break(int64_t p_key, Map<int, Map<TextAttribute, Variant>> &p_attributes);
	virtual void _optimize_attributes(Map<int, Map<TextAttribute, Variant>> &p_attributes);

	void _shape_substring(TLShapedAttributedString *p_ref, int64_t p_start, int64_t p_end, int p_trim) const;

	virtual void _generate_justification_opportunies(int32_t p_start, int32_t p_end, const char *p_lang, /*out*/ std::vector<JustificationOpportunity> &p_ops) const override;
	virtual void _generate_break_opportunies(int32_t p_start, int32_t p_end, const char *p_lang, /*out*/ std::vector<BreakOpportunity> &p_ops) const override;

	virtual void _shape_single_cluster(int64_t p_start, int64_t p_end, hb_direction_t p_run_direction, hb_script_t p_run_script, UChar32 p_codepoint, TLFontFallbackIterator p_font, /*out*/ Cluster &p_cluster, bool p_font_override = false) const override;
	virtual void _shape_bidi_script_attrib_run(hb_direction_t p_run_direction, hb_script_t p_run_script, const Map<TextAttribute, Variant> &p_attribs, int32_t p_run_start, int32_t p_run_end, TLFontFallbackIterator p_font, bool p_font_override = false);
	virtual void _shape_bidi_script_run(hb_direction_t p_run_direction, hb_script_t p_run_script, int32_t p_run_start, int32_t p_run_end, TLFontFallbackIterator p_font) override;
	virtual void _shape_rect_run(hb_direction_t p_run_direction, const Size2 &p_size, TextVAlign p_align, int32_t p_run_start, int32_t p_run_end);
	virtual void _shape_image_run(hb_direction_t p_run_direction, const Ref<Texture2D> &p_image, TextVAlign p_align, int32_t p_run_start, int32_t p_run_end);

public:
	virtual float get_cluster_face_size(int64_t p_index) const override;

	virtual Ref<TLShapedString> substr(int64_t p_start, int64_t p_end, int p_trim) const override;

	virtual void add_sstring(Ref<TLShapedString> p_text) override;
	virtual void replace_sstring(int64_t p_start, int64_t p_end, Ref<TLShapedString> p_text) override;

	virtual void replace_text(int64_t p_start, int64_t p_end, const String p_text) override;
	virtual void replace_utf8(int64_t p_start, int64_t p_end, const PackedByteArray p_text) override;
	virtual void replace_utf16(int64_t p_start, int64_t p_end, const PackedByteArray p_text) override;
	virtual void replace_utf32(int64_t p_start, int64_t p_end, const PackedByteArray p_text) override;

	virtual void add_attribute(int64_t p_attribute, Variant p_value, int64_t p_start, int64_t p_end);
	virtual void remove_attribute(int64_t p_attribute, int64_t p_start, int64_t p_end);
	virtual void remove_attributes(int64_t p_start, int64_t p_end);
	virtual void clear_attributes();
	virtual bool has_attribute(int64_t p_attribute, int64_t p_index) const;
	virtual Variant get_attribute(int64_t p_attribute, int64_t p_index) const;
	virtual int64_t get_attribute_start(int64_t p_attribute, int64_t p_index) const;
	virtual int64_t get_attribute_end(int64_t p_attribute, int64_t p_index) const;

	virtual void commit_attribute();
	virtual void reject_attribute();

	virtual void load_attributes_dict(Array p_array);
	virtual Array save_attributes_dict() const;

	virtual Vector2 draw_cluster(RID p_canvas_item, const Point2 p_position, int64_t p_index, const Color p_modulate = Color(1, 1, 1)) override;
	virtual void draw(RID p_canvas_item, const Point2 p_position, const Color p_modulate = Color(1, 1, 1)) override;

	Array get_embedded_rects();

#ifdef GODOT_MODULE
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

	static void _bind_methods();
#else
	bool _set(String p_name, Variant p_value);
	Variant _get(String p_name) const;
	Array _get_property_list() const;

	static void _register_methods();
#endif
};

#ifdef GODOT_MODULE
VARIANT_ENUM_CAST(TextAttribute);
VARIANT_ENUM_CAST(TextVAlign);
#endif

#endif /*TL_SHAPED_ATTRIBUTED_STRING_HPP*/
