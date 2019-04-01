/*************************************************************************/
/*  tl_shaped_string.cpp                                                 */
/*************************************************************************/

#include "resources/tl_shaped_string.hpp"

#ifdef GODOT_MODULE
#include "core/translation.h"
#else
#include <GlobalConstants.hpp>
#include <TranslationServer.hpp>
#endif

/*************************************************************************/
/*  TLShapedString::Glyph                                                */
/*************************************************************************/

TLShapedString::Glyph::Glyph() {

	codepoint = 0;
}

TLShapedString::Glyph::Glyph(uint32_t p_codepoint, Point2 p_offset, Point2 p_advance) {

	codepoint = p_codepoint;

	offset = p_offset;
	advance = p_advance;
}

/*************************************************************************/
/*  TLShapedString::Cluster                                              */
/*************************************************************************/

TLShapedString::Cluster::Cluster() {

	start = -1;
	end = -1;
	ascent = 0.0f;
	descent = 0.0f;
	width = 0.0f;
	valid = false;
	ignore_on_input = false;
}

/*************************************************************************/
/*  TLShapedString::ScriptIterator                                       */
/*************************************************************************/

bool TLShapedString::ScriptIterator::same_script(int32_t p_script_one, int32_t p_script_two) {

	return p_script_one <= USCRIPT_INHERITED || p_script_two <= USCRIPT_INHERITED || p_script_one == p_script_two;
}

bool TLShapedString::ScriptIterator::next() {

	if (is_rtl) {
		cur--;
	} else {
		cur++;
	}
	return (cur >= 0) && (cur < script_ranges.size());
}

int32_t TLShapedString::ScriptIterator::get_start() const {

	if ((cur >= 0) && (cur < script_ranges.size())) {
		return script_ranges[cur].start;
	} else {
		return -1;
	}
}

int32_t TLShapedString::ScriptIterator::get_end() const {

	if ((cur >= 0) && (cur < script_ranges.size())) {
		return script_ranges[cur].end;
	} else {
		return -1;
	}
}

hb_script_t TLShapedString::ScriptIterator::get_script() const {

	if ((cur >= 0) && (cur < script_ranges.size())) {
		return script_ranges[cur].script;
	} else {
		return HB_SCRIPT_INVALID;
	}
}

void TLShapedString::ScriptIterator::reset(hb_direction_t p_run_direction) {

	if (p_run_direction == HB_DIRECTION_LTR) {
		cur = -1;
		is_rtl = false;
	} else {
		cur = script_ranges.size();
		is_rtl = true;
	}
}

TLShapedString::ScriptIterator::ScriptIterator(const UChar *p_chars, int32_t p_start, int32_t p_length) {

	struct ParenStackEntry {
		int32_t pair_index;
		UScriptCode script_code;
	};

	const UChar *x_chars = p_chars;

	ParenStackEntry paren_stack[128];

	int32_t script_start;
	int32_t script_end = p_start;
	UScriptCode script_code;
	int32_t paren_sp = -1;
	int32_t start_sp = paren_sp;
	UErrorCode err = U_ZERO_ERROR;

	do {
		script_code = USCRIPT_COMMON;
		for (script_start = script_end; script_end < p_length; script_end = next_bound(p_chars, script_end + 1, p_length)) {

			UChar32 ch;
			U16_GET(p_chars, 0, script_end, p_length, ch);

			UScriptCode sc = uscript_getScript(ch, &err);
			if (U_FAILURE(err)) {
				ERR_PRINTS(u_errorName(err));
				ERR_FAIL_COND(true);
			}
			if (u_getIntPropertyValue(ch, UCHAR_BIDI_PAIRED_BRACKET_TYPE) != U_BPT_NONE) {
				if (u_getIntPropertyValue(ch, UCHAR_BIDI_PAIRED_BRACKET_TYPE) == U_BPT_OPEN) {
					paren_stack[++paren_sp].pair_index = ch;
					paren_stack[paren_sp].script_code = script_code;
				} else if (paren_sp >= 0) {
					UChar32 paired_ch = u_getBidiPairedBracket(ch);
					while (paren_sp >= 0 && paren_stack[paren_sp].pair_index != paired_ch)
						paren_sp -= 1;
					if (paren_sp < start_sp) start_sp = paren_sp;
					if (paren_sp >= 0) sc = paren_stack[paren_sp].script_code;
				}
			}

			if (same_script(script_code, sc)) {
				if (script_code <= USCRIPT_INHERITED && sc > USCRIPT_INHERITED) {
					script_code = sc;
					while (start_sp < paren_sp)
						paren_stack[++start_sp].script_code = script_code;
				}
				if ((u_getIntPropertyValue(ch, UCHAR_BIDI_PAIRED_BRACKET_TYPE) == U_BPT_CLOSE) && paren_sp >= 0) {
					paren_sp -= 1;
					start_sp -= 1;
				}
			} else {
				break;
			}
		}

		//printf("SC:%d %d %d %d\n", script_code, script_start, script_end, p_length);

		ScriptRange rng;
		rng.script = hb_icu_script_to_script(script_code);
		rng.start = script_start;
		rng.end = script_end;

		script_ranges.push_back(rng);
	} while (script_end < p_length);
}

/*************************************************************************/
/*  TLShapedString (Base)                                                */
/*************************************************************************/

void TLShapedString::_clear_props() {

	if (bidi_iter) {
		ubidi_close(bidi_iter);
		bidi_iter = NULL;
	}
	if (script_iter) {
		delete script_iter;
		script_iter = NULL;
	}
	_clear_visual();
}

void TLShapedString::_clear_visual() {

	valid = false;
	visual.clear();
	ascent = 0.0f;
	descent = 0.0f;
	width = 0.0f;
}

void TLShapedString::_generate_kashida_justification_opportunies(int64_t p_start, int64_t p_end, /*out*/ std::vector<JustificationOpportunity> &p_ops) const {

	int32_t kashida_pos = -1;
	int8_t priority = 100;
	int64_t i = p_start;

	uint32_t pc = 0;

	while ((p_end > p_start) && is_transparent(get_char(p_end - 1)))
		p_end--;

	while (i < p_end) {
		uint32_t c = get_char(i);

		if (c == 0x0640) {
			kashida_pos = i;
			priority = 0;
		}
		if (priority >= 1 && i < p_end - 1) {
			if (is_seen_sad(c) && (get_char(i + 1) != 0x200C)) {
				kashida_pos = i;
				priority = 1;
			}
		}
		if (priority >= 2 && i > p_start) {
			if (is_teh_marbuta(c) || is_dal(c) || (is_heh(c) && i == p_end - 1)) {
				if (is_connected_to_prev(c, pc)) {
					kashida_pos = i - 1;
					priority = 2;
				}
			}
		}
		if (priority >= 3 && i > p_start) {
			if (is_alef(c) || ((is_lam(c) || is_tah(c) || is_kaf(c) || is_gaf(c)) && i == p_end - 1)) {
				if (is_connected_to_prev(c, pc)) {
					kashida_pos = i - 1;
					priority = 3;
				}
			}
		}
		if (priority >= 4 && i > p_start && i < p_end - 1) {
			if (is_beh(c)) {
				if (is_reh(get_char(i + 1)) || is_yeh(get_char(i + 1))) {
					if (is_connected_to_prev(c, pc)) {
						kashida_pos = i - 1;
						priority = 4;
					}
				}
			}
		}
		if (priority >= 5 && i > p_start) {
			if (is_waw(c) || ((is_ain(c) || is_qaf(c) || is_feh(c)) && i == p_end - 1)) {
				if (is_connected_to_prev(c, pc)) {
					kashida_pos = i - 1;
					priority = 5;
				}
			}
		}
		if (priority >= 6 && i > p_start) {
			if (is_reh(c)) {
				if (is_connected_to_prev(c, pc)) {
					kashida_pos = i - 1;
					priority = 6;
				}
			}
		}
		if (!is_transparent(c)) pc = c;
		i++;
	}
	if (kashida_pos > -1) {
		JustificationOpportunity op;
		op.position = kashida_pos;
		op.kashida = true;
		p_ops.push_back(op);
	}
}

void TLShapedString::_generate_justification_opportunies(int32_t p_start, int32_t p_end, const char *p_lang, /*out*/ std::vector<JustificationOpportunity> &p_ops) const {

	UErrorCode err = U_ZERO_ERROR;
	UBreakIterator *bi = ubrk_open(UBRK_WORD, p_lang, data + p_start, p_end - p_start, &err);
	if (U_FAILURE(err)) {
		//No data - use fallback
		_generate_justification_opportunies_fallback(p_start, p_end, p_ops);
		return;
	}
	int64_t limit = 0;
	while (ubrk_next(bi) != UBRK_DONE) {
		if (ubrk_getRuleStatus(bi) != UBRK_WORD_NONE) {
			_generate_kashida_justification_opportunies(p_start + limit, p_start + ubrk_current(bi), p_ops);

			JustificationOpportunity op;
			op.position = p_start + ubrk_current(bi);
			op.kashida = false;
			p_ops.push_back(op);
			limit = ubrk_current(bi) + 1;
		}
	}
	ubrk_close(bi);
}

void TLShapedString::_generate_justification_opportunies_fallback(int32_t p_start, int32_t p_end, /*out*/ std::vector<JustificationOpportunity> &p_ops) const {

	int64_t limit = p_start;
	for (int64_t i = p_start; i < p_end; i++) {
		if (u_isWhitespace(get_char(i))) {
			_generate_kashida_justification_opportunies(limit, i, p_ops);
			JustificationOpportunity op;
			op.position = i;
			op.kashida = false;
			p_ops.push_back(op);
			limit = i + 1;
		}
	}
	_generate_kashida_justification_opportunies(limit, p_end, p_ops);
}

void TLShapedString::_generate_break_opportunies(int32_t p_start, int32_t p_end, const char *p_lang, /*out*/ std::vector<BreakOpportunity> &p_ops) const {

	UErrorCode err = U_ZERO_ERROR;
	UBreakIterator *bi = ubrk_open(UBRK_LINE, p_lang, data + p_start, p_end - p_start, &err);
	if (U_FAILURE(err)) {
		//No data - use fallback
		_generate_break_opportunies_fallback(p_start, p_end, p_ops);
		return;
	}
	while (ubrk_next(bi) != UBRK_DONE) {
		BreakOpportunity op;
		op.position = p_start + ubrk_current(bi);
		op.hard = (ubrk_getRuleStatus(bi) == UBRK_LINE_HARD);
		p_ops.push_back(op);
		//printf("brk %d [%s]\n", op.position, op.hard ? "H" : "S");
		//if (op.hard) printf("brk %d [%s]\n", op.position, op.hard ? "H" : "S");
	}
	ubrk_close(bi);
}

void TLShapedString::_generate_break_opportunies_fallback(int32_t p_start, int32_t p_end, /*out*/ std::vector<BreakOpportunity> &p_ops) const {

	for (int64_t i = p_start; i < p_end; i++) {
		if (is_break(get_char(i))) {
			BreakOpportunity op;
			op.position = i + 1;
			op.hard = true;
			p_ops.push_back(op);
			//printf("brk %d [Hf]\n", op.position);
		} else if (u_isWhitespace(get_char(i))) {
			BreakOpportunity op;
			op.position = i + 1;
			op.hard = false;
			p_ops.push_back(op);
			//printf("brk %d [Sf]\n", op.position);
		}
	}
	BreakOpportunity op;
	op.position = p_end;
	op.hard = false;
	p_ops.push_back(op);
}

