/*************************************************************************/
/*  tl_shaped_attributed_string.cpp                                      */
/*************************************************************************/

#include "resources/tl_shaped_attributed_string.hpp"

#ifdef GODOT_MODULE
#include "core/dictionary.h"
#include "core/translation.h"
#include "servers/rendering_server.h"
#else
#include <Dictionary.hpp>
#include <TranslationServer.hpp>
#include <RenderingServer.hpp>
#endif

/*************************************************************************/
/*  ShapedAttributedString                                               */
/*************************************************************************/

void TLShapedAttributedString::_shape_substring(TLShapedAttributedString *p_ref, int64_t p_start, int64_t p_end, int p_trim) const {

	if (!p_ref)
		return;

	p_ref->_clear_props();
	p_ref->_clear_visual();

	//Trim edge line breaks chars or all ws
	if (p_trim == TEXT_TRIM_BREAK_AND_WHITESPACE) {
		while ((p_end > p_start) && u_isWhitespace(get_char(p_end - 1))) {
			p_end--;
		}

		while ((p_end > p_start) && u_isWhitespace(get_char(p_start))) {
			p_start++;
		}
	} else if (p_trim == TEXT_TRIM_BREAK) {
		while ((p_end > p_start) && is_break(get_char(p_end - 1))) {
			p_end--;
		}

		while ((p_end > p_start) && is_break(get_char(p_start))) {
			p_start++;
		}
	}

	//Copy substring data
	p_ref->data = (UChar *)std::malloc((p_end - p_start + 1) * sizeof(UChar));
	memcpy(p_ref->data, &data[p_start], (p_end - p_start) * sizeof(UChar));
	p_ref->data_size = (p_end - p_start);
	p_ref->data[p_end - p_start] = 0x0000;
	p_ref->base_direction = base_direction;
	p_ref->para_direction = para_direction;
	if (p_ref->base_font.is_valid() && p_ref->base_font->is_connected(_CHANGED, callable_mp((TLShapedString *)p_ref, &TLShapedString::_font_changed))) {
		p_ref->base_font->disconnect(_CHANGED, callable_mp((TLShapedString *)p_ref, &TLShapedString::_font_changed));
	}
	p_ref->base_font = base_font;
	if (p_ref->base_font.is_valid() && !p_ref->base_font->is_connected(_CHANGED, callable_mp((TLShapedString *)p_ref, &TLShapedString::_font_changed))) {
		p_ref->base_font->connect(_CHANGED, callable_mp((TLShapedString *)p_ref, &TLShapedString::_font_changed));
	}
	p_ref->base_style = base_style;
	p_ref->base_size = base_size;

	p_ref->language = language;
	p_ref->font_features = font_features;

	//Copy attributes
	auto attrib = format_attributes.find_closest(p_start);
	while (attrib && (attrib->key() < p_end)) {
		p_ref->format_attributes[MAX(0, attrib->key() - p_start)] = attrib->get();
		attrib = attrib->next();
	}
	attrib = style_attributes.find_closest(p_start);
	while (attrib && (attrib->key() < p_end)) {
		p_ref->style_attributes[MAX(0, attrib->key() - p_start)] = attrib->get();
		attrib = attrib->next();
	}
	p_ref->_reconnect_fonts();

	UErrorCode err = U_ZERO_ERROR;
	//Create temporary line bidi & shape
	p_ref->bidi_iter = ubidi_openSized((p_end - p_start), 0, &err);
	if (U_FAILURE(err)) {
		p_ref->bidi_iter = NULL;
		//Do not error out on failure - just continue with full reshapeing
	} else {
		ubidi_setLine(bidi_iter, p_start, p_end, p_ref->bidi_iter, &err);
		if (U_FAILURE(err)) {
			ubidi_close(p_ref->bidi_iter);
			p_ref->bidi_iter = NULL;
		}
	}

	p_ref->_shape_full_string();

	//Close temorary line BiDi
	if (p_ref->bidi_iter) {
		ubidi_close(p_ref->bidi_iter);
		p_ref->bidi_iter = NULL;
	}
}

void TLShapedAttributedString::_shape_single_cluster(int64_t p_start, int64_t p_end, hb_direction_t p_run_direction, hb_script_t p_run_script, UChar32 p_codepoint, TLFontFallbackIterator p_font, /*out*/ Cluster &p_cluster, bool p_font_override) const {

	auto attrib_iter = (p_run_direction == HB_DIRECTION_LTR) ? format_attributes.find_closest(p_start) : format_attributes.find_closest(p_end - 1);
	if (!attrib_iter) {
		//Shape as plain string
		TLShapedString::_shape_single_cluster(p_start, p_end, p_run_direction, p_run_script, p_codepoint, p_font, p_cluster);
		return;
	}

	//Shape single cluster using HarfBuzz
	TLFontFallbackIterator _iter = p_font;
	Ref<TLFontFace> _font = _iter.value();
	int64_t _size = base_size;
	if (!p_font_override) {
		if (attrib_iter->get().has(TEXT_ATTRIBUTE_FONT)) {
			Ref<TLFontFamily> family = Ref<TLFontFamily>(attrib_iter->get()[TEXT_ATTRIBUTE_FONT]);
			String style = base_style;
			if (attrib_iter->get().has(TEXT_ATTRIBUTE_FONT_STYLE)) {
				style = attrib_iter->get()[TEXT_ATTRIBUTE_FONT_STYLE];
			}

			_iter = family->get_face_for_script(style, p_run_script);
			if (_iter.is_valid()) {
				_font = _iter.value();
				if (_font.is_null()) {
					_iter = p_font;
				}
			}
		}
	}
	if (attrib_iter->get().has(TEXT_ATTRIBUTE_FONT_SIZE)) {
		_size = attrib_iter->get()[TEXT_ATTRIBUTE_FONT_SIZE];
	}
	hb_font_t *hb_font = _font->get_hb_font(_size);
	if (!hb_font) {
		if (_iter.next().is_valid()) {
			_shape_single_cluster(p_start, p_end, p_run_direction, p_run_script, p_codepoint, _iter.next(), p_cluster, true);
		}
		return;
	}
	hb_buffer_clear_contents(hb_buffer);
	hb_buffer_set_direction(hb_buffer, p_run_direction);
	if (preserve_control) {
		hb_buffer_set_flags(hb_buffer, (hb_buffer_flags_t)(HB_BUFFER_FLAG_PRESERVE_DEFAULT_IGNORABLES));
	} else {
		hb_buffer_set_flags(hb_buffer, (hb_buffer_flags_t)(HB_BUFFER_FLAG_DEFAULT));
	}
	hb_buffer_set_script(hb_buffer, p_run_script);

	if (attrib_iter->get().has(TEXT_ATTRIBUTE_LANGUAGE)) {
		String cluster_language = attrib_iter->get()[TEXT_ATTRIBUTE_LANGUAGE];
		hb_language_t _language = hb_language_from_string(cluster_language.ascii().get_data(), -1);
		if (_language != HB_LANGUAGE_INVALID) hb_buffer_set_language(hb_buffer, _language);
	} else {
		if (language != HB_LANGUAGE_INVALID) hb_buffer_set_language(hb_buffer, language);
	}

	hb_buffer_add_utf32(hb_buffer, (const uint32_t *)&p_codepoint, 1, 0, 1);
	if (attrib_iter->get().has(TEXT_ATTRIBUTE_FONT_FEATURES)) {
		String s_features = attrib_iter->get()[TEXT_ATTRIBUTE_FONT_FEATURES];
#ifdef GODOT_MODULE
		Vector<String> v_features = s_features.split(",");
#else
		PoolStringArray v_features = s_features.split(",");
#endif
		std::vector<hb_feature_t> _font_features;
		for (int64_t i = 0; i < (int64_t)v_features.size(); i++) {
			hb_feature_t feature;
			if (hb_feature_from_string(v_features[i].ascii().get_data(), -1, &feature)) {
				feature.start = 0;
				feature.end = (unsigned int)-1;
				_font_features.push_back(feature);
			}
		}
		hb_shape(hb_font, hb_buffer, _font_features.empty() ? NULL : &_font_features[0], _font_features.size());
	} else {
		hb_shape(hb_font, hb_buffer, font_features.empty() ? NULL : &font_features[0], font_features.size());
	}

	unsigned int glyph_count;
	hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(hb_buffer, &glyph_count);
	hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(hb_buffer, &glyph_count);

	p_cluster.glyphs.clear();
	p_cluster.font_face = _font.ptr();

	//debug info
	p_cluster.script = p_run_script;
	p_cluster.dir = p_run_direction;
	p_cluster.lang = language;
	//debug info

	p_cluster.is_rtl = (p_run_direction == HB_DIRECTION_RTL);
	p_cluster.cl_type = _CLUSTER_TYPE_TEXT;
	p_cluster.valid = true;
	p_cluster.start = p_start;
	p_cluster.end = p_end;
	p_cluster.ascent = _font->get_ascent(_size);
	p_cluster.descent = _font->get_descent(_size);
	p_cluster.width = 0.0f;

	if (glyph_count > 0) {
		for (unsigned int i = 0; i < glyph_count; i++) {
			p_cluster.glyphs.push_back(Glyph(glyph_info[i].codepoint, Point2((glyph_pos[i].x_offset) / 64, -(glyph_pos[i].y_offset / 64)), Point2((glyph_pos[i].x_advance * _font->get_glyph_scale(_size)) / 64, ((glyph_pos[i].y_advance * _font->get_glyph_scale(_size)) / 64))));
			p_cluster.valid &= ((glyph_info[i].codepoint != 0) || !u_isgraph(p_codepoint));
			p_cluster.width += (glyph_pos[i].x_advance * _font->get_glyph_scale(_size)) / 64;
		}
	}
	if (!p_cluster.valid) {
		if (_iter.next().is_valid()) {
			_shape_single_cluster(p_start, p_end, p_run_direction, p_run_script, p_codepoint, _iter.next(), p_cluster, true);
		}
	}
}

void TLShapedAttributedString::_generate_justification_opportunies(int32_t p_start, int32_t p_end, const char *p_lang, /*out*/ std::vector<JustificationOpportunity> &p_ops) const {

	auto attrib_iter = format_attributes.find_closest(p_start);
	if (!attrib_iter) {
		TLShapedString::_generate_justification_opportunies(p_start, p_end, p_lang, p_ops);
		return;
	}

	int64_t sh_start = (attrib_iter) ? MAX(p_start, attrib_iter->key()) : p_start;
	int64_t sh_end = (attrib_iter->next()) ? MIN(p_end, attrib_iter->next()->key()) : p_end;
	while (true) {

		if (attrib_iter->get().has(TEXT_ATTRIBUTE_LANGUAGE)) {
			String s_lang = attrib_iter->get()[TEXT_ATTRIBUTE_LANGUAGE];
			TLShapedString::_generate_justification_opportunies(sh_start, sh_end, s_lang.ascii().get_data(), p_ops);
		} else {
			TLShapedString::_generate_justification_opportunies(sh_start, sh_end, p_lang, p_ops);
		}

		if (attrib_iter->next() && (attrib_iter->next()->key() <= sh_end)) attrib_iter = attrib_iter->next();
		if (sh_end == p_end) break;
		sh_start = sh_end;
		sh_end = (attrib_iter->next()) ? MIN(attrib_iter->next()->key(), p_end) : p_end;
	}
}

void TLShapedAttributedString::_generate_break_opportunies(int32_t p_start, int32_t p_end, const char *p_lang, /*out*/ std::vector<BreakOpportunity> &p_ops) const {

	auto attrib_iter = format_attributes.find_closest(p_start);
	if (!attrib_iter) {
		TLShapedString::_generate_break_opportunies(p_start, p_end, p_lang, p_ops);
		return;
	}

	int64_t sh_start = (attrib_iter) ? MAX(p_start, attrib_iter->key()) : p_start;
	int64_t sh_end = (attrib_iter->next()) ? MIN(p_end, attrib_iter->next()->key()) : p_end;
	while (true) {

		if (attrib_iter->get().has(TEXT_ATTRIBUTE_LANGUAGE)) {
			String s_lang = attrib_iter->get()[TEXT_ATTRIBUTE_LANGUAGE];
			TLShapedString::_generate_break_opportunies(sh_start, sh_end, s_lang.ascii().get_data(), p_ops);
		} else {
			TLShapedString::_generate_break_opportunies(sh_start, sh_end, p_lang, p_ops);
		}

		if (attrib_iter->next() && (attrib_iter->next()->key() <= sh_end)) attrib_iter = attrib_iter->next();
		if (sh_end == p_end) break;
		sh_start = sh_end;
		sh_end = (attrib_iter->next()) ? MIN(attrib_iter->next()->key(), p_end) : p_end;
	}
}

