/*************************************************************************/
/*  tl_shaped_string.hpp                                                 */
/*************************************************************************/

#ifndef TL_SHAPED_STRING_HPP
#define TL_SHAPED_STRING_HPP

#include "tl_font.hpp"
#include "tl_font_family.hpp"

#ifdef GODOT_MODULE

#else
#include <Array.hpp>
#endif

#include <algorithm>
#include <cstdlib>
#include <vector>

#include <unicode/ubidi.h>
#include <unicode/ubrk.h>
#include <unicode/uchar.h>
#include <unicode/uclean.h>
#include <unicode/udata.h>
#include <unicode/uiter.h>
#include <unicode/uloc.h>
#include <unicode/uscript.h>
#include <unicode/ustring.h>
#include <unicode/utypes.h>

#include <hb-icu.h>
#include <hb.h>

using namespace godot;

enum TextDirection {

	TEXT_DIRECTION_LTR = 0,
	TEXT_DIRECTION_RTL = 1,
	TEXT_DIRECTION_LOCALE = 2,
	TEXT_DIRECTION_AUTO = 3,
	TEXT_DIRECTION_INVALID = 4
};

enum TextJustification {

	TEXT_JUSTIFICATION_NONE = 0,
	TEXT_JUSTIFICATION_KASHIDA_AND_WHITESPACE = 1,
	TEXT_JUSTIFICATION_KASHIDA_ONLY = 2,
	TEXT_JUSTIFICATION_WHITESPACE_ONLY = 3,
	TEXT_JUSTIFICATION_KASHIDA_AND_WHITESPACE_AND_INTERCHAR = 4,
	TEXT_JUSTIFICATION_KASHIDA_AND_INTERCHAR = 5,
	TEXT_JUSTIFICATION_WHITESPACE_AND_INTERCHAR = 6,
	TEXT_JUSTIFICATION_INTERCHAR_ONLY = 7
};

enum TextBreak {

	TEXT_BREAK_NONE = 0,
	TEXT_BREAK_MANDATORY = 1,
	TEXT_BREAK_MANDATORY_AND_WORD_BOUND = 2,
	TEXT_BREAK_MANDATORY_AND_ANYWHERE = 3
};

enum TextTrimMode {

	TEXT_TRIM_NONE = 0,
	TEXT_TRIM_BREAK = 1,
	TEXT_TRIM_BREAK_AND_WHITESPACE = 2
};

class TLShapedString : public Resource {
	GODOT_CLASS(TLShapedString, Resource);

protected:
	enum {
		_CLUSTER_TYPE_INVALID = 0,
		_CLUSTER_TYPE_HEX_BOX = 1, //Fallback hex box font codepoint
		_CLUSTER_TYPE_TEXT = 2, //Normal glyphs
		_CLUSTER_TYPE_SKIP = 3 //Justification spaces
	};

	struct Glyph {

		UChar32 codepoint;
		Point2 offset;
		Point2 advance;

		Glyph();
		Glyph(uint32_t p_codepoint, Point2 p_offset, Point2 p_advance);
	};

	struct Cluster {

		int cl_type;

		int64_t start;
		int64_t end;

		bool valid;
		bool is_rtl;
		bool ignore_on_input;

		TLFontFace *font_face;

		float offset;
		float ascent;
		float descent;
		float width;

		//debug info
		hb_script_t script;
		hb_direction_t dir;
		hb_language_t lang;
		//debug info

		std::vector<Glyph> glyphs;

		Cluster();
	};

	static bool ClusterCompare(const Cluster &p_a, const Cluster &p_b) {
		return p_a.start < p_b.start;
	};

	class ScriptIterator {

		struct ScriptRange {

			int32_t start;
			int32_t end;
			hb_script_t script;
		};

		static bool same_script(int32_t p_script_one, int32_t p_script_two);

		_ALWAYS_INLINE_ int32_t next_bound(const UChar *p_chars, int32_t p_offset, int32_t p_length) const {

			if (p_offset < 0)
				p_offset = 0;

			if (p_offset >= p_length)
				return p_length;

			return MIN((U16_IS_SURROGATE(p_chars[p_offset]) && U16_IS_SURROGATE_TRAIL(p_chars[p_offset])) ? p_offset + 1 : p_offset, p_length);
		}

		int64_t cur;
		bool is_rtl;
		std::vector<ScriptRange> script_ranges;

	public:
		bool next();

		int32_t get_start() const;
		int32_t get_end() const;
		hb_script_t get_script() const;