void TLShapedString::_shape_full_string() {

	//Already shaped, nothing to do
	if (valid) {
		return;
	}

	//Nothing to shape
	if (data_size == 0)
		return;

	//Create BiDi iterator
	if (!bidi_iter) {
		UErrorCode err = U_ZERO_ERROR;
		bidi_iter = ubidi_openSized(data_size, 0, &err);
		if (U_FAILURE(err)) {
			ERR_PRINTS(u_errorName(err));
			ERR_FAIL_COND(true);
		}
		switch (base_direction) {
			case TEXT_DIRECTION_LOCALE: {
				ubidi_setPara(bidi_iter, data, data_size, uloc_isRightToLeft(TranslationServer::get_singleton()->get_locale().ascii().get_data()) ? UBIDI_RTL : UBIDI_LTR, NULL, &err);
			} break;
			case TEXT_DIRECTION_LTR: {
				ubidi_setPara(bidi_iter, data, data_size, UBIDI_LTR, NULL, &err);
			} break;
			case TEXT_DIRECTION_RTL: {
				ubidi_setPara(bidi_iter, data, data_size, UBIDI_RTL, NULL, &err);
			} break;
			case TEXT_DIRECTION_AUTO: {
				UBiDiDirection direction = ubidi_getBaseDirection(data, data_size);
				if (direction != UBIDI_NEUTRAL) {
					ubidi_setPara(bidi_iter, data, data_size, direction, NULL, &err);
				} else {
					ubidi_setPara(bidi_iter, data, data_size, uloc_isRightToLeft(TranslationServer::get_singleton()->get_locale().ascii().get_data()) ? UBIDI_RTL : UBIDI_LTR, NULL, &err);
				}
			} break;
		}
		if (U_FAILURE(err)) {
			ERR_PRINTS(u_errorName(err));
			ERR_FAIL_COND(true);
		}
	}

	//Create Script iterator
	if (!script_iter) {
		script_iter = new ScriptIterator(data, 0, data_size);
	}

	//Find BiDi runs in visual order
	UErrorCode err = U_ZERO_ERROR;
	int64_t bidi_run_count = ubidi_countRuns(bidi_iter, &err);
	if (U_FAILURE(err)) {
		ERR_PRINTS(u_errorName(err));
		ERR_FAIL_COND(true);
	}

	for (int64_t i = 0; i < bidi_run_count; i++) {
		int32_t bidi_run_start = 0;
		int32_t bidi_run_length = 0;
		hb_direction_t bidi_run_direction = (ubidi_getVisualRun(bidi_iter, i, &bidi_run_start, &bidi_run_length) == UBIDI_LTR) ? HB_DIRECTION_LTR : HB_DIRECTION_RTL;

		_shape_bidi_run(bidi_run_direction, bidi_run_start, bidi_run_start + bidi_run_length);
	}

	//Calculate ascent, descent and width
	if (visual.size() > 0) {
		float max_neg_offset = 0.0f;
		float max_pos_offset = 0.0f;
		float max_ascent = 0.0f;
		float max_descent = 0.0f;
		float offset = 0.0f;

		for (int64_t i = 0; i < visual.size(); i++) {
			//Calc max ascent / descent
			if (max_ascent < visual[i].ascent) {
				max_ascent = visual[i].ascent;
			}
			if (max_descent < visual[i].descent) {
				max_descent = visual[i].descent;
			}
			width += visual[i].width;
			for (int64_t j = 0; j < visual[i].glyphs.size(); j++) {
				//Calc max offsets for glyphs shifted from baseline and add glyphs width
				if (visual[i].glyphs[j].offset.y > max_pos_offset) {
					max_pos_offset = visual[i].glyphs[j].offset.y;
				}
				if (visual[i].glyphs[j].offset.y < max_neg_offset) {
					max_neg_offset = visual[i].glyphs[j].offset.y;
				}
			}
			visual[i].offset = offset;
			offset += visual[i].width;
		}
		ascent = MAX(max_ascent, -max_neg_offset);
		descent = MAX(max_descent, max_pos_offset);
	} else {
		if (base_font.is_valid()) {
			Ref<TLFontFace> _font = base_font->get_face(base_style);
			if (!_font.is_null()) {
				ascent = _font->get_ascent(base_size);
				descent = _font->get_descent(base_size);
			}
		} else {
			ascent = 15.0f;
			descent = 5.0f;
		}
	}

	//Ready
	valid = true;
	emit_signal("string_shaped");
}