void TLShapedAttributedString::_shape_bidi_script_run(hb_direction_t p_run_direction, hb_script_t p_run_script, int32_t p_run_start, int32_t p_run_end, TLFontFallbackIterator p_font) {

	auto attrib_iter = (p_run_direction == HB_DIRECTION_LTR) ? format_attributes.find_closest(p_run_start) : format_attributes.find_closest(p_run_end - 1);
	if (!attrib_iter) {
		//Shape as plain string
		TLShapedString::_shape_bidi_script_run(p_run_direction, p_run_script, p_run_start, p_run_end, p_font);
		return;
	}
	//Iter Attrib runs and call next bidi_script_attrib run
	int64_t sh_start = (attrib_iter) ? MAX(p_run_start, attrib_iter->key()) : p_run_start;
	int64_t sh_end = (attrib_iter->next()) ? MIN(p_run_end, attrib_iter->next()->key()) : p_run_end;
	while (true) {
		_shape_bidi_script_attrib_run(p_run_direction, p_run_script, attrib_iter->get(), sh_start, sh_end, p_font);
		if (p_run_direction == HB_DIRECTION_LTR) {
			if (attrib_iter->next() && (attrib_iter->next()->key() <= sh_end)) attrib_iter = attrib_iter->next();
			if (sh_end == p_run_end) break;
			sh_start = sh_end;
			sh_end = (attrib_iter->next()) ? MIN(attrib_iter->next()->key(), p_run_end) : p_run_end;
		} else {
			if (attrib_iter->prev() && (attrib_iter->key() >= sh_start)) attrib_iter = attrib_iter->prev();
			if (sh_start == p_run_start) break;
			sh_end = sh_start;
			sh_start = (attrib_iter->prev()) ? MAX(attrib_iter->key(), p_run_start) : p_run_start;
		}
	}
}

void TLShapedAttributedString::_shape_rect_run(hb_direction_t p_run_direction, const Size2 &p_size, TextVAlign p_align, int32_t p_run_start, int32_t p_run_end) {

	//"Shape" monotone image run
	if (p_run_direction == HB_DIRECTION_LTR) {
		for (int64_t i = p_run_start; i < p_run_end; i++) {
			Cluster new_cluster;
			new_cluster.is_rtl = (p_run_direction == HB_DIRECTION_RTL);
			new_cluster.cl_type = _CLUSTER_TYPE_RECT;
			new_cluster.valid = true;
			new_cluster.start = i;
			new_cluster.end = i;
			switch (p_align) {
				case TEXT_VALIGN_TOP: {
					new_cluster.ascent = p_size.height;
					new_cluster.descent = 0;
				} break;
				case TEXT_VALIGN_CENTER: {
					new_cluster.ascent = p_size.height / 2;
					new_cluster.descent = p_size.height / 2;
				} break;
				case TEXT_VALIGN_BOTTOM: {
					new_cluster.ascent = 0;
					new_cluster.descent = p_size.height;
				} break;
			}
			new_cluster.width = p_size.width;
			visual.push_back(new_cluster);
		}
	} else {
		for (int64_t i = p_run_end - 1; i >= p_run_start; i--) {
			Cluster new_cluster;
			new_cluster.is_rtl = (p_run_direction == HB_DIRECTION_RTL);
			new_cluster.cl_type = _CLUSTER_TYPE_RECT;
			new_cluster.valid = true;
			new_cluster.start = i;
			new_cluster.end = i;
			switch (p_align) {
				case TEXT_VALIGN_TOP: {
					new_cluster.ascent = p_size.height;
					new_cluster.descent = 0;
				} break;
				case TEXT_VALIGN_CENTER: {
					new_cluster.ascent = p_size.height / 2;
					new_cluster.descent = p_size.height / 2;
				} break;
				case TEXT_VALIGN_BOTTOM: {
					new_cluster.ascent = 0;
					new_cluster.descent = p_size.height;
				} break;
			}
			new_cluster.width = p_size.width;
			visual.push_back(new_cluster);
		}
	}
}

void TLShapedAttributedString::_shape_image_run(hb_direction_t p_run_direction, const Ref<Texture2D> &p_image, TextVAlign p_align, int32_t p_run_start, int32_t p_run_end) {

	//"Shape" monotone image run
	if (p_run_direction == HB_DIRECTION_LTR) {
		for (int64_t i = p_run_start; i < p_run_end; i++) {
			Cluster new_cluster;
			new_cluster.is_rtl = (p_run_direction == HB_DIRECTION_RTL);
			new_cluster.cl_type = _CLUSTER_TYPE_IMAGE;
			new_cluster.valid = true;
			new_cluster.start = i;
			new_cluster.end = i;
			switch (p_align) {
				case TEXT_VALIGN_TOP: {
					new_cluster.ascent = p_image->get_height();
					new_cluster.descent = 0;
				} break;
				case TEXT_VALIGN_CENTER: {
					new_cluster.ascent = p_image->get_height() / 2;
					new_cluster.descent = p_image->get_height() / 2;
				} break;
				case TEXT_VALIGN_BOTTOM: {
					new_cluster.ascent = 0;
					new_cluster.descent = p_image->get_height();
				} break;
			}
			new_cluster.width = p_image->get_width();
			visual.push_back(new_cluster);
		}
	} else {
		for (int64_t i = p_run_end - 1; i >= p_run_start; i--) {
			Cluster new_cluster;
			new_cluster.is_rtl = (p_run_direction == HB_DIRECTION_RTL);
			new_cluster.cl_type = _CLUSTER_TYPE_IMAGE;
			new_cluster.valid = true;
			new_cluster.start = i;
			new_cluster.end = i;
			switch (p_align) {
				case TEXT_VALIGN_TOP: {
					new_cluster.ascent = p_image->get_height();
					new_cluster.descent = 0;
				} break;
				case TEXT_VALIGN_CENTER: {
					new_cluster.ascent = p_image->get_height() / 2;
					new_cluster.descent = p_image->get_height() / 2;
				} break;
				case TEXT_VALIGN_BOTTOM: {
					new_cluster.ascent = 0;
					new_cluster.descent = p_image->get_height();
				} break;
			}
			new_cluster.width = p_image->get_width();
			visual.push_back(new_cluster);
		}
	}
}

void TLShapedAttributedString::_shape_bidi_script_attrib_run(hb_direction_t p_run_direction, hb_script_t p_run_script, const Map<TextAttribute, Variant> &p_attribs, int32_t p_run_start, int32_t p_run_end, TLFontFallbackIterator p_font, bool p_font_override) {

	//Handle rects for embedded custom objects
	if (p_attribs.has(TEXT_ATTRIBUTE_REPLACEMENT_RECT)) {
		Size2 rect = Size2(p_attribs[TEXT_ATTRIBUTE_REPLACEMENT_RECT]);
		if (rect != Size2()) {
			int64_t align = TEXT_VALIGN_CENTER;
			if (p_attribs.has(TEXT_ATTRIBUTE_REPLACEMENT_VALIGN)) {
				align = int(p_attribs[TEXT_ATTRIBUTE_REPLACEMENT_VALIGN]);
			}

			_shape_rect_run(p_run_direction, rect, (TextVAlign)align, p_run_start, p_run_end);
			return;
		}
	}

	//Handle image runs
	if (p_attribs.has(TEXT_ATTRIBUTE_REPLACEMENT_IMAGE)) {
		Ref<Texture2D> image = Ref<Texture2D>(p_attribs[TEXT_ATTRIBUTE_REPLACEMENT_IMAGE]);
		if (!image.is_null()) {
			int64_t align = TEXT_VALIGN_CENTER;
			if (p_attribs.has(TEXT_ATTRIBUTE_REPLACEMENT_VALIGN)) {
				align = int(p_attribs[TEXT_ATTRIBUTE_REPLACEMENT_VALIGN]);
			}

			_shape_image_run(p_run_direction, image, (TextVAlign)align, p_run_start, p_run_end);
			return;
		}
	}

	//Shape monotone run using HarfBuzz
	TLFontFallbackIterator _iter = p_font;
	Ref<TLFontFace> _font = _iter.value();
	if (!p_font_override) {
		if (p_attribs.has(TEXT_ATTRIBUTE_FONT)) {
			Ref<TLFontFamily> family = Ref<TLFontFamily>(p_attribs[TEXT_ATTRIBUTE_FONT]);
			String style = base_style;
			if (p_attribs.has(TEXT_ATTRIBUTE_FONT_STYLE)) {
				style = p_attribs[TEXT_ATTRIBUTE_FONT_STYLE];
			}

			_iter = family->get_face_for_script(style, p_run_script);
			if (_iter.is_valid()) {
				_font = _iter.value();
				if (_font.is_null()) {
					_iter = p_font;
				}
			}
		}
	}
	int64_t _size = base_size;
	if (p_attribs.has(TEXT_ATTRIBUTE_FONT_SIZE)) {
		_size = p_attribs[TEXT_ATTRIBUTE_FONT_SIZE];
	}
	hb_font_t *hb_font = _font->get_hb_font(_size);
	if (!hb_font) {
		if (_iter.next().is_valid()) {
			_shape_bidi_script_run(p_run_direction, p_run_script, p_run_start, p_run_end, _iter.next());
		} else {
			_shape_hex_run(p_run_direction, p_run_start, p_run_end);
		}
		return;
	}
	hb_buffer_clear_contents(hb_buffer);
	hb_buffer_set_direction(hb_buffer, p_run_direction);
	if (preserve_control) {
		hb_buffer_set_flags(hb_buffer, (hb_buffer_flags_t)(HB_BUFFER_FLAG_PRESERVE_DEFAULT_IGNORABLES | (p_run_start == 0 ? HB_BUFFER_FLAG_BOT : 0) | (p_run_end == data_size ? HB_BUFFER_FLAG_EOT : 0)));
	} else {
		hb_buffer_set_flags(hb_buffer, (hb_buffer_flags_t)(HB_BUFFER_FLAG_DEFAULT | (p_run_start == 0 ? HB_BUFFER_FLAG_BOT : 0) | (p_run_end == data_size ? HB_BUFFER_FLAG_EOT : 0)));
	}
	hb_buffer_set_script(hb_buffer, p_run_script);

	if (p_attribs.has(TEXT_ATTRIBUTE_LANGUAGE)) {
		String cluster_language = p_attribs[TEXT_ATTRIBUTE_LANGUAGE];
		hb_language_t _language = hb_language_from_string(cluster_language.ascii().get_data(), -1);
		if (_language != HB_LANGUAGE_INVALID) hb_buffer_set_language(hb_buffer, _language);
	} else {
		if (language != HB_LANGUAGE_INVALID) hb_buffer_set_language(hb_buffer, language);
	}

	hb_buffer_add_utf16(hb_buffer, (const uint16_t *)data, data_size, p_run_start, p_run_end - p_run_start);
	if (p_attribs.has(TEXT_ATTRIBUTE_FONT_FEATURES)) {
		String s_features = p_attribs[TEXT_ATTRIBUTE_FONT_FEATURES];
#ifdef GODOT_MODULE
		Vector<String> v_features = s_features.split(",");
#else
		PoolStringArray v_features = s_features.split(",");
#endif
		std::vector<hb_feature_t> _font_features;
		for (int64_t i = 0; i < (int64_t)v_features.size(); i++) {
			hb_feature_t feature;
			if (hb_feature_from_string(v_features[i].ascii().get_data(), -1, &feature)) {
				feature.start = 0;
				feature.end = (unsigned int)-1;
				_font_features.push_back(feature);
			}
		}
		hb_shape(hb_font, hb_buffer, _font_features.empty() ? NULL : &_font_features[0], _font_features.size());
	} else {
		hb_shape(hb_font, hb_buffer, font_features.empty() ? NULL : &font_features[0], font_features.size());
	}

	//Compose grapheme clusters
	std::vector<Cluster> run_clusters;

	unsigned int glyph_count;
	hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(hb_buffer, &glyph_count);
	hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(hb_buffer, &glyph_count);

	if (glyph_count > 0) {
		int64_t last_cluster_id = -1;
		for (unsigned int i = 0; i < glyph_count; i++) {
			if (glyph_info[i].cluster >= data_size) {
				ERR_FAIL_COND(true);
			}
			if (last_cluster_id != glyph_info[i].cluster) {
				//Start new cluster
				Cluster new_cluster;

				//debug info
				new_cluster.script = p_run_script;
				new_cluster.dir = p_run_direction;
				new_cluster.lang = language;
				//debug info

				new_cluster.font_face = _font.ptr();
				new_cluster.is_rtl = (p_run_direction == HB_DIRECTION_RTL);
				new_cluster.cl_type = _CLUSTER_TYPE_TEXT;
				new_cluster.glyphs.push_back(Glyph(glyph_info[i].codepoint, Point2((glyph_pos[i].x_offset) / 64, -(glyph_pos[i].y_offset / 64)), Point2((glyph_pos[i].x_advance * _font->get_glyph_scale(_size)) / 64, ((glyph_pos[i].y_advance * _font->get_glyph_scale(_size)) / 64))));
				new_cluster.valid = ((glyph_info[i].codepoint != 0) || !u_isgraph(get_char(glyph_info[i].cluster)));
				new_cluster.start = glyph_info[i].cluster;
				new_cluster.end = glyph_info[i].cluster;
				new_cluster.ascent = _font->get_ascent(_size);
				new_cluster.descent = _font->get_descent(_size);
				new_cluster.width += (glyph_pos[i].x_advance * _font->get_glyph_scale(_size)) / 64;

				//Set previous logical cluster end limit
				if (i != 0) {
					if (p_run_direction == HB_DIRECTION_LTR) {
						run_clusters[run_clusters.size() - 1].end = glyph_info[i].cluster - 1;
					} else {
						new_cluster.end = run_clusters[run_clusters.size() - 1].start - 1;
					}
				}

				run_clusters.push_back(new_cluster);

				last_cluster_id = glyph_info[i].cluster;
			} else {
				//Add glyphs to existing cluster
				run_clusters[run_clusters.size() - 1].glyphs.push_back(Glyph(glyph_info[i].codepoint, Point2((glyph_pos[i].x_offset) / 64, -(glyph_pos[i].y_offset / 64)), Point2((glyph_pos[i].x_advance * _font->get_glyph_scale(_size)) / 64, ((glyph_pos[i].y_advance * _font->get_glyph_scale(_size)) / 64))));
				run_clusters[run_clusters.size() - 1].valid &= ((glyph_info[i].codepoint != 0) || !u_isgraph(get_char(glyph_info[i].cluster)));
				run_clusters[run_clusters.size() - 1].width += (glyph_pos[i].x_advance * _font->get_glyph_scale(_size)) / 64;
			}
		}
		//Set last logical cluster end limit
		if (run_clusters.size() > 0) {
			if (p_run_direction == HB_DIRECTION_LTR) {
				run_clusters[run_clusters.size() - 1].end = p_run_end - 1;
			} else {
				run_clusters[0].end = p_run_end - 1;
			}
		}
	}

	//Reshape sub-runs with invalid clusters using fallback fonts
	if (run_clusters.size() > 0) {
		int64_t failed_subrun_start = p_run_end + 1;
		int64_t failed_subrun_end = p_run_start;
		for (int64_t i = 0; i < (int64_t)run_clusters.size(); i++) {
			if (run_clusters[i].valid) {
				if (failed_subrun_start != p_run_end + 1) {
					if (_iter.next().is_valid()) {
						_shape_bidi_script_attrib_run(p_run_direction, p_run_script, p_attribs, failed_subrun_start, failed_subrun_end + 1, _iter.next());
					} else {
						_shape_hex_run(p_run_direction, failed_subrun_start, failed_subrun_end + 1);
					}
					failed_subrun_start = p_run_end + 1;
					failed_subrun_end = p_run_start;
				}
				visual.push_back(run_clusters[i]);
			} else {
				if (failed_subrun_start >= run_clusters[i].start) failed_subrun_start = run_clusters[i].start;
				if (failed_subrun_end <= run_clusters[i].end) failed_subrun_end = run_clusters[i].end;
			}
		}
		if (failed_subrun_start != p_run_end + 1) {
			if (_iter.next().is_valid()) {
				_shape_bidi_script_attrib_run(p_run_direction, p_run_script, p_attribs, failed_subrun_start, failed_subrun_end + 1, _iter.next(), true);
			} else {
				_shape_hex_run(p_run_direction, failed_subrun_start, failed_subrun_end + 1);
			}
		}
	}
}