		void reset(hb_direction_t p_run_direction);

		ScriptIterator(const UChar *p_chars, int32_t p_start, int32_t p_length);
	};

	struct BreakOpportunity {

		int64_t position;
		bool hard;
	};

	struct JustificationOpportunity {

		int64_t position;
		bool kashida;
	};

	bool valid;

	TextDirection base_direction;
	TextDirection para_direction;
	Ref<TLFontFamily> base_font;
	String base_style;
	int base_size;

	UChar *data;
	std::vector<hb_feature_t> font_features;
	hb_language_t language;

	UBiDi *bidi_iter;
	ScriptIterator *script_iter;

	hb_buffer_t *hb_buffer;
	bool preserve_control;

	int64_t data_size;
	int64_t char_size;

	std::vector<Cluster> visual;

	float ascent;
	float descent;
	float width;

	_ALWAYS_INLINE_ bool is_ain(uint32_t p_chr) const { return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_AIN; }
	_ALWAYS_INLINE_ bool is_alef(uint32_t p_chr) const { return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_ALEF; }
	_ALWAYS_INLINE_ bool is_beh(uint32_t p_chr) const {
		int32_t prop = u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP);
		return (prop == U_JG_BEH) || (prop == U_JG_NOON) || (prop == U_JG_AFRICAN_NOON) || (prop == U_JG_NYA) || (prop == U_JG_YEH) || (prop == U_JG_FARSI_YEH);
	}
	_ALWAYS_INLINE_ bool is_dal(uint32_t p_chr) const { return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_DAL; }
	_ALWAYS_INLINE_ bool is_feh(uint32_t p_chr) const { return (u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_FEH) || (u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_AFRICAN_FEH); }
	_ALWAYS_INLINE_ bool is_gaf(uint32_t p_chr) const { return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_GAF; }
	_ALWAYS_INLINE_ bool is_heh(uint32_t p_chr) const { return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_HEH; }
	_ALWAYS_INLINE_ bool is_kaf(uint32_t p_chr) const { return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_KAF; }
	_ALWAYS_INLINE_ bool is_lam(uint32_t p_chr) const { return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_LAM; }
	_ALWAYS_INLINE_ bool is_qaf(uint32_t p_chr) const { return (u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_QAF) || (u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_AFRICAN_QAF); }
	_ALWAYS_INLINE_ bool is_reh(uint32_t p_chr) const { return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_REH; }
	_ALWAYS_INLINE_ bool is_seen_sad(uint32_t p_chr) const { return (u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_SAD) || (u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_SEEN); }
	_ALWAYS_INLINE_ bool is_tah(uint32_t p_chr) const { return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_TAH; }
	_ALWAYS_INLINE_ bool is_teh_marbuta(uint32_t p_chr) const { return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_TEH_MARBUTA; }
	_ALWAYS_INLINE_ bool is_yeh(uint32_t p_chr) const {
		int32_t prop = u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP);
		return (prop == U_JG_YEH) || (prop == U_JG_FARSI_YEH) || (prop == U_JG_YEH_BARREE) || (prop == U_JG_BURUSHASKI_YEH_BARREE) || (prop == U_JG_YEH_WITH_TAIL);
	}
	_ALWAYS_INLINE_ bool is_waw(uint32_t p_chr) const { return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_WAW; }
	_ALWAYS_INLINE_ bool is_transparent(uint32_t p_chr) const { return u_getIntPropertyValue(p_chr, UCHAR_JOINING_TYPE) == U_JT_TRANSPARENT; }
	_ALWAYS_INLINE_ bool is_ligature(uint32_t p_chr, uint32_t p_nchr) const { return (is_lam(p_chr) && is_alef(p_nchr)); }
	_ALWAYS_INLINE_ bool is_connected_to_prev(uint32_t p_chr, uint32_t p_pchr) const {
		int32_t prop = u_getIntPropertyValue(p_pchr, UCHAR_JOINING_TYPE);
		return (prop != U_JT_RIGHT_JOINING) && (prop != U_JT_NON_JOINING) ? !is_ligature(p_pchr, p_chr) : false;
	}
	_ALWAYS_INLINE_ bool is_ws(UChar32 p_char) const {
		return (p_char == 0x000A) || (p_char == 0x000B) || (p_char == 0x000C) || (p_char == 0x000D) || (p_char == 0x0085) || (p_char == 0x2028) || (p_char == 0x2029) || (p_char == 0x0009) || (p_char == 0x0020) || (p_char == 0x1680) || (p_char == 0x205F) || (p_char == 0x3000) || (p_char == 0x180E) || (p_char == 0x2060) || ((p_char >= 0x2000) && (p_char <= 0x200D));
	}
	_ALWAYS_INLINE_ bool is_break(UChar32 p_char) const {
		return (p_char == 0x000A) || (p_char == 0x000B) || (p_char == 0x000C) || (p_char == 0x000D) || (p_char == 0x0085) || (p_char == 0x2028) || (p_char == 0x2029);
	}
	_ALWAYS_INLINE_ UChar32 get_char(int64_t p_offset) const {
		UChar32 ret;
		U16_GET(data, 0, p_offset, data_size, ret);
		return ret;
	}

	virtual void _clear_props();
	virtual void _clear_visual();

	virtual void _generate_kashida_justification_opportunies(int64_t p_start, int64_t p_end, /*out*/ std::vector<JustificationOpportunity> &p_ops) const;
	virtual void _generate_justification_opportunies(int32_t p_start, int32_t p_end, const char *p_lang, /*out*/ std::vector<JustificationOpportunity> &p_ops) const;
	virtual void _generate_break_opportunies(int32_t p_start, int32_t p_end, const char *p_lang, /*out*/ std::vector<BreakOpportunity> &p_ops) const;

	virtual void _generate_justification_opportunies_fallback(int32_t p_start, int32_t p_end, /*out*/ std::vector<JustificationOpportunity> &p_ops) const;
	virtual void _generate_break_opportunies_fallback(int32_t p_start, int32_t p_end, /*out*/ std::vector<BreakOpportunity> &p_ops) const;

	void _shape_substring(TLShapedString *p_ref, int64_t p_start, int64_t p_end, int p_trim) const;

	virtual void _shape_single_cluster(int64_t p_start, int64_t p_end, hb_direction_t p_run_direction, hb_script_t p_run_script, UChar32 p_codepoint, TLFontFallbackIterator p_font, /*out*/ Cluster &p_cluster, bool p_font_override = false) const;
	virtual void _shape_bidi_script_run(hb_direction_t p_run_direction, hb_script_t p_run_script, int32_t p_run_start, int32_t p_run_end, TLFontFallbackIterator p_font);
	virtual void _shape_bidi_run(hb_direction_t p_run_direction, int32_t p_run_start, int32_t p_run_end);
	virtual void _shape_hex_run(hb_direction_t p_run_direction, int32_t p_run_start, int32_t p_run_end);
	virtual void _shape_full_string();