void TLShapedString::_shape_substring(TLShapedString *p_ref, int64_t p_start, int64_t p_end, int p_trim) const {

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
	p_ref->data = (UChar *)std::malloc((p_end - p_start) * sizeof(UChar));
	std::memcpy(p_ref->data, data + p_start, (p_end - p_start) * sizeof(UChar));
	p_ref->data_size = (p_end - p_start);
	p_ref->char_size = u_countChar32(p_ref->data, p_ref->data_size);

	p_ref->base_direction = base_direction;
	p_ref->base_font = base_font;
	p_ref->base_style = base_style;
	p_ref->base_size = base_size;

	p_ref->language = language;
	p_ref->font_features = font_features;

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

void TLShapedString::_shape_bidi_run(hb_direction_t p_run_direction, int32_t p_run_start, int32_t p_run_end) {

#ifdef DEBUG_PRINT_RUNS
	printf(" Shape BiDi Run %d %d %s\n", p_run_start, p_run_end, hb_direction_to_string(p_run_direction));
#endif
	//Find intersecting script runs in visual order
	script_iter->reset(p_run_direction);
	while (script_iter->next()) {
		int32_t script_run_start = script_iter->get_start();
		int32_t script_run_end = script_iter->get_end();
		hb_script_t script_run_script = script_iter->get_script();

		if ((script_run_start < p_run_end) && (script_run_end > p_run_start)) {
			if (base_font.is_valid()) {
				Ref<TLFontFace> _font = base_font->_get_liked_face_for_script(base_style, script_run_script);
				if (_font.is_null()) {
					_font = base_font->get_face(base_style);
					if (_font.is_null()) {
						_shape_hex_run(p_run_direction, p_run_start, p_run_end);
						return;
					}
				}
				_shape_bidi_script_run(p_run_direction, script_run_script, MAX(script_run_start, p_run_start), MIN(script_run_end, p_run_end), _font);
			} else {
				_shape_hex_run(p_run_direction, p_run_start, p_run_end);
			}
		}
	}
}

void TLShapedString::_shape_bidi_script_run(hb_direction_t p_run_direction, hb_script_t p_run_script, int32_t p_run_start, int32_t p_run_end, Ref<TLFontFace> p_font) {

	//Shape monotone run using HarfBuzz
	hb_font_t *hb_font = p_font->get_hb_font(base_size);
	if (!hb_font) {
		if (p_font->get_fallback().is_null()) {
			_shape_hex_run(p_run_direction, p_run_start, p_run_end);
		} else {
			_shape_bidi_script_run(p_run_direction, p_run_script, p_run_start, p_run_end, p_font->get_fallback());
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

	if (language != HB_LANGUAGE_INVALID) hb_buffer_set_language(hb_buffer, language);

	hb_buffer_add_utf16(hb_buffer, (const uint16_t *)data, data_size, p_run_start, p_run_end - p_run_start);
	hb_shape(hb_font, hb_buffer, font_features.empty() ? NULL : &font_features[0], font_features.size());

	//Compose grapheme clusters
	std::vector<Cluster> run_clusters;

	unsigned int glyph_count;
	hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(hb_buffer, &glyph_count);
	hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(hb_buffer, &glyph_count);

	if (glyph_count > 0) {
		uint32_t last_cluster_id = -1;
		for (int64_t i = 0; i < glyph_count; i++) {
			if (glyph_info[i].cluster >= data_size) {
				ERR_PRINTS("HarfBuzz return invalid cluster index");
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

				new_cluster.font_face = p_font.ptr();
				new_cluster.is_rtl = (p_run_direction == HB_DIRECTION_RTL);
				new_cluster.cl_type = _CLUSTER_TYPE_TEXT;

				new_cluster.glyphs.push_back(Glyph(glyph_info[i].codepoint, Point2((glyph_pos[i].x_offset) / 64, -(glyph_pos[i].y_offset / 64)), Point2((glyph_pos[i].x_advance * p_font->get_glyph_scale(base_size)) / 64, ((glyph_pos[i].y_advance * p_font->get_glyph_scale(base_size)) / 64))));
				new_cluster.valid = ((glyph_info[i].codepoint != 0) || !u_isgraph(get_char(glyph_info[i].cluster)));
				new_cluster.start = glyph_info[i].cluster;
				new_cluster.end = glyph_info[i].cluster;
				new_cluster.ascent = p_font->get_ascent(base_size);
				new_cluster.descent = p_font->get_descent(base_size);
				new_cluster.width += (glyph_pos[i].x_advance * p_font->get_glyph_scale(base_size)) / 64;

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
				run_clusters[run_clusters.size() - 1].glyphs.push_back(Glyph(glyph_info[i].codepoint, Point2((glyph_pos[i].x_offset) / 64, -(glyph_pos[i].y_offset / 64)), Point2((glyph_pos[i].x_advance * p_font->get_glyph_scale(base_size)) / 64, ((glyph_pos[i].y_advance * p_font->get_glyph_scale(base_size)) / 64))));
				run_clusters[run_clusters.size() - 1].valid &= ((glyph_info[i].codepoint != 0) || !u_isgraph(get_char(glyph_info[i].cluster)));
				run_clusters[run_clusters.size() - 1].width += (glyph_pos[i].x_advance * p_font->get_glyph_scale(base_size)) / 64;
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
		uint32_t failed_subrun_start = p_run_end + 1;
		uint32_t failed_subrun_end = p_run_start;
		for (int64_t i = 0; i < run_clusters.size(); i++) {
			if (run_clusters[i].valid) {
				if (failed_subrun_start != p_run_end + 1) {
					if (!p_font->get_fallback().is_null()) {
						_shape_bidi_script_run(p_run_direction, p_run_script, failed_subrun_start, failed_subrun_end + 1, p_font->get_fallback());
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
			if (!p_font->get_fallback().is_null() && p_font->get_fallback() != p_font) {
				_shape_bidi_script_run(p_run_direction, p_run_script, failed_subrun_start, failed_subrun_end + 1, p_font->get_fallback());
			} else {
				_shape_hex_run(p_run_direction, failed_subrun_start, failed_subrun_end + 1);
			}
		}
	}
}

void TLShapedString::_shape_hex_run(hb_direction_t p_run_direction, int32_t p_run_start, int32_t p_run_end) {

	//"Shape" monotone run using HexBox fallback
	if (p_run_direction == HB_DIRECTION_LTR) {
		int32_t i = p_run_start;
		while (i < p_run_end) {
			UChar32 c = get_char(i);
			float w = (c <= 0xFF) ? 14 : ((c <= 0xFFFF) ? 20 : 26);
			Cluster hex_cluster;
			hex_cluster.cl_type = _CLUSTER_TYPE_HEX_BOX;
			hex_cluster.is_rtl = (p_run_direction == HB_DIRECTION_RTL);
			hex_cluster.valid = true;
			hex_cluster.start = i;
			hex_cluster.end = i;
			hex_cluster.ascent = 15;
			hex_cluster.descent = 5;
			hex_cluster.width = w;
			hex_cluster.glyphs.push_back(Glyph(c, Point2(0, 0), Point2(w, 0)));
			visual.push_back(hex_cluster);
			if (!U16_IS_SINGLE(data[i])) i++; //SKIP SURROGATE
			i++;
		}
	} else {
		int32_t i = p_run_end - 1;
		while (i >= p_run_start) {
			if (!U16_IS_SINGLE(data[i])) i--; //SKIP SURROGATE
			UChar32 c = get_char(i);
			float w = (c <= 0xFF) ? 14 : ((c <= 0xFFFF) ? 20 : 26);
			Cluster hex_cluster;
			hex_cluster.cl_type = _CLUSTER_TYPE_HEX_BOX;
			hex_cluster.is_rtl = (p_run_direction == HB_DIRECTION_RTL);
			hex_cluster.valid = true;
			hex_cluster.start = i;
			hex_cluster.end = i;
			hex_cluster.ascent = 15;
			hex_cluster.descent = 5;
			hex_cluster.width = w;
			hex_cluster.glyphs.push_back(Glyph(c, Point2(0, 0), Point2(w, 0)));
			visual.push_back(hex_cluster);
			i--;
		}
	}
}

int TLShapedString::get_base_direction() const {

	return base_direction;
}

void TLShapedString::set_base_direction(int p_base_direction) {

	if (base_direction != (TextDirection)p_base_direction) {
		base_direction = (TextDirection)p_base_direction;
		_clear_props();
		emit_signal("string_changed");
	}
}

Ref<TLFontFamily> TLShapedString::get_base_font() const {

	return base_font;
}

void TLShapedString::set_base_font(const Ref<TLFontFamily> p_font) {

	if (!cast_to<TLFontFamily>(*p_font)) {
		ERR_PRINTS("Type mismatch");
		return;
	}
	if (base_font != p_font) {
		base_font = p_font;
		_clear_visual();
		emit_signal("string_changed");
	}
}

String TLShapedString::get_base_font_style() const {

	return base_style;
}

void TLShapedString::set_base_font_style(const String p_style) {

	if (base_style != p_style) {
		base_style = p_style;
		_clear_visual();
		emit_signal("string_changed");
	}
}

int TLShapedString::get_base_font_size() const {

	return base_size;
}

void TLShapedString::set_base_font_size(int p_size) {

	if (base_size != p_size) {
		base_size = p_size;
		_clear_visual();
		emit_signal("string_changed");
	}
}

String TLShapedString::get_features() const {

	String ret;
	char _feature[255];
	for (int64_t i = 0; i < font_features.size(); i++) {
		hb_feature_to_string(const_cast<hb_feature_t *>(&font_features[i]), _feature, 255);
		ret += String(_feature);
		if (i != font_features.size() - 1) ret += String(",");
	}
	return ret;
}

void TLShapedString::set_features(const String p_features) {

#ifdef GODOT_MODULE
	Vector<String> v_features = p_features.split(",");
#else
	PoolStringArray v_features = p_features.split(",");
#endif
	for (int64_t i = 0; i < v_features.size(); i++) {
		hb_feature_t feature;
		if (hb_feature_from_string(v_features[i].ascii().get_data(), -1, &feature)) {
			//feature.start = 0;
			//feature.end = (unsigned int)-1;
			font_features.push_back(feature);
		}
	}
	_clear_visual();
	emit_signal("string_changed");
}

String TLShapedString::get_language() const {

	return String(hb_language_to_string(language));
}

void TLShapedString::set_language(const String p_language) {

	language = hb_language_from_string(p_language.ascii().get_data(), -1);
	_clear_visual();
	emit_signal("string_changed");
}

bool TLShapedString::get_preserve_control() const {

	return preserve_control;
}

void TLShapedString::set_preserve_control(bool p_enable) {

	if (preserve_control != p_enable) {
		preserve_control = p_enable;
		_clear_visual();
		emit_signal("string_changed");
	}
}

bool TLShapedString::shape() {

	if (!valid)
		_shape_full_string();

	return valid;
}

bool TLShapedString::is_valid() const {

	return valid;
}

bool TLShapedString::empty() const {

	return data_size == 0;
}

int64_t TLShapedString::length() const {

	return data_size;
}

int64_t TLShapedString::char_count() const {

	return char_size;
}

float TLShapedString::get_ascent() const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid) {
		if (base_font.is_valid()) {
			Ref<TLFontFace> _font = base_font->get_face(base_style);
			if (_font.is_null()) {
				return 0.0f;
			}
			return _font->get_ascent(base_size);
		} else {
			return 15.0f;
		}
	}

	return ascent;
}

float TLShapedString::get_descent() const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid) {
		if (base_font.is_valid()) {
			Ref<TLFontFace> _font = base_font->get_face(base_style);
			if (_font.is_null()) {
				return 0.0f;
			}
			return _font->get_descent(base_size);
		} else {
			return 5.0f;
		}
	}

	return descent;
}

float TLShapedString::get_width() const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return 0.0f;

	return width;
}

float TLShapedString::get_height() const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid) {
		if (base_font.is_valid()) {
			Ref<TLFontFace> _font = base_font->get_face(base_style);
			if (_font.is_null()) {
				return 0.0f;
			}
			return _font->get_height(base_size);
		} else {
			return 20.0f;
		}
	}

	return ascent + descent;
}

std::vector<int> TLShapedString::break_words() const {

	std::vector<int> ret;

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return ret;

	//Find safe break points
	UErrorCode err = U_ZERO_ERROR;
	UBreakIterator *bi = ubrk_open(UBRK_WORD, hb_language_to_string(language), data, data_size, &err);
	if (U_FAILURE(err)) {
		//No data - use fallback
		for (int64_t i = 0; i < data_size; i++) {
			if (u_isWhitespace(get_char(i))) {
				ret.push_back(i + 1);
			}
		}
		return ret;
	}
	while (ubrk_next(bi) != UBRK_DONE) {
		ret.push_back(ubrk_current(bi));
	}
	ubrk_close(bi);

	return ret;
}

std::vector<int> TLShapedString::break_jst() const {

	std::vector<int> ret;

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return ret;

	std::vector<JustificationOpportunity> jst_ops;
	_generate_justification_opportunies(0, data_size, hb_language_to_string(language), jst_ops);

	for (int i = 0; i < jst_ops.size(); i++) {
		ret.push_back(jst_ops[i].position);
	}

	return ret;
}

std::vector<int> TLShapedString::break_lines(float p_width, TextBreak p_flags) const {

	std::vector<int> ret;

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return ret;

	if (p_flags == TEXT_BREAK_NONE) {
		ret.push_back(data_size);
		return ret;
	}

	//Find safe break points
	std::vector<BreakOpportunity> brk_ops;
	_generate_break_opportunies(0, data_size, hb_language_to_string(language), brk_ops);

	//Sort clusters in logical order
	std::vector<Cluster> logical = visual;
	std::sort(logical.begin(), logical.end(), ClusterCompare);

	//Break lines
	float width = 0.0f;
	int64_t line_start = 0;

	int64_t last_safe_brk = -1;
	int64_t last_safe_brk_cluster = -1;

	int64_t b = 0;
	int64_t i = 0;
	while (i < logical.size()) {
		//printf(" B: %d %d\n", brk_ops[b].position, logical[i].start);
		if ((b < brk_ops.size()) && (brk_ops[b].position <= logical[i].start)) {
			last_safe_brk = b;
			last_safe_brk_cluster = i;
			b++;
			if (brk_ops[last_safe_brk].hard) {
				ret.push_back(logical[i].start);
				//printf("->H: %d %d\n", logical[i].end);

				width = 0.0f;
				line_start = logical[i].end;
				last_safe_brk = -1;
				last_safe_brk_cluster = -1;
				i++;
				continue;
			}
		}
		width += logical[i].width;
		if (p_flags == TEXT_BREAK_MANDATORY_AND_WORD_BOUND) {
			if ((p_width > 0) && (width >= p_width) && (last_safe_brk != -1) && (brk_ops[last_safe_brk].position != line_start)) {
				ret.push_back(brk_ops[last_safe_brk].position);
				//printf("->W: %d %d\n", brk_ops[last_safe_brk].position);

				width = 0.0f;
				i = last_safe_brk_cluster;
				line_start = brk_ops[last_safe_brk].position;
				last_safe_brk = -1;
				last_safe_brk_cluster = -1;
				continue;
			}
		} else if (p_flags == TEXT_BREAK_MANDATORY_AND_ANYWHERE) {
			if ((p_width > 0) && (width >= p_width)) {
				ret.push_back(logical[i].end);
				//printf("->A: %d %d\n", logical[i].end);

				width = 0.0f;
				line_start = logical[i].end;
				last_safe_brk = -1;
				last_safe_brk_cluster = -1;
				i++;
				continue;
			}
		}
		i++;
	}
	//still opts left, check for hard break after end of line
	while (b < brk_ops.size()) {
		if ((brk_ops[b].position == data_size) && (brk_ops[b].hard)) {
			//printf("->X: %d %d\n", data_size);
			ret.push_back(data_size);
			break;
		}
		b++;
	}
	if (line_start < data_size) {
		//Shape clusters after last safe break
		//printf("->F: %d %d\n", data_size);
		ret.push_back(data_size);
	}

	return ret;
}

Ref<TLShapedString> TLShapedString::substr(int64_t p_start, int64_t p_end, int p_trim) const {

	Ref<TLShapedString> ret;
#ifdef GODOT_MODULE
	ret.instance();
#else
	ret = Ref<TLShapedString>::__internal_constructor(TLShapedString::_new());
#endif
	ret->base_direction = base_direction;
	ret->base_font = base_font;
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

void TLShapedString::_shape_single_cluster(int64_t p_start, int64_t p_end, hb_direction_t p_run_direction, hb_script_t p_run_script, UChar32 p_codepoint, Ref<TLFontFace> p_font, /*out*/ Cluster &p_cluster, bool p_font_override) const {

	//Shape single cluster using HarfBuzz
	hb_font_t *hb_font = p_font->get_hb_font(base_size);
	if (!hb_font) {
		if (!p_font->get_fallback().is_null()) {
			_shape_single_cluster(p_start, p_end, p_run_direction, p_run_script, p_codepoint, p_font->get_fallback(), p_cluster);
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

	if (language != HB_LANGUAGE_INVALID) hb_buffer_set_language(hb_buffer, language);

	hb_buffer_add_utf32(hb_buffer, (const uint32_t *)&p_codepoint, 1, 0, 1);
	hb_shape(hb_font, hb_buffer, font_features.empty() ? NULL : &font_features[0], font_features.size());

	unsigned int glyph_count;
	hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(hb_buffer, &glyph_count);
	hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(hb_buffer, &glyph_count);

	p_cluster.glyphs.clear();

	//debug info
	p_cluster.script = p_run_script;
	p_cluster.dir = p_run_direction;
	p_cluster.lang = language;
	//debug info

	p_cluster.font_face = p_font.ptr();
	p_cluster.is_rtl = (p_run_direction == HB_DIRECTION_RTL);
	p_cluster.cl_type = _CLUSTER_TYPE_TEXT;
	p_cluster.valid = true;
	p_cluster.start = p_start;
	p_cluster.end = p_end;
	p_cluster.ascent = p_font->get_ascent(base_size);
	p_cluster.descent = p_font->get_descent(base_size);
	p_cluster.width = 0.0f;

	if (glyph_count > 0) {
		for (int64_t i = 0; i < glyph_count; i++) {
			p_cluster.glyphs.push_back(Glyph(glyph_info[i].codepoint, Point2((glyph_pos[i].x_offset) / 64, -(glyph_pos[i].y_offset / 64)), Point2((glyph_pos[i].x_advance * p_font->get_glyph_scale(base_size)) / 64, ((glyph_pos[i].y_advance * p_font->get_glyph_scale(base_size)) / 64))));
			p_cluster.valid &= ((glyph_info[i].codepoint != 0) || !u_isgraph(p_codepoint));
			p_cluster.width += (glyph_pos[i].x_advance * p_font->get_glyph_scale(base_size)) / 64;
		}
	}
	if (!p_cluster.valid) {
		if (!p_font->get_fallback().is_null()) {
			_shape_single_cluster(p_start, p_end, p_run_direction, p_run_script, p_codepoint, p_font->get_fallback(), p_cluster);
		}
	}
}

float TLShapedString::extend_to_width(float p_width, TextJustification p_flags) {

	if (!valid)
		_shape_full_string();

	if (!valid)
		return width;

	if (p_flags == TEXT_JUSTIFICATION_NONE)
		return width;

	//Nothing to do
	if (width >= p_width)
		return width;

	//Skip edge whitespaces
	int64_t _end = data_size;
	int64_t _start = 0;

	while ((_end > _start) && u_isWhitespace(get_char(_end - 1))) {
		_end--;
	}

	while ((_end > _start) && u_isWhitespace(get_char(_start))) {
		_start++;
	}

	//Find safe justification points
	std::vector<JustificationOpportunity> jst_ops;
	_generate_justification_opportunies(_start, _end, hb_language_to_string(language), jst_ops);
	int64_t ks_count = 0;
	int64_t ws_count = 0;
	for (int64_t i = 0; i < jst_ops.size(); i++) {
		if ((jst_ops[i].position <= 0) || jst_ops[i].position >= data_size - 1)
			continue;
		if (jst_ops[i].kashida) {
			ks_count++;
		}
		if (!jst_ops[i].kashida) {
			ws_count++;
		}
	}

	//Step 1: Kashida justification
	if ((p_flags == TEXT_JUSTIFICATION_KASHIDA_AND_WHITESPACE) || (p_flags == TEXT_JUSTIFICATION_KASHIDA_ONLY)) {
		Cluster ks_cluster;
		float ks_width_per_op = (p_width - width) / ks_count;
		for (int64_t i = 0; i < jst_ops.size(); i++) {
			if ((jst_ops[i].position <= 0) || jst_ops[i].position >= data_size - 1)
				continue;
			if (jst_ops[i].kashida) {
				int64_t j = 1;
				while (j < visual.size()) {
					if (visual[j].start == jst_ops[i].position) {
						_shape_single_cluster(visual[j - 1].end, visual[j].start, HB_DIRECTION_RTL, HB_SCRIPT_ARABIC, 0x0640, visual[j].font_face, ks_cluster);
						ks_cluster.ignore_on_input = true;

						//Add new kashda multiple times
						if (ks_cluster.width > 0) {
							int64_t ks_count_per_op = ks_width_per_op / ks_cluster.width;
							for (int64_t k = 0; k < ks_count_per_op; k++) {
								visual.insert(visual.begin() + j, ks_cluster);
								j++;
							}

							width += ks_count_per_op * ks_cluster.width;
						}
					}
					j++;
				}
			}
		}
	}

	//Step 2: Whitespace justification
	if ((p_flags == TEXT_JUSTIFICATION_KASHIDA_AND_WHITESPACE) || (p_flags == TEXT_JUSTIFICATION_WHITESPACE_ONLY)) {
		float ws_width_per_op = (p_width - width) / ws_count;

		if (base_font.is_null()) {
			return width;
		}
		Ref<TLFontFace> _font = base_font->get_face(base_style);
		if (_font.is_null()) {
			return width;
		}

		Cluster ws_cluster;
		ws_cluster.font_face = _font.ptr();
		ws_cluster.cl_type = _CLUSTER_TYPE_SKIP;
		ws_cluster.valid = true;
		ws_cluster.ascent = _font->get_ascent(base_size);
		ws_cluster.descent = _font->get_descent(base_size);
		ws_cluster.width = ws_width_per_op;
		ws_cluster.glyphs.push_back(Glyph(0, Point2(), Point2(ws_width_per_op, 0)));
		ws_cluster.ignore_on_input = true;

		for (int64_t i = 0; i < jst_ops.size(); i++) {
			if ((jst_ops[i].position <= 0) || jst_ops[i].position >= data_size - 1)
				continue;
			if (!jst_ops[i].kashida) {
				int64_t j = 0;
				while (j < visual.size()) {
					if (visual[j].start == jst_ops[i].position) {
						if ((visual[j].glyphs.size() == 1) && u_isWhitespace(get_char(visual[j].start))) {
							//Extend existing whitespace
							visual[j].glyphs[0].advance.x += ws_width_per_op;
							visual[j].width += ws_width_per_op;
						} else {
							//Add new whitespace
							ws_cluster.is_rtl = visual[j].is_rtl;
							if (visual[j].is_rtl) {
								ws_cluster.start = visual[j].end + 1;
								ws_cluster.end = visual[j].end + 1;

								j++;
								visual.insert(visual.begin() + j, ws_cluster);
								j++;
							} else {
								ws_cluster.start = visual[j].start;
								ws_cluster.end = visual[j].start;

								j++;
								visual.insert(visual.begin() + j, ws_cluster);
								j++;
							}
						}
						width += ws_width_per_op;
					}
					j++;
				}
			}
		}
	}

	float offset = 0.0f;

	for (int64_t i = 0; i < visual.size(); i++) {
		visual[i].offset = offset;
		offset += visual[i].width;
	}

	return width;
}

int64_t TLShapedString::get_cluster_index(int64_t p_position) const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return -1;

	for (int64_t i = 0; i < visual.size(); i++) {
		if (visual[i].start == p_position) {
			return i;
		}
	}
	return -1;
}

float TLShapedString::get_cluster_face_size(int64_t p_index) const {

	return base_size;
}

Ref<TLFontFace> TLShapedString::get_cluster_face(int64_t p_index) const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return Ref<TLFontFace>();

	if ((p_index < 0) || (p_index >= visual.size()))
		return Ref<TLFontFace>();

	return Ref<TLFontFace>(visual[p_index].font_face);
}

int64_t TLShapedString::get_cluster_glyphs(int64_t p_index) const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return 0;

	if ((p_index < 0) || (p_index >= visual.size()))
		return 0;

	return visual[p_index].glyphs.size();
}

uint32_t TLShapedString::get_cluster_glyph(int64_t p_index, int64_t p_glyph) const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return 0;

	if ((p_index < 0) || (p_index >= visual.size()))
		return 0;

	if ((p_glyph < 0) || (p_glyph >= visual[p_index].glyphs.size()))
		return 0;

	return visual[p_index].glyphs[p_glyph].codepoint;
}