bool TLShapedAttributedString::_compare_attributes(const Map<TextAttribute, Variant> &p_first, const Map<TextAttribute, Variant> &p_second) const {

	if (p_first.size() != p_second.size()) return false;
	for (auto E = p_first.front(); E; E = E->next()) {
		auto F = p_second.find(E->key());
		if ((!F) || (E->get() != F->get())) return false;
	}
	return true;
}

void TLShapedAttributedString::_ensure_break(int64_t p_key, Map<int, Map<TextAttribute, Variant> > &p_attributes) {

	//Ensures there is a run break at offset.
	auto attrib = p_attributes.find_closest(p_key);
	p_attributes[p_key] = (attrib) ? attrib->get() : Map<TextAttribute, Variant>();
}

void TLShapedAttributedString::_optimize_attributes(Map<int, Map<TextAttribute, Variant> > &p_attributes) {

	std::vector<int> erase_list;
	for (auto E = p_attributes.front(); E; E = E->next()) {

		if (E->prev() && (_compare_attributes(E->get(), E->prev()->get()))) {
			erase_list.push_back(E->key());
		}
	}

	for (int64_t i = 0; i < (int64_t)erase_list.size(); i++) {
		p_attributes.erase(erase_list[i]);
	}
}

Ref<TLShapedString> TLShapedAttributedString::substr(int64_t p_start, int64_t p_end, int p_trim) const {

	Ref<TLShapedAttributedString> ret;
#ifdef GODOT_MODULE
	ret.instance();
#else
	ret = Ref<TLShapedAttributedString>::__internal_constructor(TLShapedAttributedString::_new());
#endif
	ret->base_direction = base_direction;
	ret->para_direction = para_direction;
	ret->base_font = base_font;
	if (ret->base_font.is_valid()) {
		ret->base_font->connect(_CHANGED, callable_mp((TLShapedString *)ret.ptr(), &TLShapedString::_font_changed));
	}
	ret->base_style = base_style;
	ret->base_size = base_size;
	ret->language = language;
	ret->font_features = font_features;

	if ((data_size == 0) || (p_start < 0) || (p_end > data_size) || (p_start > p_end) || (prev_safe_bound(p_start) > next_safe_bound(p_end))) {
		return ret;
	}

	_shape_substring(ret.ptr(), prev_safe_bound(p_start), next_safe_bound(p_end), p_trim);
	return ret;
}

void TLShapedAttributedString::_disconnect_fonts() {
	for (auto E = format_attributes.front(); E; E = E->next()) {
		for (auto F = E->get().front(); F; F = F->next()) {
			if (F->key() == TEXT_ATTRIBUTE_FONT) {
				Ref<TLFontFamily> font = F->get();
				if (font.is_valid() && font != base_font && font->is_connected(_CHANGED, callable_mp((TLShapedString *)this, &TLShapedString::_font_changed))) {
					font->disconnect(_CHANGED, callable_mp((TLShapedString *)this, &TLShapedString::_font_changed));
				}
			}
		}
	}
}

void TLShapedAttributedString::_reconnect_fonts() {
	for (auto E = format_attributes.front(); E; E = E->next()) {
		for (auto F = E->get().front(); F; F = F->next()) {
			if (F->key() == TEXT_ATTRIBUTE_FONT) {
				Ref<TLFontFamily> font = F->get();
				if (font.is_valid() && font != base_font && !font->is_connected(_CHANGED, callable_mp((TLShapedString *)this, &TLShapedString::_font_changed))) {
					font->connect(_CHANGED, callable_mp((TLShapedString *)this, &TLShapedString::_font_changed));
				}
			}
		}
	}
}

void TLShapedAttributedString::add_attribute(int64_t p_attribute, Variant p_value, int64_t p_start, int64_t p_end) {

	if (p_end == -1) p_end = data_size;
	if (p_start < 0 || p_end > data_size || p_start > p_end) {
		ERR_PRINT("Invalid substring range [" + String::num_int64(p_start) + " ..." + String::num_int64(p_end) + "] / " + String::num_int64(data_size));
		ERR_FAIL_COND(true);
	}

	//Check unsafe types
	if (p_attribute == TEXT_ATTRIBUTE_FONT) {
		Ref<TLFontFamily> _ref = p_value;
		if (!cast_to<TLFontFamily>(*_ref)) {
			ERR_PRINT("Type mismatch");
			return;
		}
	} else if (p_attribute == TEXT_ATTRIBUTE_REPLACEMENT_IMAGE) {
		Ref<Texture2D> _ref = p_value;
		if (!cast_to<Texture2D>(*_ref)) {
			ERR_PRINT("Type mismatch");
			return;
		}
	}

	if (p_attribute <= TEXT_ATTRIBUTE_MAX_FORMAT_ATTRIBUTE) {
		_disconnect_fonts();
		_ensure_break(0, format_attributes);
		_ensure_break(p_start, format_attributes);

		if (p_end < data_size) _ensure_break(p_end, format_attributes);

		auto attrib = format_attributes.find(p_start);
		while (attrib && ((attrib->key() < p_end) || (p_end == data_size))) {
			if ((p_end == data_size) && (p_end == attrib->key()) && ((p_attribute == TEXT_ATTRIBUTE_REPLACEMENT_IMAGE) || (p_attribute == TEXT_ATTRIBUTE_REPLACEMENT_RECT) || (p_attribute == TEXT_ATTRIBUTE_REPLACEMENT_ID))) {
				break; //do not apply replacement to the end of string
			}
			attrib->get()[(TextAttribute)p_attribute] = p_value;
			attrib = attrib->next();
		}
		_optimize_attributes(format_attributes);
		_reconnect_fonts();
		_clear_visual();
	} else {
		_ensure_break(0, style_attributes);
		_ensure_break(p_start, style_attributes);

		if (p_end < data_size) _ensure_break(p_end, style_attributes);

		auto attrib = style_attributes.find(p_start);
		while (attrib && ((attrib->key() < p_end) || (p_end == data_size))) {
			attrib->get()[(TextAttribute)p_attribute] = p_value;
			attrib = attrib->next();
		}
		_optimize_attributes(style_attributes);
	}
	emit_signal("string_changed");
}

void TLShapedAttributedString::remove_attribute(int64_t p_attribute, int64_t p_start, int64_t p_end) {

	if (p_end == -1) p_end = data_size;
	if (p_start < 0 || p_end > data_size || p_start > p_end) {
		ERR_PRINT("Invalid substring range [" + String::num_int64(p_start) + " ..." + String::num_int64(p_end) + "] / " + String::num_int64(data_size));
		ERR_FAIL_COND(true);
	}

	if (p_attribute <= TEXT_ATTRIBUTE_MAX_FORMAT_ATTRIBUTE) {
		_disconnect_fonts();
		_ensure_break(p_start, format_attributes);

		if (p_end < data_size) _ensure_break(p_end, format_attributes);

		auto attrib = format_attributes.find(p_start);
		while (attrib && (attrib->key() < p_end)) {
			attrib->get().erase((TextAttribute)p_attribute);
			attrib = attrib->next();
		}
		_optimize_attributes(format_attributes);
		_reconnect_fonts();
		_clear_props();
	} else {
		_ensure_break(p_start, style_attributes);

		if (p_end < data_size) _ensure_break(p_end, style_attributes);

		auto attrib = style_attributes.find(p_start);
		while (attrib && (attrib->key() < p_end)) {
			attrib->get().erase((TextAttribute)p_attribute);
			attrib = attrib->next();
		}
		_optimize_attributes(style_attributes);
	}
	_clear_visual();
	emit_signal("string_changed");
}