public:
	TLShapedString();
	virtual ~TLShapedString();

	void _init();

	void copy_properties(Ref<TLShapedString> p_source);

	//Input
	int get_base_direction() const;
	void set_base_direction(int p_base_direction);

	String get_text() const;
	void set_text(const String p_text);
	virtual void add_text(const String p_text);
	virtual void replace_text(int64_t p_start, int64_t p_end, const String p_text);

	PoolByteArray get_utf8() const;
	void set_utf8(const PoolByteArray p_text);
	virtual void add_utf8(const PoolByteArray p_text);
	virtual void replace_utf8(int64_t p_start, int64_t p_end, const PoolByteArray p_text);

	PoolByteArray get_utf16() const;
	void set_utf16(const PoolByteArray p_text);
	virtual void add_utf16(const PoolByteArray p_text);
	virtual void replace_utf16(int64_t p_start, int64_t p_end, const PoolByteArray p_text);

	PoolByteArray get_utf32() const;
	void set_utf32(const PoolByteArray p_text);
	virtual void add_utf32(const PoolByteArray p_text);
	virtual void replace_utf32(int64_t p_start, int64_t p_end, const PoolByteArray p_text);

	virtual void add_sstring(Ref<TLShapedString> p_text);
	virtual void replace_sstring(int64_t p_start, int64_t p_end, Ref<TLShapedString> p_text);

	Ref<TLFontFamily> get_base_font() const;
	void set_base_font(const Ref<TLFontFamily> p_font);

	String get_base_font_style() const;
	void set_base_font_style(const String p_style);

	int get_base_font_size() const;
	void set_base_font_size(int p_size);

	String get_features() const;
	void set_features(const String p_features);

	String get_language() const;
	void set_language(const String p_language);

	bool get_preserve_control() const;
	void set_preserve_control(bool p_enable);

	//Line data
	virtual bool shape();
	virtual bool is_valid() const;
	virtual bool empty() const;
	virtual int64_t length() const;
	virtual int64_t char_count() const;

	virtual float get_ascent() const;
	virtual float get_descent() const;
	virtual float get_width() const;
	virtual float get_height() const;

	//Line modification
	virtual std::vector<int> break_words() const;
	virtual std::vector<int> break_jst() const;
	virtual std::vector<int> break_lines(float p_width, TextBreak p_flags) const;
	virtual Ref<TLShapedString> substr(int64_t p_start, int64_t p_end, int p_trim) const;
	virtual float extend_to_width(float p_width, TextJustification p_flags);

	//Cluster data
	virtual int64_t clusters() const;
	virtual int64_t get_cluster_index(int64_t p_position) const;
	virtual Ref<TLFontFace> get_cluster_face(int64_t p_index) const;
	virtual float get_cluster_face_size(int64_t p_index) const;
	virtual int64_t get_cluster_glyphs(int64_t p_index) const;
	virtual uint32_t get_cluster_glyph(int64_t p_index, int64_t p_glyph) const;
	virtual Point2 get_cluster_glyph_offset(int64_t p_index, int64_t p_glyph) const;
	virtual Point2 get_cluster_glyph_advance(int64_t p_index, int64_t p_glyph) const;
	virtual float get_cluster_trailing_edge(int64_t p_index) const;
	virtual float get_cluster_leading_edge(int64_t p_index) const;
	virtual int64_t get_cluster_start(int64_t p_index) const;
	virtual int64_t get_cluster_end(int64_t p_index) const;
	virtual float get_cluster_ascent(int64_t p_index) const;
	virtual float get_cluster_descent(int64_t p_index) const;
	virtual float get_cluster_width(int64_t p_index) const;
	virtual float get_cluster_height(int64_t p_index) const;
	virtual Rect2 get_cluster_rect(int64_t p_index) const;
	virtual String get_cluster_debug_info(int64_t p_index) const;

	//Output
	virtual std::vector<Rect2> get_highlight_shapes(int64_t p_start, int64_t p_end) const;
	virtual std::vector<float> get_cursor_positions(int64_t p_position, TextDirection p_primary_dir) const;
	virtual TextDirection get_char_direction(int64_t p_position) const;
	virtual int64_t hit_test(float p_position) const;
	virtual int64_t hit_test_cluster(float p_position) const;

	int get_para_direction() const;

	virtual Vector2 draw_cluster(RID p_canvas_item, const Point2 p_position, int64_t p_index, const Color p_modulate = Color(1, 1, 1));
	virtual void draw(RID p_canvas_item, const Point2 p_position, const Color p_modulate = Color(1, 1, 1));

	//Extra
	virtual void draw_dbg(RID p_canvas_item, const Point2 p_position, const Color p_modulate = Color(1, 1, 1), bool p_draw_brk_ops = false, bool p_draw_jst_ops = false);
	virtual void draw_as_hex(RID p_canvas_item, const Point2 p_position, const Color p_modulate = Color(1, 1, 1), bool p_draw_brk_ops = false, bool p_draw_jst_ops = false);
	virtual void draw_logical_as_hex(RID p_canvas_item, const Point2 p_position, const Color p_modulate = Color(1, 1, 1), bool p_draw_brk_ops = false, bool p_draw_jst_ops = false);

	//GDNative wrappers
	Array _break_words() const;
	Array _break_jst() const;
	Array _break_lines(float p_width, int64_t p_flags) const;
	Array _get_highlight_shapes(int64_t p_start, int64_t p_end) const;
	Array _get_cursor_positions(int64_t p_position, int64_t p_primary_dir) const;
	float _extend_to_width(float p_width, int64_t p_flags);
	float _collapse_to_width(float p_width, int64_t p_flags);

	//Helpers
	int64_t pos_u16_to_wcs(int64_t p_position) const;
	int64_t pos_wcs_to_u16(int64_t p_position) const;

	int64_t next_safe_bound(int64_t p_offset) const;
	int64_t prev_safe_bound(int64_t p_offset) const;

	void _font_changed();

#ifdef GODOT_MODULE
	static void _bind_methods();
#else
	static void _register_methods();
#endif
};

#ifdef GODOT_MODULE
VARIANT_ENUM_CAST(TextDirection);
VARIANT_ENUM_CAST(TextJustification);
VARIANT_ENUM_CAST(TextBreak);
VARIANT_ENUM_CAST(TextTrimMode);
#endif

#endif /*TL_SHAPED_STRING_HPP*/