Point2 TLShapedString::get_cluster_glyph_offset(int64_t p_index, int64_t p_glyph) const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return Point2();

	if ((p_index < 0) || (p_index >= visual.size()))
		return Point2();

	if ((p_glyph < 0) || (p_glyph >= visual[p_index].glyphs.size()))
		return Point2();

	return visual[p_index].glyphs[p_glyph].offset;
}

Point2 TLShapedString::get_cluster_glyph_advance(int64_t p_index, int64_t p_glyph) const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return Point2();

	if ((p_index < 0) || (p_index >= visual.size()))
		return Point2();

	if ((p_glyph < 0) || (p_glyph >= visual[p_index].glyphs.size()))
		return Point2();

	return visual[p_index].glyphs[p_glyph].advance;
}

Rect2 TLShapedString::get_cluster_rect(int64_t p_index) const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return Rect2();

	if ((p_index < 0) || (p_index >= visual.size()))
		return Rect2();

	return Rect2(Point2(visual[p_index].offset, -visual[p_index].ascent), Size2(visual[p_index].width, visual[p_index].ascent + visual[p_index].descent));
}

String TLShapedString::get_cluster_debug_info(int64_t p_index) const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return String();

	if ((p_index < 0) || (p_index >= visual.size()))
		return String();

	String info;
	info += String("Type: ") + String::num_int64(visual[p_index].cl_type) + String("; ");
	if (visual[p_index].cl_type == _CLUSTER_TYPE_TEXT) {
		info += String("Lang: ") + hb_language_to_string(visual[p_index].lang) + String("; ");
		info += String("Dir: ") + hb_direction_to_string(visual[p_index].dir) + String("; ");
		char tag[5] = "";
		hb_tag_to_string(hb_script_to_iso15924_tag(visual[p_index].script), tag);
		info += String("Script: ") + tag + String("; ");
	}
	return info;
}

float TLShapedString::get_cluster_leading_edge(int64_t p_index) const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return 0.0f;

	if ((p_index < 0) || (p_index >= visual.size()))
		return 0.0f;

	float ret = visual[p_index].offset;
	if (!visual[p_index].is_rtl) {
		ret += visual[p_index].width;
	}
	return ret;
}

float TLShapedString::get_cluster_trailing_edge(int64_t p_index) const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return 0.0f;

	if ((p_index < 0) || (p_index >= visual.size()))
		return 0.0f;

	float ret = visual[p_index].offset;
	if (visual[p_index].is_rtl) {
		ret += visual[p_index].width;
	}
	return ret;
}

int64_t TLShapedString::get_cluster_start(int64_t p_index) const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return -1;

	if ((p_index < 0) || (p_index >= visual.size()))
		return -1;

	return visual[p_index].start;
}

int64_t TLShapedString::get_cluster_end(int64_t p_index) const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return -1;

	if ((p_index < 0) || (p_index >= visual.size()))
		return -1;

	return visual[p_index].end;
}

float TLShapedString::get_cluster_ascent(int64_t p_index) const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return 0.0f;

	if ((p_index < 0) || (p_index >= visual.size()))
		return 0.0f;

	return visual[p_index].ascent;
}

float TLShapedString::get_cluster_descent(int64_t p_index) const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return 0.0f;

	if ((p_index < 0) || (p_index >= visual.size()))
		return 0.0f;

	return visual[p_index].descent;
}

float TLShapedString::get_cluster_width(int64_t p_index) const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return 0.0f;

	if ((p_index < 0) || (p_index >= visual.size()))
		return 0.0f;

	return visual[p_index].width;
}

float TLShapedString::get_cluster_height(int64_t p_index) const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return 0.0f;

	if ((p_index < 0) || (p_index >= visual.size()))
		return 0.0f;

	return visual[p_index].ascent + visual[p_index].descent;
}