void TLShapedAttributedString::remove_attributes(int64_t p_start, int64_t p_end) {

	if (p_end == -1) p_end = data_size;
	if (p_start < 0 || p_end > data_size || p_start > p_end) {
		ERR_PRINT("Invalid substring range [" + String::num_int64(p_start) + " ..." + String::num_int64(p_end) + "] / " + String::num_int64(data_size));
		ERR_FAIL_COND(true);
	}

	_disconnect_fonts();

	_ensure_break(p_start, format_attributes);
	_ensure_break(p_start, style_attributes);

	if (p_end < data_size) {
		_ensure_break(p_end, format_attributes);
		_ensure_break(p_end, style_attributes);
	}

	auto attrib = format_attributes.find(p_start);
	while (attrib && (attrib->key() < p_end)) {
		attrib->get().clear();
		attrib = attrib->next();
	}
	attrib = style_attributes.find(p_start);
	while (attrib && (attrib->key() < p_end)) {
		attrib->get().clear();
		attrib = attrib->next();
	}

	_optimize_attributes(format_attributes);
	_optimize_attributes(style_attributes);

	_reconnect_fonts();
	_clear_props();
	emit_signal("string_changed");
}

void TLShapedAttributedString::clear_attributes() {

	_disconnect_fonts();
	format_attributes.clear();
	style_attributes.clear();
	_clear_props();
	emit_signal("string_changed");
}

Array TLShapedAttributedString::get_embedded_rects() {

	Array ret;

	if (!valid)
		_shape_full_string();

	if (!valid)
		return ret;

	Vector2 ofs;
	for (int64_t i = 0; i < (int64_t)visual.size(); i++) {
		if (visual[i].cl_type == (int)_CLUSTER_TYPE_RECT) {
			Dictionary ifo;
			auto attrib = format_attributes.find_closest(visual[i].start);
			if (attrib && attrib->get().has(TEXT_ATTRIBUTE_REPLACEMENT_ID)) {
				ifo[String("id")] = Variant(attrib->get()[TEXT_ATTRIBUTE_REPLACEMENT_ID]);
			}
			if (attrib && attrib->get().has(TEXT_ATTRIBUTE_REPLACEMENT_RECT)) {
				ifo[String("rect")] = Rect2(ofs + Point2(0, -visual[i].ascent), Vector2(attrib->get()[TEXT_ATTRIBUTE_REPLACEMENT_RECT]));
			}
			ret.push_back(ifo);
			ofs += Vector2(visual[i].width, 0);
		} else {
			ofs += Vector2(visual[i].width, 0);
		}
	}
	return ret;
}

float TLShapedAttributedString::get_cluster_face_size(int64_t p_index) const {

	if (!valid)
		const_cast<TLShapedAttributedString *>(this)->_shape_full_string();

	if (!valid)
		return base_size;

	if ((p_index < 0) || (p_index >= (int64_t)visual.size()))
		return base_size;

	int64_t _size = base_size;
	auto fattrib = format_attributes.find_closest(visual[p_index].start);
	if (fattrib && fattrib->get().has(TEXT_ATTRIBUTE_FONT_SIZE)) {
		_size = fattrib->get()[TEXT_ATTRIBUTE_FONT_SIZE];
	}

	return _size;
}

Vector2 TLShapedAttributedString::draw_cluster(RID p_canvas_item, const Point2 p_position, int64_t p_index, const Color p_modulate) {

	if (!valid)
		const_cast<TLShapedAttributedString *>(this)->_shape_full_string();

	if (!valid)
		return Vector2();

	if ((p_index < 0) || (p_index >= (int64_t)visual.size()))
		return Vector2();

	Vector2 ofs;
	if (visual[p_index].cl_type == (int)_CLUSTER_TYPE_HEX_BOX) {
		for (int64_t i = 0; i < (int64_t)visual[p_index].glyphs.size(); i++) {
			TLFontFace::draw_hexbox(p_canvas_item, p_position + ofs - Point2(0, visual[p_index].ascent), visual[p_index].glyphs[i].codepoint, p_modulate);
		}
	} else if (visual[p_index].cl_type == (int)_CLUSTER_TYPE_TEXT) {
		for (int64_t i = 0; i < (int64_t)visual[p_index].glyphs.size(); i++) {
			int64_t _size = base_size;
			auto fattrib = format_attributes.find_closest(visual[p_index].start);
			if (fattrib && fattrib->get().has(TEXT_ATTRIBUTE_FONT_SIZE)) {
				_size = fattrib->get()[TEXT_ATTRIBUTE_FONT_SIZE];
			}
			Color _color = p_modulate;
			auto attrib = style_attributes.find_closest(visual[p_index].start);
			if (attrib && attrib->get().has(TEXT_ATTRIBUTE_COLOR)) {
				_color = Color(attrib->get()[TEXT_ATTRIBUTE_COLOR]);
			}
			if (attrib && attrib->get().has(TEXT_ATTRIBUTE_HIGHLIGHT_COLOR)) {
				Color _hl_color = Color(attrib->get()[TEXT_ATTRIBUTE_HIGHLIGHT_COLOR]);
				Rect2 _rect = get_cluster_rect(p_index);
				RenderingServer::get_singleton()->canvas_item_add_rect(p_canvas_item, Rect2(p_position + _rect.position, _rect.size), _hl_color);
			}
			visual[p_index].font_face->draw_glyph(p_canvas_item, p_position + ofs + visual[p_index].glyphs[i].offset - Point2(0, visual[p_index].ascent), visual[i].glyphs[i].codepoint, _color, _size);

			if (attrib && attrib->get().has(TEXT_ATTRIBUTE_OUTLINE_COLOR)) {
				Color _outline_color = Color(attrib->get()[TEXT_ATTRIBUTE_OUTLINE_COLOR]);
				visual[p_index].font_face->draw_glyph_outline(p_canvas_item, p_position + ofs + visual[p_index].glyphs[i].offset - Point2(0, visual[p_index].ascent), visual[i].glyphs[i].codepoint, _outline_color, _size);
			}

			if (attrib && attrib->get().has(TEXT_ATTRIBUTE_UNDERLINE_COLOR)) {
				Color _ln_color = Color(attrib->get()[TEXT_ATTRIBUTE_UNDERLINE_COLOR]);
				float _width = 1.0f;
				if (attrib->get().has(TEXT_ATTRIBUTE_UNDERLINE_WIDTH)) {
					_width = float(attrib->get()[TEXT_ATTRIBUTE_UNDERLINE_WIDTH]);
				}
				RenderingServer::get_singleton()->canvas_item_add_line(p_canvas_item, p_position + ofs + Point2(0, descent), p_position + ofs + Point2(visual[p_index].width, descent), _ln_color, _width);
			}
			if (attrib && attrib->get().has(TEXT_ATTRIBUTE_STRIKETHROUGH_COLOR)) {
				Color _ln_color = Color(attrib->get()[TEXT_ATTRIBUTE_STRIKETHROUGH_COLOR]);
				float _width = 1.0f;
				if (attrib->get().has(TEXT_ATTRIBUTE_STRIKETHROUGH_WIDTH)) {
					_width = float(attrib->get()[TEXT_ATTRIBUTE_STRIKETHROUGH_WIDTH]);
				}
				RenderingServer::get_singleton()->canvas_item_add_line(p_canvas_item, p_position + ofs + Point2(0, 2 * visual[p_index].descent - visual[p_index].ascent), p_position + ofs + Point2(visual[p_index].width, 2 * visual[p_index].descent - visual[p_index].ascent), _ln_color, _width);
			}
			if (attrib && attrib->get().has(TEXT_ATTRIBUTE_OVERLINE_COLOR)) {
				Color _ln_color = Color(attrib->get()[TEXT_ATTRIBUTE_OVERLINE_COLOR]);
				float _width = 1.0f;
				if (attrib->get().has(TEXT_ATTRIBUTE_OVERLINE_WIDTH)) {
					_width = float(attrib->get()[TEXT_ATTRIBUTE_OVERLINE_WIDTH]);
				}
				RenderingServer::get_singleton()->canvas_item_add_line(p_canvas_item, p_position + ofs + Point2(0, -visual[p_index].ascent), p_position + ofs + Point2(visual[p_index].width, -visual[p_index].ascent), _ln_color, _width);
			}
			ofs += visual[p_index].glyphs[i].advance;
		}
	} else if (visual[p_index].cl_type == (int)_CLUSTER_TYPE_IMAGE) {
		auto attrib = format_attributes.find_closest(visual[p_index].start);
		if (attrib && attrib->get().has(TEXT_ATTRIBUTE_REPLACEMENT_IMAGE)) {
			Ref<Texture2D> image = Ref<Texture2D>(attrib->get()[TEXT_ATTRIBUTE_REPLACEMENT_IMAGE]);
			if (!image.is_null()) {
				image->draw(p_canvas_item, p_position + ofs + Point2(0, -visual[p_index].ascent));
			}
			ofs += Vector2(visual[p_index].width, 0);
		}
	} else if (visual[p_index].cl_type == (int)_CLUSTER_TYPE_RECT) {
		ofs += Vector2(visual[p_index].width, 0);
	} else if (visual[p_index].cl_type == (int)_CLUSTER_TYPE_SKIP) {
		ofs += Vector2(visual[p_index].width, 0);
	} else {
		WARN_PRINT("Invalid cluster type");
	}

	return ofs;
}

void TLShapedAttributedString::draw(RID p_canvas_item, const Point2 p_position, const Color p_modulate) {

	if (!valid)
		const_cast<TLShapedAttributedString *>(this)->_shape_full_string();

	if (!valid)
		return;
#ifdef DEBUG_DISPLAY_METRICS
	RenderingServer::get_singleton()->canvas_item_add_line(p_canvas_item, p_position + Point2(0, -ascent), p_position + Point2(width, -ascent), Color(1, 0, 0, 0.5), 1);
	RenderingServer::get_singleton()->canvas_item_add_line(p_canvas_item, p_position + Point2(0, 0), p_position + Point2(width, 0), Color(1, 1, 0, 0.5), 1);
	RenderingServer::get_singleton()->canvas_item_add_line(p_canvas_item, p_position + Point2(0, descent), p_position + Point2(width, descent), Color(0, 0, 1, 0.5), 1);
#endif

	Vector2 ofs;
	for (int64_t i = 0; i < (int64_t)visual.size(); i++) {
#ifdef DEBUG_DISPLAY_METRICS
		RenderingServer::get_singleton()->canvas_item_add_line(p_canvas_item, p_position + ofs + Point2(0, -visual[i].ascent), p_position + ofs + Point2(visual[i].width, -visual[i].ascent), Color(1, 0.5, 0.5, 0.2), 3);
#endif
		if (visual[i].cl_type == (int)_CLUSTER_TYPE_HEX_BOX) {
			for (int64_t j = 0; j < (int64_t)visual[i].glyphs.size(); j++) {
				TLFontFace::draw_hexbox(p_canvas_item, p_position + ofs - Point2(0, visual[i].ascent), visual[i].glyphs[j].codepoint, p_modulate);
				ofs += visual[i].glyphs[j].advance;
			}
		} else if (visual[i].cl_type == (int)_CLUSTER_TYPE_TEXT) {
			for (int64_t j = 0; j < (int64_t)visual[i].glyphs.size(); j++) {
				int64_t _size = base_size;
				auto fattrib = format_attributes.find_closest(visual[i].start);
				if (fattrib && fattrib->get().has(TEXT_ATTRIBUTE_FONT_SIZE)) {
					_size = fattrib->get()[TEXT_ATTRIBUTE_FONT_SIZE];
				}
				Color _color = p_modulate;
				auto attrib = style_attributes.find_closest(visual[i].start);
				if (attrib && attrib->get().has(TEXT_ATTRIBUTE_COLOR)) {
					Variant c = attrib->get()[TEXT_ATTRIBUTE_COLOR];
					_color = Color(c);
				}
				if (attrib && attrib->get().has(TEXT_ATTRIBUTE_HIGHLIGHT_COLOR)) {
					Color _hl_color = Color(attrib->get()[TEXT_ATTRIBUTE_HIGHLIGHT_COLOR]);
					Rect2 _rect = get_cluster_rect(i);
					RenderingServer::get_singleton()->canvas_item_add_rect(p_canvas_item, Rect2(p_position + _rect.position, _rect.size), _hl_color);
				}
				visual[i].font_face->draw_glyph(p_canvas_item, p_position + ofs + visual[i].glyphs[j].offset - Point2(0, visual[i].ascent), visual[i].glyphs[j].codepoint, _color, _size);

				if (attrib && attrib->get().has(TEXT_ATTRIBUTE_OUTLINE_COLOR)) {
					Color _outline_color = Color(attrib->get()[TEXT_ATTRIBUTE_OUTLINE_COLOR]);
					visual[i].font_face->draw_glyph_outline(p_canvas_item, p_position + ofs + visual[i].glyphs[j].offset - Point2(0, visual[i].ascent), visual[i].glyphs[j].codepoint, _outline_color, _size);
				}

				if (attrib && attrib->get().has(TEXT_ATTRIBUTE_UNDERLINE_COLOR)) {
					Color _ln_color = Color(attrib->get()[TEXT_ATTRIBUTE_UNDERLINE_COLOR]);
					float _width = 1.0f;
					if (attrib->get().has(TEXT_ATTRIBUTE_UNDERLINE_WIDTH)) {
						_width = float(attrib->get()[TEXT_ATTRIBUTE_UNDERLINE_WIDTH]);
					}
					RenderingServer::get_singleton()->canvas_item_add_line(p_canvas_item, p_position + ofs + Point2(0, descent), p_position + ofs + Point2(visual[i].width, descent), _ln_color, _width);
				}
				if (attrib && attrib->get().has(TEXT_ATTRIBUTE_STRIKETHROUGH_COLOR)) {
					Color _ln_color = Color(attrib->get()[TEXT_ATTRIBUTE_STRIKETHROUGH_COLOR]);
					float _width = 1.0f;
					if (attrib->get().has(TEXT_ATTRIBUTE_STRIKETHROUGH_WIDTH)) {
						_width = float(attrib->get()[TEXT_ATTRIBUTE_STRIKETHROUGH_WIDTH]);
					}

					RenderingServer::get_singleton()->canvas_item_add_line(p_canvas_item, p_position + ofs + Point2(0, 2 * visual[i].descent - visual[i].ascent), p_position + ofs + Point2(visual[i].width, 2 * visual[i].descent - visual[i].ascent), _ln_color, _width);
				}
				if (attrib && attrib->get().has(TEXT_ATTRIBUTE_OVERLINE_COLOR)) {
					Color _ln_color = Color(attrib->get()[TEXT_ATTRIBUTE_OVERLINE_COLOR]);
					float _width = 1.0f;
					if (attrib->get().has(TEXT_ATTRIBUTE_OVERLINE_WIDTH)) {
						_width = float(attrib->get()[TEXT_ATTRIBUTE_OVERLINE_WIDTH]);
					}

					RenderingServer::get_singleton()->canvas_item_add_line(p_canvas_item, p_position + ofs + Point2(0, -visual[i].ascent), p_position + ofs + Point2(visual[i].width, -visual[i].ascent), _ln_color, _width);
				}
				ofs += visual[i].glyphs[j].advance;
			}
		} else if (visual[i].cl_type == (int)_CLUSTER_TYPE_IMAGE) {
			auto attrib = format_attributes.find_closest(visual[i].start);
			if (attrib && attrib->get().has(TEXT_ATTRIBUTE_REPLACEMENT_IMAGE)) {
				Ref<Texture2D> image = Ref<Texture2D>(attrib->get()[TEXT_ATTRIBUTE_REPLACEMENT_IMAGE]);
				if (!image.is_null()) {
					image->draw(p_canvas_item, p_position + ofs + Point2(0, -visual[i].ascent));
				}
				ofs += Vector2(visual[i].width, 0);
			}
		} else if (visual[i].cl_type == (int)_CLUSTER_TYPE_RECT) {
			ofs += Vector2(visual[i].width, 0);
		} else if (visual[i].cl_type == (int)_CLUSTER_TYPE_SKIP) {
			ofs += Vector2(visual[i].width, 0);
		} else {
			WARN_PRINT("Invalid cluster type");
		}
	}
}

bool TLShapedAttributedString::has_attribute(int64_t p_attribute, int64_t p_index) const {

	if (p_index < 0 || p_index > data_size) {
		ERR_PRINT("Invalid substring range [" + String::num_int64(p_index) + "] / " + String::num_int64(data_size));
		ERR_FAIL_COND_V(true, false);
	}
	if (p_attribute <= TEXT_ATTRIBUTE_MAX_FORMAT_ATTRIBUTE) {
		auto attrib = format_attributes.find_closest(p_index);
		if (!attrib) {
			return false;
		}
		return (attrib->get().has((TextAttribute)p_attribute));
	} else {
		auto attrib = style_attributes.find_closest(p_index);
		if (!attrib) {
			return false;
		}
		return (attrib->get().has((TextAttribute)p_attribute));
	}
}

Variant TLShapedAttributedString::get_attribute(int64_t p_attribute, int64_t p_index) const {

	if (p_index < 0 || p_index > data_size) {
		ERR_PRINT("Invalid substring range [" + String::num_int64(p_index) + "] / " + String::num_int64(data_size));
		ERR_FAIL_COND_V(true, Variant());
	}
	if (p_attribute <= TEXT_ATTRIBUTE_MAX_FORMAT_ATTRIBUTE) {
		auto attrib = format_attributes.find_closest(p_index);
		if (!attrib) {
			return Variant();
		}
		if (attrib->get().has((TextAttribute)p_attribute)) {
			return attrib->get()[(TextAttribute)p_attribute];
		} else {
			return Variant();
		}
	} else {
		auto attrib = style_attributes.find_closest(p_index);
		if (!attrib) {
			return Variant();
		}
		if (attrib->get().has((TextAttribute)p_attribute)) {
			return attrib->get()[(TextAttribute)p_attribute];
		} else {
			return Variant();
		}
	}
}

void TLShapedAttributedString::load_attributes_dict(Array p_array) {

	clear_attributes();
	for (int64_t i = 0; i < (int64_t)p_array.size(); i++) {
		Dictionary item = p_array[i];
		if (item.has("index") && item.has("format")) {
			int64_t index = item["index"];
			Dictionary run = item["format"];
			Array keys = run.keys();
			format_attributes[index] = Map<TextAttribute, Variant>();
			for (int64_t j = 0; j < (int64_t)keys.size(); j++) {
				String key = keys[j];
				if (key == String("font")) {
					Ref<TLFontFamily> ff = run[key];
					format_attributes[index][TEXT_ATTRIBUTE_FONT] = ff;
				} else if (key == String("font_style")) {
					format_attributes[index][TEXT_ATTRIBUTE_FONT_STYLE] = run[key];
				} else if (key == String("font_size")) {
					format_attributes[index][TEXT_ATTRIBUTE_FONT_SIZE] = run[key];
				} else if (key == String("font_features")) {
					format_attributes[index][TEXT_ATTRIBUTE_FONT_FEATURES] = run[key];
				} else if (key == String("language")) {
					format_attributes[index][TEXT_ATTRIBUTE_LANGUAGE] = run[key];
				} else if (key == String("repl_image")) {
					format_attributes[index][TEXT_ATTRIBUTE_REPLACEMENT_IMAGE] = run[key];
				} else if (key == String("repl_rect")) {
					format_attributes[index][TEXT_ATTRIBUTE_REPLACEMENT_RECT] = run[key];
				} else if (key == String("repl_id")) {
					format_attributes[index][TEXT_ATTRIBUTE_REPLACEMENT_ID] = run[key];
				} else if (key == String("relp_valign")) {
					format_attributes[index][TEXT_ATTRIBUTE_REPLACEMENT_VALIGN] = run[key];
				}
			}
			run.clear();
		} else if (item.has("index") && item.has("style")) {
			int64_t index = item["index"];
			Dictionary run = item["style"];
			Array keys = run.keys();
			style_attributes[index] = Map<TextAttribute, Variant>();
			for (int64_t j = 0; j < (int64_t)keys.size(); j++) {
				String key = keys[j];
				String pref = "meta_";
				if (key == String("color")) {
					style_attributes[index][TEXT_ATTRIBUTE_COLOR] = run[key];
				} else if (key == String("out_color")) {
					style_attributes[index][TEXT_ATTRIBUTE_OUTLINE_COLOR] = run[key];
				} else if (key == String("ul_color")) {
					style_attributes[index][TEXT_ATTRIBUTE_UNDERLINE_COLOR] = run[key];
				} else if (key == String("ul_width")) {
					style_attributes[index][TEXT_ATTRIBUTE_UNDERLINE_WIDTH] = run[key];
				} else if (key == String("st_color")) {
					style_attributes[index][TEXT_ATTRIBUTE_STRIKETHROUGH_COLOR] = run[key];
				} else if (key == String("st_width")) {
					style_attributes[index][TEXT_ATTRIBUTE_STRIKETHROUGH_WIDTH] = run[key];
				} else if (key == String("ol_color")) {
					style_attributes[index][TEXT_ATTRIBUTE_OVERLINE_COLOR] = run[key];
				} else if (key == String("ol_width")) {
					style_attributes[index][TEXT_ATTRIBUTE_OVERLINE_WIDTH] = run[key];
				} else if (key == String("hl_color")) {
					style_attributes[index][TEXT_ATTRIBUTE_HIGHLIGHT_COLOR] = run[key];
				} else if (key.begins_with(pref)) {
					style_attributes[index][(TextAttribute)key.substr(5, key.length() - 5).to_int()] = run[key];
				}
			}
			run.clear();
		}
	}
	_reconnect_fonts();
	_clear_props();
	emit_signal("string_changed");
}

Array TLShapedAttributedString::save_attributes_dict() const {

	Array ret;
	for (auto it = format_attributes.front(); it; it = it->next()) {
		Dictionary item;
		Dictionary run;
		for (auto sit = it->get().front(); sit; sit = sit->next()) {
			switch (sit->key()) {
				case TEXT_ATTRIBUTE_FONT: {
					run["font"] = sit->get();
				} break;
				case TEXT_ATTRIBUTE_FONT_STYLE: {
					run["font_style"] = sit->get();
				} break;
				case TEXT_ATTRIBUTE_FONT_SIZE: {
					run["font_size"] = sit->get();
				} break;
				case TEXT_ATTRIBUTE_FONT_FEATURES: {
					run["font_features"] = sit->get();
				} break;
				case TEXT_ATTRIBUTE_LANGUAGE: {
					run["language"] = sit->get();
				} break;
				case TEXT_ATTRIBUTE_REPLACEMENT_IMAGE: {
					run["repl_image"] = sit->get();
				} break;
				case TEXT_ATTRIBUTE_REPLACEMENT_RECT: {
					run["repl_rect"] = sit->get();
				} break;
				case TEXT_ATTRIBUTE_REPLACEMENT_ID: {
					run["repl_id"] = sit->get();
				} break;
				case TEXT_ATTRIBUTE_REPLACEMENT_VALIGN: {
					run["relp_valign"] = sit->get();
				} break;
				default: {
					ERR_PRINT("Invalid format attribute");
				} break;
			}
		}
		item["index"] = String::num_int64(it->key());
		item["format"] = run;
		ret.push_back(item);
	}

	for (auto it = style_attributes.front(); it; it = it->next()) {
		Dictionary item;
		Dictionary run;
		for (auto sit = it->get().front(); sit; sit = sit->next()) {
			switch (sit->key()) {
				case TEXT_ATTRIBUTE_COLOR: {
					run["color"] = ((Color)(sit->get())).to_html();
				} break;
				case TEXT_ATTRIBUTE_OUTLINE_COLOR: {
					run["out_color"] = ((Color)(sit->get())).to_html();
				} break;
				case TEXT_ATTRIBUTE_UNDERLINE_COLOR: {
					run["ul_color"] = ((Color)(sit->get())).to_html();
				} break;
				case TEXT_ATTRIBUTE_UNDERLINE_WIDTH: {
					run["ul_width"] = sit->get();
				} break;
				case TEXT_ATTRIBUTE_STRIKETHROUGH_COLOR: {
					run["st_color"] = ((Color)(sit->get())).to_html();
				} break;
				case TEXT_ATTRIBUTE_STRIKETHROUGH_WIDTH: {
					run["st_width"] = sit->get();
				} break;
				case TEXT_ATTRIBUTE_OVERLINE_COLOR: {
					run["ol_color"] = ((Color)(sit->get())).to_html();
				} break;
				case TEXT_ATTRIBUTE_OVERLINE_WIDTH: {
					run["ol_width"] = sit->get();
				} break;
				case TEXT_ATTRIBUTE_HIGHLIGHT_COLOR: {
					run["hl_color"] = ((Color)(sit->get())).to_html();
				} break;
				default: {
					run["meta_" + String::num_int64(sit->key())] = sit->get();
				} break;
			}
		}
		item["index"] = String::num_int64(it->key());
		item["style"] = run;
		ret.push_back(item);
	}
	return ret;
}