std::vector<Rect2> TLShapedString::get_highlight_shapes(int64_t p_start, int64_t p_end) const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return std::vector<Rect2>();

	std::vector<Rect2> ret;

	float prev = 0.0f;
	for (int64_t i = 0; i < visual.size(); i++) {
		float width = 0.0f;

		width += visual[i].width;

		if ((visual[i].start >= p_start) && (visual[i].end < p_end)) {
			ret.push_back(Rect2(prev, 0, width, descent + ascent));
		} else if ((visual[i].start < p_start) && (visual[i].end >= p_end)) {
			float char_width = visual[i].width / (visual[i].end + 1 - visual[i].start);
			int64_t pos_ofs_s = p_start - visual[i].start;
			int64_t pos_ofs_e = p_end - visual[i].start;
			ret.push_back(Rect2(prev + pos_ofs_s * char_width, 0, (pos_ofs_e - pos_ofs_s) * char_width, descent + ascent));
		} else if ((visual[i].start < p_start) && (visual[i].end >= p_start)) {
			float char_width = visual[i].width / (visual[i].end + 1 - visual[i].start);
			int64_t pos_ofs = p_start - visual[i].start;
			if (visual[i].is_rtl) {
				ret.push_back(Rect2(prev, 0, pos_ofs * char_width, descent + ascent));
			} else {
				ret.push_back(Rect2(prev + pos_ofs * char_width, 0, width - pos_ofs * char_width, descent + ascent));
			}
		} else if ((visual[i].start < p_end) && (visual[i].end >= p_end)) {
			float char_width = visual[i].width / (visual[i].end + 1 - visual[i].start);
			int64_t pos_ofs = p_end - visual[i].start;
			if (visual[i].is_rtl) {
				ret.push_back(Rect2(prev + pos_ofs * char_width, 0, width - pos_ofs * char_width, descent + ascent));
			} else {
				ret.push_back(Rect2(prev, 0, pos_ofs * char_width, descent + ascent));
			}
		}
		prev += width;
	}

	//merge intersectiong ranges
	int64_t i = 0;
	while (i < ret.size()) {
		int64_t j = i + 1;
		while (j < ret.size()) {
			if (ret[i].position.x + ret[i].size.x == ret[j].position.x) {
				ret[i].size.x += ret[j].size.x;
				ret.erase(ret.begin() + j);
				continue;
			}
			j++;
		}
		i++;
	}
	return ret;
}

std::vector<float> TLShapedString::get_cursor_positions(int64_t p_position, TextDirection p_primary_dir) const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return std::vector<float>();

	std::vector<float> ret;

	int64_t leading_cluster = -1;
	int64_t trailing_cluster = -1;
	int64_t mid_cluster = -1;

	if ((p_position == 0) && (data_size == 0)) {
		//no data - cusror at start
		ret.push_back(0.0f);
		return ret;
	}
	if (p_position > data_size) {
		//cursor after last char
		ret.push_back(get_cluster_leading_edge(data_size - 1));
		return ret;
	}

	for (int64_t i = 0; i < visual.size(); i++) {
		if (!visual[i].ignore_on_input) {
			if (((visual[i].start >= p_position) && (visual[i].end <= p_position)) || (visual[i].start == p_position)) {
				trailing_cluster = i;
			} else if (((visual[i].start >= p_position - 1) && (visual[i].end <= p_position - 1)) || (visual[i].end == p_position - 1)) {
				leading_cluster = i;
			} else if ((visual[i].start <= p_position) && (visual[i].end >= p_position)) {
				mid_cluster = i;
			}
		}
	}
	if ((leading_cluster != -1) && (trailing_cluster != -1)) {
		if ((p_primary_dir == TEXT_DIRECTION_RTL) && (visual[trailing_cluster].is_rtl)) {
			ret.push_back(get_cluster_leading_edge(leading_cluster));
			ret.push_back(get_cluster_trailing_edge(trailing_cluster));
		} else {
			ret.push_back(get_cluster_trailing_edge(trailing_cluster));
			ret.push_back(get_cluster_leading_edge(leading_cluster));
		}
	} else {
		if (leading_cluster != -1) ret.push_back(get_cluster_leading_edge(leading_cluster));
		if (trailing_cluster != -1) ret.push_back(get_cluster_trailing_edge(trailing_cluster));
	}

	if (mid_cluster != -1) {
		float char_width = visual[mid_cluster].width / (visual[mid_cluster].end + 1 - visual[mid_cluster].start);
		int64_t pos_ofs = p_position - visual[mid_cluster].start;
		if (visual[mid_cluster].is_rtl) {
			ret.push_back(get_cluster_trailing_edge(mid_cluster) - pos_ofs * char_width);
		} else {
			ret.push_back(get_cluster_trailing_edge(mid_cluster) + pos_ofs * char_width);
		}
	}

	return ret;
}

TextDirection TLShapedString::get_char_direction(int64_t p_position) const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return TEXT_DIRECTION_LTR;

	for (int64_t i = 0; i < visual.size(); i++) {
		if ((p_position >= visual[i].start) && (p_position <= visual[i].end)) {
			return visual[i].is_rtl ? TEXT_DIRECTION_RTL : TEXT_DIRECTION_LTR;
		}
	}
	return TEXT_DIRECTION_LTR;
}

int64_t TLShapedString::next_safe_bound(int64_t p_offset) const {

	if (p_offset < 0)
		p_offset = 0;

	if (p_offset >= data_size)
		return data_size;

	return MIN((U16_IS_SURROGATE(data[p_offset]) && U16_IS_SURROGATE_TRAIL(data[p_offset])) ? p_offset + 1 : p_offset, data_size);
}

int64_t TLShapedString::prev_safe_bound(int64_t p_offset) const {

	if (p_offset < 0)
		return 0;

	if (p_offset >= data_size)
		p_offset = data_size - 1;

	return MAX((U16_IS_SURROGATE(data[p_offset]) && U16_IS_SURROGATE_TRAIL(data[p_offset])) ? p_offset - 1 : p_offset, 0);
}

int64_t TLShapedString::pos_wcs_to_u16(int64_t p_position) const {

	int64_t _from = 0;
	if ((sizeof(wchar_t) == 4) && (char_size != data_size)) {
		U16_FWD_N(data, _from, data_size, p_position);
	} else {
		_from = p_position;
	}
	return _from;
}

int64_t TLShapedString::pos_u16_to_wcs(int64_t p_position) const {

	if ((sizeof(wchar_t) == 4) && (char_size != data_size)) {
		int64_t i = 0;
		int64_t c = 0;
		while (i < data_size) {
			if (U16_IS_LEAD(*(data + i))) {
				i++;
			}
			if (i >= p_position)
				break;
			i++;
			c++;
		}
		return c;
	} else {
		return p_position;
	}
}

int64_t TLShapedString::hit_test_cluster(float p_position) const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return -1;

	if (visual.size() == 0) {
		return -1;
	}

	if (p_position < 0) {
		//Hit before first cluster
		return -1;
	}

	float offset = 0.0f;
	for (int64_t i = 0; i < visual.size(); i++) {
		if ((p_position >= offset) && (p_position <= offset + visual[i].width)) {
			//Dircet hit
			return i;
		}
		offset += visual[i].width;
	};

	//Hit after last cluster
	return -1;
}

int64_t TLShapedString::hit_test(float p_position) const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return -1;

	if (visual.size() == 0) {
		return -1;
	}

	if (p_position < 0) {
		//Hit before first cluster
		if (visual[0].is_rtl) {
			return visual[0].end + 1;
		} else {
			return visual[0].start;
		}
	}

	float offset = 0.0f;
	for (int64_t i = 0; i < visual.size(); i++) {
		if ((p_position >= offset) && (p_position <= offset + visual[i].width)) {
			//Dircet hit
			float char_width = visual[i].width / (visual[i].end + 1 - visual[i].start);
			int64_t pos_ofs = round((p_position - offset) / char_width);
			if (visual[i].is_rtl) {
				return visual[i].end + 1 - pos_ofs;
			} else {
				return pos_ofs + visual[i].start;
			}
		}
		offset += visual[i].width;
	};

	//Hit after last cluster
	if (visual[visual.size() - 1].is_rtl) {
		return visual[visual.size() - 1].start;
	} else {
		return visual[visual.size() - 1].end + 1;
	}
}

int64_t TLShapedString::clusters() const {

	if (!valid)
		const_cast<TLShapedString *>(this)->_shape_full_string();

	if (!valid)
		return 0;

	return visual.size();
}

Vector2 TLShapedString::draw_cluster(RID p_canvas_item, const Point2 p_position, int64_t p_index, const Color p_modulate) {

	if (!valid)
		_shape_full_string();

	if (!valid)
		return Vector2();

	if ((p_index < 0) || (p_index >= visual.size()))
		return Vector2();

	Vector2 ofs;
	for (int64_t i = 0; i < visual[p_index].glyphs.size(); i++) {
		if (visual[p_index].cl_type == (int)_CLUSTER_TYPE_HEX_BOX) {
			TLFontFace::draw_hexbox(p_canvas_item, p_position + ofs - Point2(0, visual[p_index].ascent), visual[p_index].glyphs[i].codepoint, p_modulate);
		} else if (visual[p_index].cl_type == (int)_CLUSTER_TYPE_TEXT) {
			visual[p_index].font_face->draw_glyph(p_canvas_item, p_position + ofs + visual[p_index].glyphs[i].offset - Point2(0, visual[p_index].ascent), visual[p_index].glyphs[i].codepoint, p_modulate, base_size);
		} else if (visual[p_index].cl_type == (int)_CLUSTER_TYPE_SKIP) {
			//NOP
		} else {
			ERR_PRINTS("Invalid cluster type");
		}
		ofs += visual[p_index].glyphs[i].advance;
	}

	return ofs;
}

void TLShapedString::draw(RID p_canvas_item, const Point2 p_position, const Color p_modulate) {

	if (!valid)
		_shape_full_string();

	if (!valid)
		return;

#ifdef DEBUG_DISPLAY_METRICS
	VisualServer::get_singleton()->canvas_item_add_line(p_canvas_item, p_position + Point2(0, -ascent), p_position + Point2(width, -ascent), Color(1, 0, 0, 0.5), 1);
	VisualServer::get_singleton()->canvas_item_add_line(p_canvas_item, p_position + Point2(0, 0), p_position + Point2(width, 0), Color(1, 1, 0, 0.5), 1);
	VisualServer::get_singleton()->canvas_item_add_line(p_canvas_item, p_position + Point2(0, descent), p_position + Point2(width, descent), Color(0, 0, 1, 0.5), 1);
#endif

	Vector2 ofs;
	for (int64_t i = 0; i < visual.size(); i++) {
		for (int64_t j = 0; j < visual[i].glyphs.size(); j++) {
			if (visual[i].cl_type == (int)_CLUSTER_TYPE_HEX_BOX) {
				TLFontFace::draw_hexbox(p_canvas_item, p_position + ofs - Point2(0, visual[i].ascent), visual[i].glyphs[j].codepoint, p_modulate);
			} else if (visual[i].cl_type == (int)_CLUSTER_TYPE_TEXT) {
				visual[i].font_face->draw_glyph(p_canvas_item, p_position + ofs + visual[i].glyphs[j].offset - Point2(0, visual[i].ascent), visual[i].glyphs[j].codepoint, p_modulate, base_size);
			} else if (visual[i].cl_type == (int)_CLUSTER_TYPE_SKIP) {
				//NOP
			} else {
				ERR_PRINTS("Invalid cluster type");
			}
			ofs += visual[i].glyphs[j].advance;
		}
	}
}

Array TLShapedString::_break_words() const {

	Array ret;

	std::vector<int> words = break_words();
	for (int64_t i = 0; i < words.size(); i++) {
		ret.push_back(words[i]);
	}

	return ret;
}

Array TLShapedString::_break_jst() const {

	Array ret;

	std::vector<int> jst = break_jst();
	for (int64_t i = 0; i < jst.size(); i++) {
		ret.push_back(jst[i]);
	}

	return ret;
}

Array TLShapedString::_break_lines(float p_width, int64_t p_flags) const {

	Array ret;

	std::vector<int> lines = break_lines(p_width, (TextBreak)p_flags);
	for (int64_t i = 0; i < lines.size(); i++) {
		ret.push_back(lines[i]);
	}

	return ret;
}

Array TLShapedString::_get_highlight_shapes(int64_t p_start, int64_t p_end) const {

	Array ret;
	std::vector<Rect2> rects = get_highlight_shapes(p_start, p_end);
	for (int64_t i = 0; i < rects.size(); i++) {
		ret.push_back(rects[i]);
	}

	return ret;
}