int64_t TLShapedAttributedString::get_attribute_start(int64_t p_attribute, int64_t p_index) const {

	if (p_index < 0 || p_index > data_size) {
		ERR_PRINT("Invalid substring range [" + String::num_int64(p_index) + "] / " + String::num_int64(data_size));
		ERR_FAIL_COND_V(true, -1);
	}
	if (p_attribute <= TEXT_ATTRIBUTE_MAX_FORMAT_ATTRIBUTE) {
		auto attrib = format_attributes.find_closest(p_index);
		if (!attrib) {
			ERR_PRINT("Attribute not set");
			ERR_FAIL_COND_V(true, -1);
		}
		return attrib->key();
	} else {
		auto attrib = style_attributes.find_closest(p_index);
		if (!attrib) {
			ERR_PRINT("Attribute not set");
			ERR_FAIL_COND_V(true, -1);
		}
		return attrib->key();
	}
}

int64_t TLShapedAttributedString::get_attribute_end(int64_t p_attribute, int64_t p_index) const {

	if (p_index < 0 || p_index > data_size) {
		ERR_PRINT("Invalid substring range [" + String::num_int64(p_index) + "] / " + String::num_int64(data_size));
		ERR_FAIL_COND_V(true, -1);
	}
	if (p_attribute <= TEXT_ATTRIBUTE_MAX_FORMAT_ATTRIBUTE) {
		auto attrib = format_attributes.find_closest(p_index);
		if (!attrib) {
			ERR_PRINT("Attribute not set");
			ERR_FAIL_COND_V(true, -1);
		}
		attrib = attrib->next();
		if (!attrib) {
			ERR_PRINT("Attribute not set");
			ERR_FAIL_COND_V(true, -1);
		}
		return attrib->key();
	} else {
		auto attrib = style_attributes.find_closest(p_index);
		if (!attrib) {
			ERR_PRINT("Attribute not set");
			ERR_FAIL_COND_V(true, -1);
		}
		attrib = attrib->next();
		if (!attrib) {
			ERR_PRINT("Attribute not set");
			ERR_FAIL_COND_V(true, -1);
		}
		return attrib->key();
	}
}

void TLShapedAttributedString::replace_text(int64_t p_start, int64_t p_end, const String p_text) {

	int32_t _len = data_size;

	TLShapedString::replace_text(p_start, p_end, p_text);

	_len = data_size - _len;

	_disconnect_fonts();

	Map<int, Map<TextAttribute, Variant> > new_format_attributes;
	for (auto it = format_attributes.front(); it; it = it->next()) {
		if (it->key() <= p_start) {
			new_format_attributes[it->key()] = it->get();
		}
		if (it->key() >= p_end)
			new_format_attributes[it->key() + _len] = it->get();
	}
	format_attributes = new_format_attributes;

	_reconnect_fonts();

	Map<int, Map<TextAttribute, Variant> > new_style_attributes;
	for (auto it = style_attributes.front(); it; it = it->next()) {
		if (it->key() <= p_start)
			new_style_attributes[it->key()] = it->get();
		if (it->key() >= p_end)
			new_style_attributes[it->key() + _len] = it->get();
	}
	style_attributes = new_style_attributes;

	_clear_props();
	emit_signal("string_changed");
}

void TLShapedAttributedString::replace_utf8(int64_t p_start, int64_t p_end, const PackedByteArray p_text) {

	int32_t _len = data_size;

	TLShapedString::replace_utf8(p_start, p_end, p_text);

	_len = data_size - _len;

	_disconnect_fonts();

	Map<int, Map<TextAttribute, Variant> > new_format_attributes;
	for (auto it = format_attributes.front(); it; it = it->next()) {
		if (it->key() <= p_start) {
			new_format_attributes[it->key()] = it->get();
		}
		if (it->key() >= p_end)
			new_format_attributes[it->key() + _len] = it->get();
	}
	format_attributes = new_format_attributes;

	_reconnect_fonts();

	Map<int, Map<TextAttribute, Variant> > new_style_attributes;
	for (auto it = style_attributes.front(); it; it = it->next()) {
		if (it->key() <= p_start)
			new_style_attributes[it->key()] = it->get();
		if (it->key() >= p_end)
			new_style_attributes[it->key() + _len] = it->get();
	}
	style_attributes = new_style_attributes;

	_clear_props();
	emit_signal("string_changed");
}

void TLShapedAttributedString::replace_utf16(int64_t p_start, int64_t p_end, const PackedByteArray p_text) {

	int32_t _len = data_size;

	TLShapedString::replace_utf16(p_start, p_end, p_text);

	_len = data_size - _len;

	_disconnect_fonts();

	Map<int, Map<TextAttribute, Variant> > new_format_attributes;
	for (auto it = format_attributes.front(); it; it = it->next()) {
		if (it->key() <= p_start) {
			new_format_attributes[it->key()] = it->get();
		}
		if (it->key() >= p_end)
			new_format_attributes[it->key() + _len] = it->get();
	}
	format_attributes = new_format_attributes;

	_reconnect_fonts();

	Map<int, Map<TextAttribute, Variant> > new_style_attributes;
	for (auto it = style_attributes.front(); it; it = it->next()) {
		if (it->key() <= p_start)
			new_style_attributes[it->key()] = it->get();
		if (it->key() >= p_end)
			new_style_attributes[it->key() + _len] = it->get();
	}
	style_attributes = new_style_attributes;

	_clear_props();
	emit_signal("string_changed");
}

void TLShapedAttributedString::replace_utf32(int64_t p_start, int64_t p_end, const PackedByteArray p_text) {

	int32_t _len = data_size;

	TLShapedString::replace_utf32(p_start, p_end, p_text);

	_len = data_size - _len;

	_disconnect_fonts();

	Map<int, Map<TextAttribute, Variant> > new_format_attributes;
	for (auto it = format_attributes.front(); it; it = it->next()) {
		if (it->key() <= p_start) {
			new_format_attributes[it->key()] = it->get();
		}
		if (it->key() >= p_end)
			new_format_attributes[it->key() + _len] = it->get();
	}
	format_attributes = new_format_attributes;

	_reconnect_fonts();

	Map<int, Map<TextAttribute, Variant> > new_style_attributes;
	for (auto it = style_attributes.front(); it; it = it->next()) {
		if (it->key() <= p_start)
			new_style_attributes[it->key()] = it->get();
		if (it->key() >= p_end)
			new_style_attributes[it->key() + _len] = it->get();
	}
	style_attributes = new_style_attributes;

	_clear_props();
	emit_signal("string_changed");
}

void TLShapedAttributedString::replace_sstring(int64_t p_start, int64_t p_end, Ref<TLShapedString> p_text) {

	if ((p_start < 0) || (p_start > p_end) || (p_end > data_size)) {
		ERR_PRINT("Invalid range");
		return;
	}

	if (p_text.is_null()) {
		ERR_PRINT("Invalid string");
		return;
	}

	int32_t _len = data_size;

	TLShapedString::replace_sstring(p_start, p_end, p_text);

	_len = data_size - _len;

	TLShapedAttributedString *at_ref = cast_to<TLShapedAttributedString>(*p_text);

	_disconnect_fonts();

	Map<int, Map<TextAttribute, Variant> > new_format_attributes;
	for (auto it = format_attributes.front(); it; it = it->next()) {
		if (it->key() <= p_start) {
			new_format_attributes[it->key()] = it->get();
		}
		if (it->key() >= p_end)
			new_format_attributes[it->key() + _len] = it->get();
	}
	if (at_ref) {
		for (auto it = at_ref->format_attributes.front(); it; it = it->next()) {
			new_format_attributes[it->key() + p_start] = it->get();
		}
	}
	format_attributes = new_format_attributes;

	_reconnect_fonts();

	Map<int, Map<TextAttribute, Variant> > new_style_attributes;
	for (auto it = style_attributes.front(); it; it = it->next()) {
		if (it->key() <= p_start)
			new_style_attributes[it->key()] = it->get();
		if (it->key() >= p_end)
			new_style_attributes[it->key() + _len] = it->get();
	}
	if (at_ref) {
		for (auto it = at_ref->style_attributes.front(); it; it = it->next()) {
			new_style_attributes[it->key() + p_start] = it->get();
		}
	}
	style_attributes = new_style_attributes;

	_clear_props();
	emit_signal("string_changed");
}

void TLShapedAttributedString::add_sstring(Ref<TLShapedString> p_ref) {

	if (p_ref.is_null()) {
		ERR_PRINT("Invalid string");
		return;
	}

	TLShapedAttributedString *at_ref = cast_to<TLShapedAttributedString>(*p_ref);
	if (!at_ref) {
		TLShapedString::add_sstring(p_ref);
		return;
	}

	if (at_ref->data_size == 0)
		return;

	_clear_props();

	int32_t _len = data_size;

	//copy data
	data = (UChar *)std::realloc(data, (data_size + at_ref->data_size + 1) * sizeof(UChar));
	std::memcpy(&data[data_size], at_ref->data, at_ref->data_size * sizeof(UChar));
	data[data_size + at_ref->data_size] = 0x0000;
	data_size = data_size + at_ref->data_size;

	//copy attributes
	_disconnect_fonts();
	for (auto it = at_ref->format_attributes.front(); it; it = it->next()) {
		format_attributes[it->key() + _len] = it->get();
	}
	_reconnect_fonts();
	for (auto it = at_ref->style_attributes.front(); it; it = it->next()) {
		style_attributes[it->key() + _len] = it->get();
	}

	_clear_props();

	emit_signal("string_changed");
}

void TLShapedAttributedString::reject_attribute() {
	remove_attributes(_edited_attrib_start, _edited_attrib_end);
	_edited_attrib = TEXT_ATTRIBUTE_COLOR;
	_edited_attrib_value = Variant();
	_edited_attrib_start = 0;
	_edited_attrib_end = 0;
#ifdef GODOT_MODULE
	_change_notify();
#else
	property_list_changed_notify();
#endif
}

void TLShapedAttributedString::commit_attribute() {
	add_attribute(_edited_attrib, _edited_attrib_value, _edited_attrib_start, _edited_attrib_end);
	_edited_attrib = TEXT_ATTRIBUTE_COLOR;
	_edited_attrib_value = Variant();
	_edited_attrib_start = 0;
	_edited_attrib_end = 0;
#ifdef GODOT_MODULE
	_change_notify();
#else
	property_list_changed_notify();
#endif
}

#ifdef GODOT_MODULE
bool TLShapedAttributedString::_set(const StringName &p_name, const Variant &p_value) {
	String name = p_name;
	if (name == "attribute/type") {
		int64_t v = p_value;
		switch (v) {
			case 0: _edited_attrib = TEXT_ATTRIBUTE_FONT; break;
			case 1: _edited_attrib = TEXT_ATTRIBUTE_FONT_STYLE; break;
			case 2: _edited_attrib = TEXT_ATTRIBUTE_FONT_SIZE; break;
			case 3: _edited_attrib = TEXT_ATTRIBUTE_FONT_FEATURES; break;
			case 4: _edited_attrib = TEXT_ATTRIBUTE_LANGUAGE; break;
			case 5: _edited_attrib = TEXT_ATTRIBUTE_REPLACEMENT_IMAGE; break;
			case 6: _edited_attrib = TEXT_ATTRIBUTE_REPLACEMENT_RECT; break;
			case 7: _edited_attrib = TEXT_ATTRIBUTE_REPLACEMENT_ID; break;
			case 8: _edited_attrib = TEXT_ATTRIBUTE_REPLACEMENT_VALIGN; break;
			case 9: _edited_attrib = TEXT_ATTRIBUTE_COLOR; break;
			case 10: _edited_attrib = TEXT_ATTRIBUTE_OUTLINE_COLOR; break;
			case 11: _edited_attrib = TEXT_ATTRIBUTE_UNDERLINE_COLOR; break;
			case 12: _edited_attrib = TEXT_ATTRIBUTE_UNDERLINE_WIDTH; break;
			case 13: _edited_attrib = TEXT_ATTRIBUTE_STRIKETHROUGH_COLOR; break;
			case 14: _edited_attrib = TEXT_ATTRIBUTE_STRIKETHROUGH_WIDTH; break;
			case 15: _edited_attrib = TEXT_ATTRIBUTE_OVERLINE_COLOR; break;
			case 16: _edited_attrib = TEXT_ATTRIBUTE_OVERLINE_WIDTH; break;
			case 17: _edited_attrib = TEXT_ATTRIBUTE_HIGHLIGHT_COLOR; break;
			default: return false;
		}
		_change_notify();
		return true;
	} else if (name == "attribute/value") {
		if (_edited_attrib == TEXT_ATTRIBUTE_FONT) {
			Object *p_obj = p_value;
			Ref<TLFontFamily> v = Ref<TLFontFamily>(Object::cast_to<TLFontFamily>(p_obj));
			if (v.is_null()) return false;
		} else if (_edited_attrib == TEXT_ATTRIBUTE_REPLACEMENT_IMAGE) {
			Object *p_obj = p_value;
			Ref<Texture2D> v = Ref<Texture2D>(Object::cast_to<Texture2D>(p_obj));
			if (v.is_null()) return false;
		}
		_edited_attrib_value = p_value;
		_change_notify();
		return true;
	} else if (name == "attribute/start") {
		_edited_attrib_start = (int64_t)p_value;
		if (_edited_attrib_start < 0) _edited_attrib_start = 0;
		if (_edited_attrib_start > data_size) _edited_attrib_start = data_size;
		if (_edited_attrib_start > _edited_attrib_end) _edited_attrib_end = _edited_attrib_start;
		_change_notify();
		return true;
	} else if (name == "attribute/end") {
		_edited_attrib_end = (int64_t)p_value;
		if (_edited_attrib_end < 0) _edited_attrib_end = 0;
		if (_edited_attrib_end > data_size) _edited_attrib_end = data_size;
		if (_edited_attrib_end < _edited_attrib_start) _edited_attrib_start = _edited_attrib_end;
		_change_notify();
		return true;
	} else if (name == "attribute_dict") {
		load_attributes_dict(p_value);
		return true;
	}
	return false;
}

bool TLShapedAttributedString::_get(const StringName &p_name, Variant &r_ret) const {
	String name = p_name;
	if (name == "attribute/type") {
		switch (_edited_attrib) {
			case TEXT_ATTRIBUTE_FONT: r_ret = 0; break;
			case TEXT_ATTRIBUTE_FONT_STYLE: r_ret = 1; break;
			case TEXT_ATTRIBUTE_FONT_SIZE: r_ret = 2; break;
			case TEXT_ATTRIBUTE_FONT_FEATURES: r_ret = 3; break;
			case TEXT_ATTRIBUTE_LANGUAGE: r_ret = 4; break;
			case TEXT_ATTRIBUTE_REPLACEMENT_IMAGE: r_ret = 5; break;
			case TEXT_ATTRIBUTE_REPLACEMENT_RECT: r_ret = 6; break;
			case TEXT_ATTRIBUTE_REPLACEMENT_ID: r_ret = 7; break;
			case TEXT_ATTRIBUTE_REPLACEMENT_VALIGN: r_ret = 8; break;
			case TEXT_ATTRIBUTE_COLOR: r_ret = 9; break;
			case TEXT_ATTRIBUTE_OUTLINE_COLOR: r_ret = 10; break;
			case TEXT_ATTRIBUTE_UNDERLINE_COLOR: r_ret = 11; break;
			case TEXT_ATTRIBUTE_UNDERLINE_WIDTH: r_ret = 12; break;
			case TEXT_ATTRIBUTE_STRIKETHROUGH_COLOR: r_ret = 13; break;
			case TEXT_ATTRIBUTE_STRIKETHROUGH_WIDTH: r_ret = 14; break;
			case TEXT_ATTRIBUTE_OVERLINE_COLOR: r_ret = 15; break;
			case TEXT_ATTRIBUTE_OVERLINE_WIDTH: r_ret = 16; break;
			case TEXT_ATTRIBUTE_HIGHLIGHT_COLOR: r_ret = 17; break;
			default: return false;
		}
		return true;
	} else if (name == "attribute/value") {
		r_ret = _edited_attrib_value;
		return true;
	} else if (name == "attribute/start") {
		r_ret = _edited_attrib_start;
		return true;
	} else if (name == "attribute/end") {
		r_ret = _edited_attrib_end;
		return true;
	} else if (name == "attribute_dict") {
		r_ret = save_attributes_dict();
		return true;
	}
	return false;
}