Array TLShapedString::_get_cursor_positions(int64_t p_position, int64_t p_primary_dir) const {

	Array ret;
	std::vector<float> cpos = get_cursor_positions(p_position, (TextDirection)p_primary_dir);
	for (int64_t i = 0; i < cpos.size(); i++) {
		ret.push_back(cpos[i]);
	}

	return ret;
}

float TLShapedString::_extend_to_width(float p_width, int64_t p_flags) {

	return extend_to_width(p_width, (TextJustification)p_flags);
}

TLShapedString::TLShapedString() {

#ifdef GODOT_MODULE
	_init();
#endif
}

void TLShapedString::_init() {

	valid = false;
	preserve_control = false;

	data = NULL;
	bidi_iter = NULL;
	script_iter = NULL;
	hb_buffer = hb_buffer_create();
	language = hb_language_from_string(TranslationServer::get_singleton()->get_locale().ascii().get_data(), -1);

	ascent = 0.0f;
	descent = 0.0f;
	width = 0.0f;
	data_size = 0;
	char_size = 0;
	base_direction = TEXT_DIRECTION_AUTO;
	base_size = 12;
}

TLShapedString::~TLShapedString() {

	if (bidi_iter) {
		ubidi_close(bidi_iter);
		bidi_iter = NULL;
	}

	if (script_iter) {
		delete script_iter;
		script_iter = NULL;
	}

	if (hb_buffer) {
		hb_buffer_destroy(hb_buffer);
		hb_buffer = NULL;
	}

	if (data) {
		std::free(data);
	}
}

//Data in/out

String TLShapedString::get_text() const {

	String ret;
	if (data_size > 0) {
		wchar_t *_data = NULL;

		UErrorCode err = U_ZERO_ERROR;
		int32_t _length = 0;
		u_strToWCS(NULL, 0, &_length, data, data_size, &err);
		if (err != U_BUFFER_OVERFLOW_ERROR) {
			ERR_PRINTS(u_errorName(err));
			return String();
		} else {
			err = U_ZERO_ERROR;
			_data = (wchar_t *)std::malloc((_length + 1) * sizeof(wchar_t));
			if (!_data)
				return String();
			std::memset(_data, 0x00, (_length + 1) * sizeof(wchar_t));
			u_strToWCS(_data, _length, &_length, data, data_size, &err);
			if (U_FAILURE(err)) {
				ERR_PRINTS(u_errorName(err));
				std::free(_data);
				return String();
			}
		}

		ret = String(_data);
		std::free(_data);
	}

	return ret;
}

PoolByteArray TLShapedString::get_utf8() const {

	PoolByteArray ret;

	if (data_size > 0) {
		UErrorCode err = U_ZERO_ERROR;
		int32_t _length = 0;
		int32_t _subs = 0;
		u_strToUTF8WithSub(NULL, 0, &_length, data, data_size, 0xFFFD, &_subs, &err);
		if (err != U_BUFFER_OVERFLOW_ERROR) {
			ERR_PRINTS(u_errorName(err));
			return ret;
		} else {
			err = U_ZERO_ERROR;
			ret.resize(_length);
			u_strToUTF8WithSub((char *)ret.write().ptr(), _length, &_length, data, data_size, 0xFFFD, &_subs, &err);
			if (U_FAILURE(err)) {
				ERR_PRINTS(u_errorName(err));
				ret.resize(0);
				return ret;
			}
		}
	}

	return ret;
}

PoolByteArray TLShapedString::get_utf16() const {

	PoolByteArray ret;

	if (data_size > 0) {
		ret.resize(data_size * sizeof(UChar));
		std::memcpy(ret.write().ptr(), data, data_size * sizeof(UChar));
	}

	return ret;
}

PoolByteArray TLShapedString::get_utf32() const {

	PoolByteArray ret;

	if (data_size > 0) {
		UErrorCode err = U_ZERO_ERROR;
		int32_t _length = 0;
		int32_t _subs = 0;
		u_strToUTF32WithSub(NULL, 0, &_length, data, data_size, 0xFFFD, &_subs, &err);
		if (err != U_BUFFER_OVERFLOW_ERROR) {
			ERR_PRINTS(u_errorName(err));
			return ret;
		} else {
			err = U_ZERO_ERROR;
			ret.resize(_length * sizeof(UChar32));
			u_strToUTF32WithSub((UChar32 *)ret.write().ptr(), _length, &_length, data, data_size, 0xFFFD, &_subs, &err);
			if (U_FAILURE(err)) {
				ERR_PRINTS(u_errorName(err));
				ret.resize(0);
				return ret;
			}
		}
	}

	return ret;
}

void TLShapedString::set_text(const String p_text) {

	_clear_props();

	UErrorCode err = U_ZERO_ERROR;
	int64_t _length = p_text.length();
	int32_t _real_length = 0;

#ifdef GODOT_MODULE
	const wchar_t *_data = p_text.c_str();
#else
	const wchar_t *_data = p_text.unicode_str();
#endif

	//clear
	if (data) std::free(data);
	data = NULL;
	data_size = 0;
	char_size = 0;

	if (_length == 0)
		return;

	u_strFromWCS(NULL, 0, &_real_length, _data, _length, &err);
	if (err != U_BUFFER_OVERFLOW_ERROR) {
		ERR_PRINTS(u_errorName(err));
		return;
	} else {
		err = U_ZERO_ERROR;
		data = (UChar *)std::malloc(_real_length * sizeof(UChar));
		u_strFromWCS(data, _real_length, &_real_length, _data, _length, &err);
		if (U_FAILURE(err)) {
			ERR_PRINTS(u_errorName(err));
			return;
		}
		data_size += _real_length;
		char_size = u_countChar32(data, data_size);
	}
	emit_signal("string_changed");
}

void TLShapedString::set_utf8(const PoolByteArray p_text) {

	_clear_props();

	UErrorCode err = U_ZERO_ERROR;
	int64_t _length = p_text.size();
	int32_t _real_length = 0;
	int32_t _subs = 0;

	//clear
	if (data) std::free(data);
	data = NULL;
	data_size = 0;
	char_size = 0;

	if (_length == 0)
		return;

	u_strFromUTF8WithSub(NULL, 0, &_real_length, (const char *)p_text.read().ptr(), _length, 0xFFFD, &_subs, &err);
	if (err != U_BUFFER_OVERFLOW_ERROR) {
		ERR_PRINTS(u_errorName(err));
		return;
	} else {
		err = U_ZERO_ERROR;
		data = (UChar *)std::malloc(_real_length * sizeof(UChar));
		u_strFromUTF8WithSub(data, _real_length, &_real_length, (const char *)p_text.read().ptr(), _length, 0xFFFD, &_subs, &err);
		if (U_FAILURE(err)) {
			ERR_PRINTS(u_errorName(err));
			return;
		}
		data_size += _real_length;
		char_size = u_countChar32(data, data_size);
	}
	emit_signal("string_changed");
}

void TLShapedString::set_utf16(const PoolByteArray p_text) {

	_clear_props();

	int64_t _length = p_text.size();
	int32_t _real_length = 0;

	//clear
	if (data) std::free(data);
	data = NULL;
	data_size = 0;
	char_size = 0;
	char_size = 0;

	if (_length == 0)
		return;

	_real_length = _length / sizeof(UChar);
	data = (UChar *)std::malloc(_length * sizeof(UChar));
	if (!data)
		return;
	std::memcpy(data, p_text.read().ptr(), _length * sizeof(UChar));
	data_size += _real_length;
	char_size = u_countChar32(data, data_size);
	emit_signal("string_changed");
}

void TLShapedString::set_utf32(const PoolByteArray p_text) {

	_clear_props();

	UErrorCode err = U_ZERO_ERROR;
	int64_t _length = p_text.size() / sizeof(UChar32);
	int32_t _real_length = 0;
	int32_t _subs = 0;

	//clear
	if (data) std::free(data);
	data = NULL;
	data_size = 0;
	char_size = 0;

	if (_length == 0)
		return;

	u_strFromUTF32WithSub(NULL, 0, &_real_length, (const UChar32 *)p_text.read().ptr(), _length, 0xFFFD, &_subs, &err);
	if (err != U_BUFFER_OVERFLOW_ERROR) {
		ERR_PRINTS(u_errorName(err));
		return;
	} else {
		err = U_ZERO_ERROR;
		data = (UChar *)std::malloc(_real_length * sizeof(UChar));
		u_strFromUTF32WithSub(data, _real_length, &_real_length, (const UChar32 *)p_text.read().ptr(), _length, 0xFFFD, &_subs, &err);
		if (U_FAILURE(err)) {
			ERR_PRINTS(u_errorName(err));
			return;
		}
		data_size += _real_length;
		char_size = u_countChar32(data, data_size);
	}
	emit_signal("string_changed");
}

void TLShapedString::add_text(const String p_text) {

	replace_text(data_size, data_size, p_text);
}

void TLShapedString::add_utf8(const PoolByteArray p_text) {

	replace_utf8(data_size, data_size, p_text);
}

void TLShapedString::add_utf16(const PoolByteArray p_text) {

	replace_utf16(data_size, data_size, p_text);
}

void TLShapedString::add_utf32(const PoolByteArray p_text) {

	replace_utf32(data_size, data_size, p_text);
}

void TLShapedString::add_sstring(Ref<TLShapedString> p_text) {

	replace_sstring(data_size, data_size, p_text);
}

void TLShapedString::replace_text(int64_t p_start, int64_t p_end, const String p_text) {

	if ((p_start > p_end) || (p_end > data_size)) {
		ERR_PRINTS("Invalid range");
		return;
	}

	_clear_props();

	UErrorCode err = U_ZERO_ERROR;
	int64_t _length = p_text.length();
	int32_t _real_length = 0;

#ifdef GODOT_MODULE
	const wchar_t *_data = p_text.c_str();
#else
	const wchar_t *_data = p_text.unicode_str();
#endif

	if (_length != 0) {
		u_strFromWCS(NULL, 0, &_real_length, _data, _length, &err);
		if (err != U_BUFFER_OVERFLOW_ERROR) {
			ERR_PRINTS(u_errorName(err));
			return;
		} else {
			err = U_ZERO_ERROR;
		}
	}

	UChar *new_data = (UChar *)std::malloc((data_size - (p_end - p_start) + _real_length) * sizeof(UChar));
	if (!new_data)
		return;
	if (data) {
		std::memcpy(new_data, data, p_start * sizeof(UChar));
		std::memcpy(new_data + p_start + _real_length, data + p_end, (data_size - p_end) * sizeof(UChar));
		std::free(data);
	}
	data = new_data;

	u_strFromWCS(data + p_start, _real_length, &_real_length, _data, _length, &err);
	if (U_FAILURE(err)) {
		ERR_PRINTS(u_errorName(err));
		return;
	}
	data_size = data_size - (p_end - p_start) + _real_length;
	char_size = u_countChar32(data, data_size);
	emit_signal("string_changed");
}