void TLShapedAttributedString::_get_property_list(List<PropertyInfo> *p_list) const {
	p_list->push_back(PropertyInfo(Variant::INT, "attribute/type", PROPERTY_HINT_ENUM, "Font,Font Style,Font Size,Font Features,Language,Replacement Image,Replacement Rect,Replacement Id,Replacement Valign,Color,Outline Color,Underline Color,Underline Width,Strikethrough Color,Strikethrough Width,Overline Color,Overline Width,Highlight Color", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_INTERNAL));
	switch (_edited_attrib) {
		case TEXT_ATTRIBUTE_FONT: {
			p_list->push_back(PropertyInfo(Variant::OBJECT, "attribute/value", PROPERTY_HINT_RESOURCE_TYPE, "TLFontFace", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_INTERNAL));
		} break;
		case TEXT_ATTRIBUTE_LANGUAGE:
		case TEXT_ATTRIBUTE_REPLACEMENT_ID:
		case TEXT_ATTRIBUTE_FONT_FEATURES:
		case TEXT_ATTRIBUTE_FONT_STYLE: {
			p_list->push_back(PropertyInfo(Variant::STRING, "attribute/value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_INTERNAL));
		} break;
		case TEXT_ATTRIBUTE_FONT_SIZE:
		case TEXT_ATTRIBUTE_UNDERLINE_WIDTH:
		case TEXT_ATTRIBUTE_STRIKETHROUGH_WIDTH:
		case TEXT_ATTRIBUTE_OVERLINE_WIDTH: {
			p_list->push_back(PropertyInfo(Variant::INT, "attribute/value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_INTERNAL));
		} break;
		case TEXT_ATTRIBUTE_REPLACEMENT_IMAGE: {
			p_list->push_back(PropertyInfo(Variant::OBJECT, "attribute/value", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_INTERNAL));
		} break;
		case TEXT_ATTRIBUTE_REPLACEMENT_RECT: {
			p_list->push_back(PropertyInfo(Variant::VECTOR2, "attribute/value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_INTERNAL));
		} break;
		case TEXT_ATTRIBUTE_REPLACEMENT_VALIGN: {
			p_list->push_back(PropertyInfo(Variant::INT, "attribute/value", PROPERTY_HINT_ENUM, "Top,Center,Bottom", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_INTERNAL));
		} break;
		case TEXT_ATTRIBUTE_OUTLINE_COLOR:
		case TEXT_ATTRIBUTE_UNDERLINE_COLOR:
		case TEXT_ATTRIBUTE_STRIKETHROUGH_COLOR:
		case TEXT_ATTRIBUTE_OVERLINE_COLOR:
		case TEXT_ATTRIBUTE_HIGHLIGHT_COLOR:
		case TEXT_ATTRIBUTE_COLOR: {
			p_list->push_back(PropertyInfo(Variant::COLOR, "attribute/value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_INTERNAL));
		} break;
		default: {
			p_list->push_back(PropertyInfo(Variant::NIL, "attribute/value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_INTERNAL));
		} break;
	}
	p_list->push_back(PropertyInfo(Variant::INT, "attribute/start", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_INTERNAL));
	p_list->push_back(PropertyInfo(Variant::INT, "attribute/end", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_INTERNAL));
	p_list->push_back(PropertyInfo(Variant::NIL, "attribute/_commit", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_INTERNAL));

	p_list->push_back(PropertyInfo(Variant::ARRAY, "attribute_dict", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_STORAGE));
}

void TLShapedAttributedString::_bind_methods() {

	//Attribute control
	ClassDB::bind_method(D_METHOD("commit_attribute"), &TLShapedAttributedString::commit_attribute);
	ClassDB::bind_method(D_METHOD("reject_attribute"), &TLShapedAttributedString::reject_attribute);

	ClassDB::bind_method(D_METHOD("add_attribute", "attribute", "value", "start", "end"), &TLShapedAttributedString::add_attribute);
	ClassDB::bind_method(D_METHOD("remove_attribute", "attribute", "start", "end"), &TLShapedAttributedString::remove_attribute);
	ClassDB::bind_method(D_METHOD("has_attribute", "attribute", "index"), &TLShapedAttributedString::has_attribute);
	ClassDB::bind_method(D_METHOD("get_attribute", "attribute", "index"), &TLShapedAttributedString::get_attribute);
	ClassDB::bind_method(D_METHOD("get_attribute_start", "attribute", "index"), &TLShapedAttributedString::get_attribute_start);
	ClassDB::bind_method(D_METHOD("get_attribute_end", "attribute", "index"), &TLShapedAttributedString::get_attribute_end);
	ClassDB::bind_method(D_METHOD("remove_attributes", "start", "end"), &TLShapedAttributedString::remove_attributes);
	ClassDB::bind_method(D_METHOD("clear_attributes"), &TLShapedAttributedString::clear_attributes);

	ClassDB::bind_method(D_METHOD("get_embedded_rects"), &TLShapedAttributedString::get_embedded_rects);

	ClassDB::bind_method(D_METHOD("load_attributes_dict", "array"), &TLShapedAttributedString::load_attributes_dict);
	ClassDB::bind_method(D_METHOD("save_attributes_dict"), &TLShapedAttributedString::save_attributes_dict);

	BIND_ENUM_CONSTANT(TEXT_ATTRIBUTE_FONT);
	BIND_ENUM_CONSTANT(TEXT_ATTRIBUTE_FONT_STYLE);
	BIND_ENUM_CONSTANT(TEXT_ATTRIBUTE_FONT_SIZE);
	BIND_ENUM_CONSTANT(TEXT_ATTRIBUTE_FONT_FEATURES);
	BIND_ENUM_CONSTANT(TEXT_ATTRIBUTE_LANGUAGE);
	BIND_ENUM_CONSTANT(TEXT_ATTRIBUTE_REPLACEMENT_IMAGE);
	BIND_ENUM_CONSTANT(TEXT_ATTRIBUTE_REPLACEMENT_RECT);
	BIND_ENUM_CONSTANT(TEXT_ATTRIBUTE_REPLACEMENT_ID);
	BIND_ENUM_CONSTANT(TEXT_ATTRIBUTE_REPLACEMENT_VALIGN);
	BIND_ENUM_CONSTANT(TEXT_ATTRIBUTE_COLOR);
	BIND_ENUM_CONSTANT(TEXT_ATTRIBUTE_OUTLINE_COLOR);
	BIND_ENUM_CONSTANT(TEXT_ATTRIBUTE_UNDERLINE_COLOR);
	BIND_ENUM_CONSTANT(TEXT_ATTRIBUTE_UNDERLINE_WIDTH);
	BIND_ENUM_CONSTANT(TEXT_ATTRIBUTE_STRIKETHROUGH_COLOR);
	BIND_ENUM_CONSTANT(TEXT_ATTRIBUTE_STRIKETHROUGH_WIDTH);
	BIND_ENUM_CONSTANT(TEXT_ATTRIBUTE_OVERLINE_COLOR);
	BIND_ENUM_CONSTANT(TEXT_ATTRIBUTE_OVERLINE_WIDTH);
	BIND_ENUM_CONSTANT(TEXT_ATTRIBUTE_HIGHLIGHT_COLOR);
	BIND_ENUM_CONSTANT(TEXT_ATTRIBUTE_META);

	BIND_ENUM_CONSTANT(TEXT_VALIGN_TOP);
	BIND_ENUM_CONSTANT(TEXT_VALIGN_CENTER);
	BIND_ENUM_CONSTANT(TEXT_VALIGN_BOTTOM);
}
#else

bool TLShapedAttributedString::_set(String p_name, Variant p_value) {
	String name = p_name;
	if (name == "attribute/type") {
		int64_t v = p_value;
		switch (v) {
			case 0: _edited_attrib = TEXT_ATTRIBUTE_FONT; break;
			case 1: _edited_attrib = TEXT_ATTRIBUTE_FONT_STYLE; break;
			case 2: _edited_attrib = TEXT_ATTRIBUTE_FONT_SIZE; break;
			case 3: _edited_attrib = TEXT_ATTRIBUTE_FONT_FEATURES; break;
			case 4: _edited_attrib = TEXT_ATTRIBUTE_LANGUAGE; break;
			case 5: _edited_attrib = TEXT_ATTRIBUTE_REPLACEMENT_IMAGE; break;
			case 6: _edited_attrib = TEXT_ATTRIBUTE_REPLACEMENT_RECT; break;
			case 7: _edited_attrib = TEXT_ATTRIBUTE_REPLACEMENT_ID; break;
			case 8: _edited_attrib = TEXT_ATTRIBUTE_REPLACEMENT_VALIGN; break;
			case 9: _edited_attrib = TEXT_ATTRIBUTE_COLOR; break;
			case 10: _edited_attrib = TEXT_ATTRIBUTE_OUTLINE_COLOR; break;
			case 11: _edited_attrib = TEXT_ATTRIBUTE_UNDERLINE_COLOR; break;
			case 12: _edited_attrib = TEXT_ATTRIBUTE_UNDERLINE_WIDTH; break;
			case 13: _edited_attrib = TEXT_ATTRIBUTE_STRIKETHROUGH_COLOR; break;
			case 14: _edited_attrib = TEXT_ATTRIBUTE_STRIKETHROUGH_WIDTH; break;
			case 15: _edited_attrib = TEXT_ATTRIBUTE_OVERLINE_COLOR; break;
			case 16: _edited_attrib = TEXT_ATTRIBUTE_OVERLINE_WIDTH; break;
			case 17: _edited_attrib = TEXT_ATTRIBUTE_HIGHLIGHT_COLOR; break;
			default: return false;
		}
		property_list_changed_notify();
		return true;
	} else if (name == "attribute/value") {
		if (_edited_attrib == TEXT_ATTRIBUTE_FONT) {
			Object *p_obj = p_value;
			Ref<TLFontFamily> v = Ref<TLFontFamily>(Object::cast_to<TLFontFamily>(p_obj));
			if (v.is_null()) return false;
		} else if (_edited_attrib == TEXT_ATTRIBUTE_REPLACEMENT_IMAGE) {
			Object *p_obj = p_value;
			Ref<Texture2D> v = Ref<Texture2D>(Object::cast_to<Texture2D>(p_obj));
			if (v.is_null()) return false;
		}
		_edited_attrib_value = p_value;
		property_list_changed_notify();
		return true;
	} else if (name == "attribute/start") {
		_edited_attrib_start = (int64_t)p_value;
		if (_edited_attrib_start < 0) _edited_attrib_start = 0;
		if (_edited_attrib_start > data_size) _edited_attrib_start = data_size;
		if (_edited_attrib_start > _edited_attrib_end) _edited_attrib_end = _edited_attrib_start;
		property_list_changed_notify();
		return true;
	} else if (name == "attribute/end") {
		_edited_attrib_end = (int64_t)p_value;
		if (_edited_attrib_end < 0) _edited_attrib_end = 0;
		if (_edited_attrib_end > data_size) _edited_attrib_end = data_size;
		if (_edited_attrib_end < _edited_attrib_start) _edited_attrib_start = _edited_attrib_end;
		property_list_changed_notify();
		return true;
	} else if (name == "attribute_dict") {
		load_attributes_dict(p_value);
		return true;
	}
	return false;
}

Variant TLShapedAttributedString::_get(String p_name) const {
	String name = p_name;
	if (name == "attribute/type") {
		switch (_edited_attrib) {
			case TEXT_ATTRIBUTE_FONT: return 0;
			case TEXT_ATTRIBUTE_FONT_STYLE: return 1;
			case TEXT_ATTRIBUTE_FONT_SIZE: return 2;
			case TEXT_ATTRIBUTE_FONT_FEATURES: return 3;
			case TEXT_ATTRIBUTE_LANGUAGE: return 4;
			case TEXT_ATTRIBUTE_REPLACEMENT_IMAGE: return 5;
			case TEXT_ATTRIBUTE_REPLACEMENT_RECT: return 6;
			case TEXT_ATTRIBUTE_REPLACEMENT_ID: return 7;
			case TEXT_ATTRIBUTE_REPLACEMENT_VALIGN: return 8;
			case TEXT_ATTRIBUTE_COLOR: return 9;
			case TEXT_ATTRIBUTE_OUTLINE_COLOR: return 10;
			case TEXT_ATTRIBUTE_UNDERLINE_COLOR: return 11;
			case TEXT_ATTRIBUTE_UNDERLINE_WIDTH: return 12;
			case TEXT_ATTRIBUTE_STRIKETHROUGH_COLOR: return 13;
			case TEXT_ATTRIBUTE_STRIKETHROUGH_WIDTH: return 14;
			case TEXT_ATTRIBUTE_OVERLINE_COLOR: return 15;
			case TEXT_ATTRIBUTE_OVERLINE_WIDTH: return 16;
			case TEXT_ATTRIBUTE_HIGHLIGHT_COLOR: return 17;
			default: return Variant();
		}
	} else if (name == "attribute/value") {
		return _edited_attrib_value;
	} else if (name == "attribute/start") {
		return _edited_attrib_start;
	} else if (name == "attribute/end") {
		return _edited_attrib_end;
	} else if (name == "attribute_dict") {
		return save_attributes_dict();
	}
	return Variant();
}

Array TLShapedAttributedString::_get_property_list() const {
	Array ret;
	{
		Dictionary prop;
		prop["name"] = "attribute/type";
		prop["type"] = GlobalConstants::TYPE_INT;
		prop["hint"] = GlobalConstants::PROPERTY_HINT_ENUM;
		prop["hint_string"] = "Font,Font Style,Font Size,Font Features,Language,Replacement Image,Replacement Rect,Replacement Id,Replacement Valign,Color,Outline Color,Underline Color,Underline Width,Strikethrough Color,Strikethrough Width,Overline Color,Overline Width,Highlight Color";
		prop["usage"] = GlobalConstants::PROPERTY_USAGE_EDITOR;
		ret.push_back(prop);
	}
	{
		Dictionary prop;
		prop["name"] = "attribute/value";
		switch (_edited_attrib) {
			case TEXT_ATTRIBUTE_FONT: {
				prop["type"] = GlobalConstants::TYPE_OBJECT;
				prop["hint"] = GlobalConstants::PROPERTY_HINT_RESOURCE_TYPE;
				prop["hint_string"] = "TLFontFamily";
			} break;
			case TEXT_ATTRIBUTE_LANGUAGE:
			case TEXT_ATTRIBUTE_REPLACEMENT_ID:
			case TEXT_ATTRIBUTE_FONT_FEATURES:
			case TEXT_ATTRIBUTE_FONT_STYLE: {
				prop["type"] = GlobalConstants::TYPE_STRING;
				prop["hint"] = GlobalConstants::PROPERTY_HINT_NONE;
				prop["hint_string"] = "";
			} break;
			case TEXT_ATTRIBUTE_FONT_SIZE:
			case TEXT_ATTRIBUTE_UNDERLINE_WIDTH:
			case TEXT_ATTRIBUTE_STRIKETHROUGH_WIDTH:
			case TEXT_ATTRIBUTE_OVERLINE_WIDTH: {
				prop["type"] = GlobalConstants::TYPE_INT;
				prop["hint"] = GlobalConstants::PROPERTY_HINT_NONE;
				prop["hint_string"] = "";
			} break;
			case TEXT_ATTRIBUTE_REPLACEMENT_IMAGE: {
				prop["type"] = GlobalConstants::TYPE_OBJECT;
				prop["hint"] = GlobalConstants::PROPERTY_HINT_RESOURCE_TYPE;
				prop["hint_string"] = "Texture2D";
			} break;
			case TEXT_ATTRIBUTE_REPLACEMENT_RECT: {
				prop["type"] = GlobalConstants::TYPE_VECTOR2;
				prop["hint"] = GlobalConstants::PROPERTY_HINT_NONE;
				prop["hint_string"] = "";
			} break;
			case TEXT_ATTRIBUTE_REPLACEMENT_VALIGN: {
				prop["type"] = GlobalConstants::TYPE_INT;
				prop["hint"] = GlobalConstants::PROPERTY_HINT_ENUM;
				prop["hint_string"] = "Top,Center,Bottom";
			} break;
			case TEXT_ATTRIBUTE_OUTLINE_COLOR:
			case TEXT_ATTRIBUTE_UNDERLINE_COLOR:
			case TEXT_ATTRIBUTE_STRIKETHROUGH_COLOR:
			case TEXT_ATTRIBUTE_OVERLINE_COLOR:
			case TEXT_ATTRIBUTE_HIGHLIGHT_COLOR:
			case TEXT_ATTRIBUTE_COLOR: {
				prop["type"] = GlobalConstants::TYPE_COLOR;
				prop["hint"] = GlobalConstants::PROPERTY_HINT_NONE;
				prop["hint_string"] = "";
			} break;
			default: {
				prop["type"] = GlobalConstants::TYPE_NIL;
				prop["hint"] = GlobalConstants::PROPERTY_HINT_NONE;
				prop["hint_string"] = "";
			} break;
		}
		prop["usage"] = GlobalConstants::PROPERTY_USAGE_EDITOR;
		ret.push_back(prop);
	}
	{
		Dictionary prop;
		prop["name"] = "attribute/start";
		prop["type"] = GlobalConstants::TYPE_INT;
		prop["hint"] = GlobalConstants::PROPERTY_HINT_NONE;
		prop["hint_string"] = "";
		prop["usage"] = GlobalConstants::PROPERTY_USAGE_EDITOR;
		ret.push_back(prop);
	}
	{
		Dictionary prop;
		prop["name"] = "attribute/end";
		prop["type"] = GlobalConstants::TYPE_INT;
		prop["hint"] = GlobalConstants::PROPERTY_HINT_NONE;
		prop["hint_string"] = "";
		prop["usage"] = GlobalConstants::PROPERTY_USAGE_EDITOR;
		ret.push_back(prop);
	}
	{
		Dictionary prop;
		prop["name"] = "attribute/_commit";
		prop["type"] = GlobalConstants::TYPE_NIL;
		prop["hint"] = GlobalConstants::PROPERTY_HINT_NONE;
		prop["hint_string"] = "";
		prop["usage"] = GlobalConstants::PROPERTY_USAGE_EDITOR;
		ret.push_back(prop);
	}

	{
		Dictionary prop;
		prop["name"] = "attribute_dict";
		prop["type"] = GlobalConstants::TYPE_ARRAY;
		prop["hint"] = GlobalConstants::PROPERTY_HINT_NONE;
		prop["hint_string"] = "";
		prop["usage"] = GlobalConstants::PROPERTY_USAGE_NOEDITOR | GlobalConstants::PROPERTY_USAGE_STORAGE;
		ret.push_back(prop);
	}
	return ret;
}

void TLShapedAttributedString::_register_methods() {

	//Attribute control
	register_method("commit_attribute", &TLShapedAttributedString::commit_attribute);
	register_method("reject_attribute", &TLShapedAttributedString::reject_attribute);

	register_method("add_attribute", &TLShapedAttributedString::add_attribute);
	register_method("remove_attribute", &TLShapedAttributedString::remove_attribute);
	register_method("has_attribute", &TLShapedAttributedString::has_attribute);
	register_method("get_attribute", &TLShapedAttributedString::get_attribute);
	register_method("get_attribute_start", &TLShapedAttributedString::get_attribute_start);
	register_method("get_attribute_end", &TLShapedAttributedString::get_attribute_end);
	register_method("remove_attributes", &TLShapedAttributedString::remove_attributes);
	register_method("clear_attributes", &TLShapedAttributedString::clear_attributes);

	register_method("get_embedded_rects", &TLShapedAttributedString::get_embedded_rects);

	register_method("load_attributes_dict", &TLShapedAttributedString::load_attributes_dict);
	register_method("save_attributes_dict", &TLShapedAttributedString::save_attributes_dict);

	register_method("_get_property_list", &TLShapedAttributedString::_get_property_list);
	register_method("_get", &TLShapedAttributedString::_get);
	register_method("_set", &TLShapedAttributedString::_set);
}
#endif