void TLShapedString::replace_utf8(int64_t p_start, int64_t p_end, const PoolByteArray p_text) {

	if ((p_start > p_end) || (p_end > data_size)) {
		ERR_PRINTS("Invalid range");
		return;
	}

	_clear_props();

	UErrorCode err = U_ZERO_ERROR;
	int64_t _length = p_text.size();
	int32_t _real_length = 0;
	int32_t _subs = 0;

	if (_length != 0) {
		u_strFromUTF8WithSub(NULL, 0, &_real_length, (const char *)p_text.read().ptr(), _length, 0xFFFD, &_subs, &err);
		if (err != U_BUFFER_OVERFLOW_ERROR) {
			ERR_PRINTS(u_errorName(err));
			return;
		} else {
			err = U_ZERO_ERROR;
		}
	}

	UChar *new_data = (UChar *)std::malloc((data_size - (p_end - p_start) + _real_length) * sizeof(UChar));
	if (!new_data)
		return;
	if (data) {
		std::memcpy(new_data, data, p_start * sizeof(UChar));
		std::memcpy(new_data + p_start + _real_length, data + p_end, (data_size - p_end) * sizeof(UChar));
		std::free(data);
	}
	data = new_data;

	u_strFromUTF8WithSub(data + p_start, _real_length, &_real_length, (const char *)p_text.read().ptr(), _length, 0xFFFD, &_subs, &err);
	if (U_FAILURE(err)) {
		ERR_PRINTS(u_errorName(err));
		return;
	}
	data_size = data_size - (p_end - p_start) + _real_length;
	char_size = u_countChar32(data, data_size);
	emit_signal("string_changed");
}

void TLShapedString::replace_utf16(int64_t p_start, int64_t p_end, const PoolByteArray p_text) {

	if ((p_start > p_end) || (p_end > data_size)) {
		ERR_PRINTS("Invalid range");
		return;
	}

	_clear_props();

	int64_t _length = p_text.size();
	int32_t _real_length = 0;

	if (_length != 0) {
		_real_length = _length / sizeof(UChar);
	}

	UChar *new_data = (UChar *)std::malloc((data_size - (p_end - p_start) + _real_length) * sizeof(UChar));
	if (!new_data)
		return;
	if (data) {
		std::memcpy(new_data, data, p_start * sizeof(UChar));
		std::memcpy(new_data + p_start + _real_length, data + p_end, (data_size - p_end) * sizeof(UChar));
		std::free(data);
	}
	data = new_data;

	std::memcpy(data + p_start, p_text.read().ptr(), _length * sizeof(UChar));

	data_size = data_size - (p_end - p_start) + _real_length;
	char_size = u_countChar32(data, data_size);
	emit_signal("string_changed");
}

void TLShapedString::replace_utf32(int64_t p_start, int64_t p_end, const PoolByteArray p_text) {

	if ((p_start > p_end) || (p_end > data_size)) {
		ERR_PRINTS("Invalid range");
		return;
	}

	_clear_props();

	UErrorCode err = U_ZERO_ERROR;
	int64_t _length = p_text.size() / sizeof(UChar32);
	int32_t _real_length = 0;
	int32_t _subs = 0;

	if (_length != 0) {
		u_strFromUTF32WithSub(NULL, 0, &_real_length, (const UChar32 *)p_text.read().ptr(), _length, 0xFFFD, &_subs, &err);
		if (err != U_BUFFER_OVERFLOW_ERROR) {
			ERR_PRINTS(u_errorName(err));
			return;
		} else {
			err = U_ZERO_ERROR;
		}
	}

	UChar *new_data = (UChar *)std::malloc((data_size - (p_end - p_start) + _real_length) * sizeof(UChar));
	if (!new_data)
		return;
	if (data) {
		std::memcpy(new_data, data, p_start * sizeof(UChar));
		std::memcpy(new_data + p_start + _real_length, data + p_end, (data_size - p_end) * sizeof(UChar));
		std::free(data);
	}
	data = new_data;

	u_strFromUTF32WithSub(data + p_start, _real_length, &_real_length, (const UChar32 *)p_text.read().ptr(), _length, 0xFFFD, &_subs, &err);
	if (U_FAILURE(err)) {
		ERR_PRINTS(u_errorName(err));
		return;
	}
	data_size = data_size - (p_end - p_start) + _real_length;
	char_size = u_countChar32(data, data_size);
	emit_signal("string_changed");
}

void TLShapedString::replace_sstring(int64_t p_start, int64_t p_end, Ref<TLShapedString> p_text) {

	if ((p_start > p_end) || (p_end > data_size)) {
		ERR_PRINTS("Invalid range");
		return;
	}

	_clear_props();

	int64_t _length = p_text->length();
	int32_t _real_length = 0;

	if (_length != 0) {
		_real_length = _length / sizeof(UChar);
	}

	UChar *new_data = (UChar *)std::malloc((data_size - (p_end - p_start) + _real_length) * sizeof(UChar));
	if (!new_data)
		return;
	if (data) {
		std::memcpy(new_data, data, p_start * sizeof(UChar));
		std::memcpy(new_data + p_start + _real_length, data + p_end, (data_size - p_end) * sizeof(UChar));
		std::free(data);
	}
	data = new_data;

	std::memcpy(data + p_start, p_text->data, _length * sizeof(UChar));

	data_size = data_size - (p_end - p_start) + _real_length;
	char_size = u_countChar32(data, data_size);
	emit_signal("string_changed");
}

void TLShapedString::copy_properties(Ref<TLShapedString> p_source) {

	base_direction = p_source->base_direction;
	language = p_source->language;
	font_features = p_source->font_features;
	base_font = p_source->base_font;
	base_size = p_source->base_size;
	base_style = p_source->base_style;

	_clear_props();
	emit_signal("string_changed");
}

#ifdef GODOT_MODULE

void TLShapedString::_bind_methods() {

	ClassDB::bind_method(D_METHOD("copy_properties", "source"), &TLShapedString::copy_properties);

	ClassDB::bind_method(D_METHOD("set_base_direction", "direction"), &TLShapedString::set_base_direction);
	ClassDB::bind_method(D_METHOD("get_base_direction"), &TLShapedString::get_base_direction);

	ClassDB::bind_method(D_METHOD("set_text", "text"), &TLShapedString::set_text);
	ClassDB::bind_method(D_METHOD("get_text"), &TLShapedString::get_text);
	ClassDB::bind_method(D_METHOD("add_text", "text"), &TLShapedString::add_text);
	ClassDB::bind_method(D_METHOD("replace_text", "start", "end", "text"), &TLShapedString::replace_text);

	ClassDB::bind_method(D_METHOD("add_sstring", "text"), &TLShapedString::add_sstring);
	ClassDB::bind_method(D_METHOD("replace_sstring", "start", "end", "text"), &TLShapedString::replace_sstring);

	ClassDB::bind_method(D_METHOD("get_utf8"), &TLShapedString::get_utf8);
	ClassDB::bind_method(D_METHOD("set_utf8", "data"), &TLShapedString::set_utf8);
	ClassDB::bind_method(D_METHOD("add_utf8", "text"), &TLShapedString::add_utf8);
	ClassDB::bind_method(D_METHOD("replace_utf8", "start", "end", "text"), &TLShapedString::replace_utf8);

	ClassDB::bind_method(D_METHOD("get_utf16"), &TLShapedString::get_utf16);
	ClassDB::bind_method(D_METHOD("set_utf16", "data"), &TLShapedString::set_utf16);
	ClassDB::bind_method(D_METHOD("add_utf16", "text"), &TLShapedString::add_utf16);
	ClassDB::bind_method(D_METHOD("replace_utf16", "start", "end", "text"), &TLShapedString::replace_utf16);

	ClassDB::bind_method(D_METHOD("get_utf32"), &TLShapedString::get_utf32);
	ClassDB::bind_method(D_METHOD("set_utf32", "data"), &TLShapedString::set_utf32);
	ClassDB::bind_method(D_METHOD("add_utf32", "text"), &TLShapedString::add_utf32);
	ClassDB::bind_method(D_METHOD("replace_utf32", "start", "end", "text"), &TLShapedString::replace_utf32);

	ClassDB::bind_method(D_METHOD("set_base_font", "ref"), &TLShapedString::set_base_font);
	ClassDB::bind_method(D_METHOD("get_base_font"), &TLShapedString::get_base_font);

	ClassDB::bind_method(D_METHOD("set_base_font_style", "style"), &TLShapedString::set_base_font_style);
	ClassDB::bind_method(D_METHOD("get_base_font_style"), &TLShapedString::get_base_font_style);

	ClassDB::bind_method(D_METHOD("set_base_font_size", "size"), &TLShapedString::set_base_font_size);
	ClassDB::bind_method(D_METHOD("get_base_font_size"), &TLShapedString::get_base_font_size);

	ClassDB::bind_method(D_METHOD("set_features", "features"), &TLShapedString::set_features);
	ClassDB::bind_method(D_METHOD("get_features"), &TLShapedString::get_features);

	ClassDB::bind_method(D_METHOD("set_language", "language"), &TLShapedString::set_language);
	ClassDB::bind_method(D_METHOD("get_language"), &TLShapedString::get_language);

	ClassDB::bind_method(D_METHOD("set_preserve_control", "enable"), &TLShapedString::set_preserve_control);
	ClassDB::bind_method(D_METHOD("get_preserve_control"), &TLShapedString::get_preserve_control);

	//Line data
	ClassDB::bind_method(D_METHOD("shape"), &TLShapedString::shape);
	ClassDB::bind_method(D_METHOD("is_valid"), &TLShapedString::is_valid);
	ClassDB::bind_method(D_METHOD("empty"), &TLShapedString::empty);
	ClassDB::bind_method(D_METHOD("length"), &TLShapedString::length);
	ClassDB::bind_method(D_METHOD("char_count"), &TLShapedString::char_count);

	ClassDB::bind_method(D_METHOD("get_ascent"), &TLShapedString::get_ascent);
	ClassDB::bind_method(D_METHOD("get_descent"), &TLShapedString::get_descent);
	ClassDB::bind_method(D_METHOD("get_width"), &TLShapedString::get_width);
	ClassDB::bind_method(D_METHOD("get_height"), &TLShapedString::get_height);

	//Line modification
	ClassDB::bind_method(D_METHOD("break_words"), &TLShapedString::_break_words);
	ClassDB::bind_method(D_METHOD("break_jst"), &TLShapedString::_break_jst);
	ClassDB::bind_method(D_METHOD("break_lines", "width", "flags"), &TLShapedString::_break_lines);
	ClassDB::bind_method(D_METHOD("substr", "start", "end", "trim"), &TLShapedString::substr);
	ClassDB::bind_method(D_METHOD("extend_to_width", "width", "flags"), &TLShapedString::_extend_to_width);

	//Cluster data
	ClassDB::bind_method(D_METHOD("clusters"), &TLShapedString::clusters);
	ClassDB::bind_method(D_METHOD("get_cluster_index", "position"), &TLShapedString::get_cluster_index);
	ClassDB::bind_method(D_METHOD("get_cluster_face", "position"), &TLShapedString::get_cluster_face);
	ClassDB::bind_method(D_METHOD("get_cluster_face_size", "position"), &TLShapedString::get_cluster_face_size);
	ClassDB::bind_method(D_METHOD("get_cluster_trailing_edge", "index"), &TLShapedString::get_cluster_trailing_edge);
	ClassDB::bind_method(D_METHOD("get_cluster_leading_edge", "index"), &TLShapedString::get_cluster_leading_edge);
	ClassDB::bind_method(D_METHOD("get_cluster_start", "index"), &TLShapedString::get_cluster_start);
	ClassDB::bind_method(D_METHOD("get_cluster_end", "index"), &TLShapedString::get_cluster_end);
	ClassDB::bind_method(D_METHOD("get_cluster_ascent", "index"), &TLShapedString::get_cluster_ascent);
	ClassDB::bind_method(D_METHOD("get_cluster_descent", "index"), &TLShapedString::get_cluster_descent);
	ClassDB::bind_method(D_METHOD("get_cluster_width", "index"), &TLShapedString::get_cluster_width);
	ClassDB::bind_method(D_METHOD("get_cluster_height", "index"), &TLShapedString::get_cluster_height);
	ClassDB::bind_method(D_METHOD("get_cluster_rect", "index"), &TLShapedString::get_cluster_rect);
	ClassDB::bind_method(D_METHOD("get_cluster_debug_info", "index"), &TLShapedString::get_cluster_debug_info);

	//Glyph data
	ClassDB::bind_method(D_METHOD("get_cluster_glyphs", "index"), &TLShapedString::get_cluster_glyphs);
	ClassDB::bind_method(D_METHOD("get_cluster_glyph", "index", "glyph"), &TLShapedString::get_cluster_glyph);
	ClassDB::bind_method(D_METHOD("get_cluster_glyph_offset", "index", "glyph"), &TLShapedString::get_cluster_glyph_offset);
	ClassDB::bind_method(D_METHOD("get_cluster_glyph_advance", "index", "glyph"), &TLShapedString::get_cluster_glyph_advance);

	//Output
	ClassDB::bind_method(D_METHOD("get_highlight_shapes", "start", "end"), &TLShapedString::_get_highlight_shapes);
	ClassDB::bind_method(D_METHOD("get_cursor_positions", "position", "primary_dir"), &TLShapedString::_get_cursor_positions);
	ClassDB::bind_method(D_METHOD("get_char_direction", "position"), &TLShapedString::get_char_direction);
	ClassDB::bind_method(D_METHOD("hit_test", "position"), &TLShapedString::hit_test);
	ClassDB::bind_method(D_METHOD("hit_test_cluster", "position"), &TLShapedString::hit_test_cluster);

	ClassDB::bind_method(D_METHOD("draw_cluster", "canvas_item", "position", "index", "modulate"), &TLShapedString::draw_cluster);
	ClassDB::bind_method(D_METHOD("draw", "canvas_item", "position", "modulate"), &TLShapedString::draw);

	//Helpers
	ClassDB::bind_method(D_METHOD("pos_u16_to_wcs", "position"), &TLShapedString::pos_u16_to_wcs);
	ClassDB::bind_method(D_METHOD("pos_wcs_to_u16", "position"), &TLShapedString::pos_wcs_to_u16);

	ClassDB::bind_method(D_METHOD("next_safe_bound", "position"), &TLShapedString::next_safe_bound);
	ClassDB::bind_method(D_METHOD("prev_safe_bound", "position"), &TLShapedString::prev_safe_bound);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "base_direction", PROPERTY_HINT_ENUM, "LTR,RTL,Locale,Auto"), "set_base_direction", "get_base_direction");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "text", PROPERTY_HINT_MULTILINE_TEXT), "set_text", "get_text");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "base_font", PROPERTY_HINT_RESOURCE_TYPE, "TLFontFamily"), "set_base_font", "get_base_font");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "base_font_style"), "set_base_font_style", "get_base_font_style");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "base_font_size"), "set_base_font_size", "get_base_font_size");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "features"), "set_features", "get_features");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "language"), "set_language", "get_language");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "preserve_control"), "set_preserve_control", "get_preserve_control");

	ADD_SIGNAL(MethodInfo("string_changed"));
	ADD_SIGNAL(MethodInfo("string_shaped"));

	BIND_ENUM_CONSTANT(TEXT_DIRECTION_LTR);
	BIND_ENUM_CONSTANT(TEXT_DIRECTION_RTL);
	BIND_ENUM_CONSTANT(TEXT_DIRECTION_LOCALE);
	BIND_ENUM_CONSTANT(TEXT_DIRECTION_AUTO);

	BIND_ENUM_CONSTANT(TEXT_JUSTIFICATION_NONE);
	BIND_ENUM_CONSTANT(TEXT_JUSTIFICATION_KASHIDA_AND_WHITESPACE);
	BIND_ENUM_CONSTANT(TEXT_JUSTIFICATION_KASHIDA_ONLY);
	BIND_ENUM_CONSTANT(TEXT_JUSTIFICATION_WHITESPACE_ONLY);

	BIND_ENUM_CONSTANT(TEXT_BREAK_NONE);
	BIND_ENUM_CONSTANT(TEXT_BREAK_MANDATORY);
	BIND_ENUM_CONSTANT(TEXT_BREAK_MANDATORY_AND_WORD_BOUND);
	BIND_ENUM_CONSTANT(TEXT_BREAK_MANDATORY_AND_ANYWHERE);

	BIND_ENUM_CONSTANT(TEXT_TRIM_NONE);
	BIND_ENUM_CONSTANT(TEXT_TRIM_BREAK);
	BIND_ENUM_CONSTANT(TEXT_TRIM_BREAK_AND_WHITESPACE);
}

#else

void TLShapedString::_register_methods() {

	register_method("copy_properties", &TLShapedString::copy_properties);

	register_method("set_base_direction", &TLShapedString::set_base_direction);
	register_method("get_base_direction", &TLShapedString::get_base_direction);

	register_method("set_text", &TLShapedString::set_text);
	register_method("get_text", &TLShapedString::get_text);
	register_method("add_text", &TLShapedString::add_text);
	register_method("replace_text", &TLShapedString::replace_text);

	register_method("add_sstring", &TLShapedString::add_sstring);
	register_method("replace_sstring", &TLShapedString::replace_sstring);

	register_method("get_utf8", &TLShapedString::get_utf8);
	register_method("set_utf8", &TLShapedString::set_utf8);
	register_method("add_utf8", &TLShapedString::add_utf8);
	register_method("replace_utf8", &TLShapedString::replace_utf8);

	register_method("get_utf16", &TLShapedString::get_utf16);
	register_method("set_utf16", &TLShapedString::set_utf16);
	register_method("add_utf16", &TLShapedString::add_utf16);
	register_method("replace_utf16", &TLShapedString::replace_utf16);

	register_method("get_utf32", &TLShapedString::get_utf32);
	register_method("set_utf32", &TLShapedString::set_utf32);
	register_method("add_utf32", &TLShapedString::add_utf32);
	register_method("replace_utf32", &TLShapedString::replace_utf32);

	register_method("set_base_font", &TLShapedString::set_base_font);
	register_method("get_base_font", &TLShapedString::get_base_font);

	register_method("set_base_font_style", &TLShapedString::set_base_font_style);
	register_method("get_base_font_style", &TLShapedString::get_base_font_style);

	register_method("set_base_font_size", &TLShapedString::set_base_font_size);
	register_method("get_base_font_size", &TLShapedString::get_base_font_size);

	register_method("set_features", &TLShapedString::set_features);
	register_method("get_features", &TLShapedString::get_features);

	register_method("set_language", &TLShapedString::set_language);
	register_method("get_language", &TLShapedString::get_language);

	register_method("set_preserve_control", &TLShapedString::set_preserve_control);
	register_method("get_preserve_control", &TLShapedString::get_preserve_control);

	//Line data
	register_method("shape", &TLShapedString::shape);
	register_method("is_valid", &TLShapedString::is_valid);
	register_method("empty", &TLShapedString::empty);
	register_method("length", &TLShapedString::length);
	register_method("char_count", &TLShapedString::char_count);

	register_method("get_ascent", &TLShapedString::get_ascent);
	register_method("get_descent", &TLShapedString::get_descent);
	register_method("get_width", &TLShapedString::get_width);
	register_method("get_height", &TLShapedString::get_height);

	//Line modification
	register_method("break_words", &TLShapedString::_break_words);
	register_method("break_jst", &TLShapedString::_break_jst);
	register_method("break_lines", &TLShapedString::_break_lines);

	register_method("substr", &TLShapedString::substr);
	register_method("extend_to_width", &TLShapedString::_extend_to_width);

	//Cluster data
	register_method("clusters", &TLShapedString::clusters);
	register_method("get_cluster_index", &TLShapedString::get_cluster_index);
	register_method("get_cluster_face", &TLShapedString::get_cluster_face);
	register_method("get_cluster_face_size", &TLShapedString::get_cluster_face_size);
	register_method("get_cluster_trailing_edge", &TLShapedString::get_cluster_trailing_edge);
	register_method("get_cluster_leading_edge", &TLShapedString::get_cluster_leading_edge);
	register_method("get_cluster_start", &TLShapedString::get_cluster_start);
	register_method("get_cluster_end", &TLShapedString::get_cluster_end);
	register_method("get_cluster_ascent", &TLShapedString::get_cluster_ascent);
	register_method("get_cluster_descent", &TLShapedString::get_cluster_descent);
	register_method("get_cluster_width", &TLShapedString::get_cluster_width);
	register_method("get_cluster_height", &TLShapedString::get_cluster_height);
	register_method("get_cluster_rect", &TLShapedString::get_cluster_rect);
	register_method("get_cluster_debug_info", &TLShapedString::get_cluster_debug_info);

	//Glyph data
	register_method("get_cluster_glyphs", &TLShapedString::get_cluster_glyphs);
	register_method("get_cluster_glyph", &TLShapedString::get_cluster_glyph);
	register_method("get_cluster_glyph_offset", &TLShapedString::get_cluster_glyph_offset);
	register_method("get_cluster_glyph_advance", &TLShapedString::get_cluster_glyph_advance);

	//Output
	register_method("get_highlight_shapes", &TLShapedString::_get_highlight_shapes);
	register_method("get_cursor_positions", &TLShapedString::_get_cursor_positions);
	register_method("get_char_direction", &TLShapedString::get_char_direction);
	register_method("hit_test", &TLShapedString::hit_test);
	register_method("hit_test_cluster", &TLShapedString::hit_test_cluster);

	register_method("draw_cluster", &TLShapedString::draw_cluster);
	register_method("draw", &TLShapedString::draw);

	//Helpers
	register_method("pos_u16_to_wcs", &TLShapedString::pos_u16_to_wcs);
	register_method("pos_wcs_to_u16", &TLShapedString::pos_wcs_to_u16);

	register_method("next_safe_bound", &TLShapedString::next_safe_bound);
	register_method("prev_safe_bound", &TLShapedString::prev_safe_bound);

	register_property<TLShapedString, int>("base_direction", &TLShapedString::set_base_direction, &TLShapedString::get_base_direction, TEXT_DIRECTION_AUTO, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM, String("LTR,RTL,Locale,Auto"));
	register_property<TLShapedString, String>("text", &TLShapedString::set_text, &TLShapedString::get_text, String());
	register_property<TLShapedString, Ref<TLFontFamily> >("base_font", &TLShapedString::set_base_font, &TLShapedString::get_base_font, Ref<TLFontFamily>(), GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RESOURCE_TYPE, String("TLFontFamily"));
	register_property<TLShapedString, String>("base_font_style", &TLShapedString::set_base_font_style, &TLShapedString::get_base_font_style, String());
	register_property<TLShapedString, int>("base_font_ssize", &TLShapedString::set_base_font_size, &TLShapedString::get_base_font_size, 12);
	register_property<TLShapedString, String>("features", &TLShapedString::set_features, &TLShapedString::get_features, String());
	register_property<TLShapedString, String>("language", &TLShapedString::set_language, &TLShapedString::get_language, String());
	register_property<TLShapedString, bool>("preserve_control", &TLShapedString::set_preserve_control, &TLShapedString::get_preserve_control, false);

	register_signal<TLShapedString>("string_changed");
	register_signal<TLShapedString>("string_shaped");
}

#endif
