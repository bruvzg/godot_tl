/*************************************************************************/
/*  tl_font_family.cpp                                                   */
/*************************************************************************/

#include "resources/tl_font_family.hpp"
Ref<TLFontFace> TLFontFallbackIterator::value() {
	if (index >= 0) {
		switch (chain_id) {
			case SINGLE_FONT: {
				return font;
			} break;
			case MAIN_CHAIN: {
				if (index < (int64_t)data->main_chain.size()) {
					return data->main_chain[index];
				}
			} break;
			case SCRIPT_CHAIN: {
				if (index < (int64_t)data->linked_src_chain.at(script).size()) {
					return data->linked_src_chain.at(script)[index];
				}
			} break;
			case LANG_CHAIN: {
				if (index < (int64_t)data->linked_lang_chain.at(lang).size()) {
					return data->linked_lang_chain.at(lang)[index];
				}
			} break;
			default: {
				return NULL;
			} break;
		}
	}
	return NULL;
}

bool TLFontFallbackIterator::is_valid() {
	return (index >= 0) && (chain_id != INVALID_CHAIN);
}

bool TLFontFallbackIterator::is_linked() {
	return (index >= 0) && ((chain_id == SCRIPT_CHAIN) || (chain_id == LANG_CHAIN));
}

TLFontFallbackIterator TLFontFallbackIterator::next() {
	switch (chain_id) {
		case SINGLE_FONT: {
			return TLFontFallbackIterator();
		} break;
		case MAIN_CHAIN: {
			if (index < (int64_t)data->main_chain.size() - 1) {
				TLFontFallbackIterator next;
				next.data = data;
				next.lang = lang;
				next.script = script;
				next.chain_id = chain_id;
				next.index = index + 1;
				return next;
			} else {
				return TLFontFallbackIterator();
			}
		}
		case SCRIPT_CHAIN: {
			if ((data->linked_src_chain.count(script) != 0) && (index < (int64_t)data->linked_src_chain.at(script).size() - 1)) {
				TLFontFallbackIterator next;
				next.data = data;
				next.lang = lang;
				next.script = script;
				next.chain_id = chain_id;
				next.index = index + 1;
				return next;
			} else {
				if (data->main_chain.size() > 0) {
					TLFontFallbackIterator next;
					next.data = data;
					next.lang = lang;
					next.script = script;
					next.chain_id = MAIN_CHAIN;
					next.index = 0;
					return next;
				} else {
					return TLFontFallbackIterator();
				}
			}
		}
		case LANG_CHAIN: {
			if ((data->linked_lang_chain.count(lang) != 0) && (index < (int64_t)data->linked_lang_chain.at(lang).size() - 1)) {
				TLFontFallbackIterator next;
				next.data = data;
				next.lang = lang;
				next.script = script;
				next.chain_id = chain_id;
				next.index = index + 1;
				return next;
			} else {
				if (data->main_chain.size() > 0) {
					TLFontFallbackIterator next;
					next.data = data;
					next.lang = lang;
					next.script = script;
					next.chain_id = MAIN_CHAIN;
					next.index = 0;
					return next;
				} else {
					return TLFontFallbackIterator();
				}
			}
		}
		default: {
			return TLFontFallbackIterator();
		} break;
	}
	return TLFontFallbackIterator();
}

TLFontFallbackIterator::TLFontFallbackIterator(Ref<TLFontFace> p_font) {

	font = p_font;
	data = NULL;
	script = HB_SCRIPT_INVALID;
	lang = HB_LANGUAGE_INVALID;
	index = 0;
	chain_id = SINGLE_FONT;
}

TLFontFallbackIterator::TLFontFallbackIterator(const StyleData *p_data) {
	data = p_data;
	script = HB_SCRIPT_INVALID;
	lang = HB_LANGUAGE_INVALID;

	if (data->main_chain.size() > 0) {
		index = 0;
		chain_id = MAIN_CHAIN;
	} else {
		index = -1;
		chain_id = INVALID_CHAIN;
	}
}

TLFontFallbackIterator::TLFontFallbackIterator(const StyleData *p_data, hb_script_t p_script) {
	data = p_data;
	script = p_script;
	lang = HB_LANGUAGE_INVALID;

	if ((data->linked_src_chain.count(script) > 0) && (data->linked_src_chain.at(script).size() > 0)) {
		index = 0;
		chain_id = SCRIPT_CHAIN;
	} else {
		if (data->main_chain.size() > 0) {
			index = 0;
			chain_id = MAIN_CHAIN;
		} else {
			index = -1;
			chain_id = INVALID_CHAIN;
		}
	}
}

TLFontFallbackIterator::TLFontFallbackIterator(const StyleData *p_data, hb_language_t p_lang) {
	data = p_data;
	script = HB_SCRIPT_INVALID;
	lang = p_lang;

	if ((data->linked_lang_chain.count(lang) > 0) && (data->linked_lang_chain.at(lang).size() > 0)) {
		index = 0;
		chain_id = LANG_CHAIN;
	} else {
		if (data->main_chain.size() > 0) {
			index = 0;
			chain_id = MAIN_CHAIN;
		} else {
			index = -1;
			chain_id = INVALID_CHAIN;
		}
	}
}

/*************************************************************************/

TLFontIterator::TLFontIterator() {
	//NOP
}

TLFontIterator::~TLFontIterator() {
	//NOP
}

void TLFontIterator::_set_iter(const TLFontFallbackIterator &p_iter) {
	_iter = p_iter;
}

void TLFontIterator::_init() {
	//NOP
}

Variant TLFontIterator::_iter_init(const Array p_iter) {
	return _iter.is_valid();
}

Variant TLFontIterator::_iter_next(const Array p_iter) {
	_iter = _iter.next();
	return _iter.is_valid();
}

Variant TLFontIterator::_iter_get(const Array p_iter) {
	if (_iter.is_valid()) {
		return _iter.value();
	} else {
		return Variant();
	}
}

#ifdef GODOT_MODULE
void TLFontIterator::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_iter_init", "iter"), &TLFontIterator::_iter_init);
	ClassDB::bind_method(D_METHOD("_iter_get", "iter"), &TLFontIterator::_iter_get);
	ClassDB::bind_method(D_METHOD("_iter_next", "iter"), &TLFontIterator::_iter_next);
}
#else
void TLFontIterator::_register_methods() {
	register_method("_iter_init", &TLFontIterator::_iter_init);
	register_method("_iter_get", &TLFontIterator::_iter_get);
	register_method("_iter_next", &TLFontIterator::_iter_next);
}
#endif

/*************************************************************************/

TLFontFamily::TLFontFamily() {

#ifdef GODOT_MODULE
	_init();
#endif
}

void TLFontFamily::_init() {
	//NOP
}

TLFontFamily::~TLFontFamily() {
	//NOP
}

void TLFontFamily::add_style(String p_style) {
	if (styles.count(p_style.to_upper()) == 0) {
		styles[p_style.to_upper()] = StyleData();
#ifdef GODOT_MODULE
		_change_notify();
#endif
	}
}

void TLFontFamily::add_script(String p_style, String p_script) {
	if (styles.count(p_style.to_upper()) > 0) {
		hb_script_t scr = hb_script_from_string(p_script.ascii().get_data(), -1);
		if (styles[p_style.to_upper()].linked_src_chain.count(scr) == 0) {
			styles[p_style.to_upper()].linked_src_chain[scr] = std::vector<Ref<TLFontFace> >();
#ifdef GODOT_MODULE
			_change_notify();
#endif
		}
	}
}

void TLFontFamily::add_language(String p_style, String p_lang) {
	if (styles.count(p_style.to_upper()) > 0) {
		hb_language_t lang = hb_language_from_string(p_lang.ascii().get_data(), -1);
		if (styles[p_style.to_upper()].linked_lang_chain.count(lang) == 0) {
			styles[p_style.to_upper()].linked_lang_chain[lang] = std::vector<Ref<TLFontFace> >();
#ifdef GODOT_MODULE
			_change_notify();
#endif
		}
	}
}

void TLFontFamily::remove_script(String p_style, String p_script) {
	if (styles.count(p_style.to_upper()) > 0) {
		hb_script_t scr = hb_script_from_string(p_script.ascii().get_data(), -1);
		if (styles[p_style.to_upper()].linked_src_chain.count(scr) > 0) {
			for (auto E = styles[p_style.to_upper()].linked_src_chain[scr].begin(); E != styles[p_style.to_upper()].linked_src_chain[scr].end(); ++E) {
				_dec_ref(*E);
			}
			styles[p_style.to_upper()].linked_src_chain.erase(scr);
#ifdef GODOT_MODULE
			_change_notify();
#endif
		}
	}
}

void TLFontFamily::remove_language(String p_style, String p_lang) {
	if (styles.count(p_style.to_upper()) > 0) {
		hb_language_t lang = hb_language_from_string(p_lang.ascii().get_data(), -1);
		if (styles[p_style.to_upper()].linked_lang_chain.count(lang) > 0) {
			for (auto E = styles[p_style.to_upper()].linked_lang_chain[lang].begin(); E != styles[p_style.to_upper()].linked_lang_chain[lang].end(); ++E) {
				_dec_ref(*E);
			}
			styles[p_style.to_upper()].linked_lang_chain.erase(lang);
#ifdef GODOT_MODULE
			_change_notify();
#endif
		}
	}
}

void TLFontFamily::remove_style(String p_style) {

	for (auto E = styles[p_style.to_upper()].main_chain.begin(); E != styles[p_style.to_upper()].main_chain.end(); ++E) {
		_dec_ref(*E);
	}
	for (auto E = styles[p_style.to_upper()].linked_src_chain.begin(); E != styles[p_style.to_upper()].linked_src_chain.end(); ++E) {
		for (auto F = E->second.begin(); F != E->second.end(); ++F) {
			_dec_ref(*F);
		}
	}
	for (auto E = styles[p_style.to_upper()].linked_lang_chain.begin(); E != styles[p_style.to_upper()].linked_lang_chain.end(); ++E) {
		for (auto F = E->second.begin(); F != E->second.end(); ++F) {
			_dec_ref(*F);
		}
	}

	styles.erase(p_style.to_upper());
#ifdef GODOT_MODULE
	_change_notify();
#endif
}

bool TLFontFamily::has_style(String p_style) const {

	return styles.count(p_style.to_upper()) > 0;
}

TLFontFallbackIterator TLFontFamily::get_face(String p_style) const {

	if (styles.count(p_style.to_upper()) > 0) {
		return TLFontFallbackIterator(&styles.at(p_style.to_upper()));
	} else {
		return TLFontFallbackIterator();
	}
}

TLFontFallbackIterator TLFontFamily::get_face_for_script(String p_style, hb_script_t p_script) const {

	if (styles.count(p_style.to_upper()) > 0) {
		if (styles.at(p_style.to_upper()).linked_src_chain.count(p_script) > 0) {
			return TLFontFallbackIterator(&styles.at(p_style.to_upper()), p_script);
		} else {
			return TLFontFallbackIterator(&styles.at(p_style.to_upper()));
		}
	}
	return TLFontFallbackIterator();
}

TLFontFallbackIterator TLFontFamily::get_face_for_language(String p_style, hb_language_t p_lang) const {

	if (styles.count(p_style.to_upper()) > 0) {
		if (styles.at(p_style.to_upper()).linked_lang_chain.count(p_lang) > 0) {
			return TLFontFallbackIterator(&styles.at(p_style.to_upper()), p_lang);
		} else {
			return TLFontFallbackIterator(&styles.at(p_style.to_upper()));
		}
	}
	return TLFontFallbackIterator();
}

void TLFontFamily::add_face(String p_style, Ref<TLFontFace> p_ref) {

	if (p_ref.is_valid() && !cast_to<TLFontFace>(*p_ref)) {
		ERR_PRINTS("Type mismatch");
		return;
	}

	std::vector<hb_script_t> scripts = p_ref->unicode_scripts_supported();
	for (int64_t i = 0; i < (int64_t)scripts.size(); i++) {
		if (scripts[i] != HB_SCRIPT_COMMON) {
			//add to scr chain
			_inc_ref(p_ref);
			styles[p_style.to_upper()].linked_src_chain[scripts[i]].push_back(p_ref);
		} else {
			//add to main chain
			_inc_ref(p_ref);
			styles[p_style.to_upper()].main_chain.push_back(p_ref);
		}
	}

#ifdef GODOT_MODULE
	_change_notify();
#endif
}

void TLFontFamily::add_face_unlinked(String p_style, Ref<TLFontFace> p_ref) {

	if (p_ref.is_valid() && !cast_to<TLFontFace>(*p_ref)) {
		ERR_PRINTS("Type mismatch");
		return;
	}

	_inc_ref(p_ref);

	styles[p_style.to_upper()].main_chain.push_back(p_ref);

#ifdef GODOT_MODULE
	_change_notify();
#endif
}

void TLFontFamily::add_face_for_script(String p_style, Ref<TLFontFace> p_ref, String p_script) {

	if (p_ref.is_valid() && !cast_to<TLFontFace>(*p_ref)) {
		ERR_PRINTS("Type mismatch");
		return;
	}

	_inc_ref(p_ref);

	hb_script_t scr = hb_script_from_string(p_script.ascii().get_data(), -1);
	styles[p_style.to_upper()].linked_src_chain[scr].push_back(p_ref);

#ifdef GODOT_MODULE
	_change_notify();
#endif
}

void TLFontFamily::add_face_for_language(String p_style, Ref<TLFontFace> p_ref, String p_lang) {

	if (p_ref.is_valid() && !cast_to<TLFontFace>(*p_ref)) {
		ERR_PRINTS("Type mismatch");
		return;
	}

	_inc_ref(p_ref);

	hb_language_t lang = hb_language_from_string(p_lang.ascii().get_data(), -1);
	styles[p_style.to_upper()].linked_lang_chain[lang].push_back(p_ref);

#ifdef GODOT_MODULE
	_change_notify();
#endif
}

Ref<TLFontIterator> TLFontFamily::_get_face(String p_style) const {
	Ref<TLFontIterator> iter;
#ifdef GODOT_MODULE
	iter.instance();
#else
	iter = Ref<TLFontIterator>::__internal_constructor(TLFontIterator::_new());
#endif
	iter->_set_iter(get_face(p_style));

	return iter;
}

Ref<TLFontIterator> TLFontFamily::_get_face_for_script(String p_style, String p_script) const {
	Ref<TLFontIterator> iter;
#ifdef GODOT_MODULE
	iter.instance();
#else
	iter = Ref<TLFontIterator>::__internal_constructor(TLFontIterator::_new());
#endif
	hb_script_t scr = hb_script_from_string(p_script.ascii().get_data(), -1);
	iter->_set_iter(get_face_for_script(p_style, scr));

	return iter;
}

Ref<TLFontIterator> TLFontFamily::_get_face_for_language(String p_style, String p_lang) const {
	Ref<TLFontIterator> iter;
#ifdef GODOT_MODULE
	iter.instance();
#else
	iter = Ref<TLFontIterator>::__internal_constructor(TLFontIterator::_new());
#endif
	hb_language_t lang = hb_language_from_string(p_lang.ascii().get_data(), -1);
	iter->_set_iter(get_face_for_language(p_style, lang));

	return iter;
}

void TLFontFamily::_font_changed() {
	emit_signal(_CHANGED);
#ifdef GODOT_MODULE
	_change_notify();
#endif
}

void TLFontFamily::_inc_ref(Ref<TLFontFace> &p_font) {
	if (p_font.is_null()) return;
	if (_ref_count.count(p_font) == 0) {
		p_font->connect(_CHANGED, this, "_font_changed");
		_ref_count[p_font] = 1;
	} else {
		_ref_count[p_font]++;
	}
}

void TLFontFamily::_dec_ref(Ref<TLFontFace> &p_font) {
	if (p_font.is_null()) return;
	if (_ref_count.count(p_font) == 1) {
		if (p_font->is_connected(_CHANGED, this, "_font_changed")) {
			p_font->disconnect(_CHANGED, this, "_font_changed");
		}
		_ref_count.erase(p_font);
	} else {
		_ref_count[p_font]--;
	}
}

#ifdef GODOT_MODULE

bool TLFontFamily::_set(const StringName &p_name, const Variant &p_value) {

	String name = p_name;
	Vector<String> tokens = name.split("/");
	if (tokens.size() >= 2) {
		String style = tokens[0];
		if (tokens.size() == 2) {
			int64_t index = (int64_t)tokens[1].to_int64();
			if (index == (int64_t)styles[style.to_upper()].main_chain.size()) {
				Ref<TLFontFace> face = p_value;
				if (face.is_valid()) {
					_inc_ref(face);
					styles[style.to_upper()].main_chain.push_back(face);
					_change_notify();
					return true;
				}
			} else if ((index >= 0) && (index < (int64_t)styles[style.to_upper()].main_chain.size())) {
				Ref<TLFontFace> face = p_value;
				if (face.is_null()) {
					_dec_ref(styles[style.to_upper()].main_chain[index]);
					styles[style.to_upper()].main_chain.erase(styles[style.to_upper()].main_chain.begin() + index);
					_change_notify();
					return true;
				} else {
					_dec_ref(styles[style.to_upper()].main_chain[index]);
					_inc_ref(face);
					styles[style.to_upper()].main_chain[index] = face;
					_change_notify();
					return true;
				}
			}
		} else if (tokens.size() == 4) {
			int64_t index = (int64_t)tokens[3].to_int64();
			if (tokens[1] == "script") {
				//add script
				hb_script_t scr = hb_script_from_string(tokens[2].ascii().get_data(), -1);
				if (index == (int64_t)styles[style.to_upper()].linked_src_chain[scr].size()) {
					Ref<TLFontFace> face = p_value;
					if (face.is_valid()) {
						_inc_ref(face);
						styles[style.to_upper()].linked_src_chain[scr].push_back(face);
						_change_notify();
						return true;
					}
				} else if ((index >= 0) && (index < (int64_t)styles[style.to_upper()].linked_src_chain[scr].size())) {
					Ref<TLFontFace> face = p_value;
					if (face.is_null()) {
						_dec_ref(styles[style.to_upper()].linked_src_chain[scr][index]);
						styles[style.to_upper()].linked_src_chain[scr].erase(styles[style.to_upper()].linked_src_chain[scr].begin() + index);
						_change_notify();
						return true;
					} else {
						_dec_ref(styles[style.to_upper()].linked_src_chain[scr][index]);
						_inc_ref(face);
						styles[style.to_upper()].linked_src_chain[scr][index] = face;
						_change_notify();
						return true;
					}
				}
			} else if (tokens[1] == "lang") {
				//add lang
				hb_language_t lang = hb_language_from_string(tokens[2].ascii().get_data(), -1);
				if (index == (int64_t)styles[style.to_upper()].linked_lang_chain[lang].size()) {
					Ref<TLFontFace> face = p_value;
					if (face.is_valid()) {
						_inc_ref(face);
						styles[style.to_upper()].linked_lang_chain[lang].push_back(face);
						_change_notify();
						return true;
					}
				} else if ((index >= 0) && (index < (int64_t)styles[style.to_upper()].linked_lang_chain[lang].size())) {
					Ref<TLFontFace> face = p_value;
					if (face.is_null()) {
						_dec_ref(styles[style.to_upper()].linked_lang_chain[lang][index]);
						styles[style.to_upper()].linked_lang_chain[lang].erase(styles[style.to_upper()].linked_lang_chain[lang].begin() + index);
						_change_notify();
						return true;
					} else {
						_dec_ref(styles[style.to_upper()].linked_lang_chain[lang][index]);
						_inc_ref(face);
						styles[style.to_upper()].linked_lang_chain[lang][index] = face;
						_change_notify();
						return true;
					}
				}
			}
		}
	}

	return false;
}

bool TLFontFamily::_get(const StringName &p_name, Variant &r_ret) const {

	String name = p_name;

	Vector<String> tokens = name.split("/");
	if (tokens.size() >= 2) {
		String style = tokens[0];
		if (styles.count(style.to_upper()) > 0) {
			if (tokens.size() == 2) {
				int index = (int)tokens[1].to_int();
				if ((index >= 0) && (index < (int64_t)styles.at(style.to_upper()).main_chain.size())) {
					r_ret = styles.at(style.to_upper()).main_chain[index];
					return true;
				}
			} else if (tokens.size() == 4) {
				int index = (int)tokens[3].to_int();
				if (tokens[1] == "script") {
					//add script
					hb_script_t scr = hb_script_from_string(tokens[2].ascii().get_data(), -1);
					if (styles.at(style.to_upper()).linked_src_chain.count(scr) > 0) {
						if ((index >= 0) && (index < (int64_t)styles.at(style.to_upper()).linked_src_chain.at(scr).size())) {
							r_ret = styles.at(style.to_upper()).linked_src_chain.at(scr)[index];
							return true;
						}
					}
				} else if (tokens[1] == "lang") {
					//add lang
					hb_language_t lang = hb_language_from_string(tokens[2].ascii().get_data(), -1);
					if (styles.at(style.to_upper()).linked_lang_chain.count(lang) > 0) {
						if ((index >= 0) && (index < (int64_t)styles.at(style.to_upper()).linked_lang_chain.at(lang).size())) {
							r_ret = styles.at(style.to_upper()).linked_lang_chain.at(lang)[index];
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

void TLFontFamily::_get_property_list(List<PropertyInfo> *p_list) const {

	for (auto it = styles.begin(); it != styles.end(); ++it) {
		p_list->push_back(PropertyInfo(Variant::NIL, it->first.to_lower() + "/" + "_prev_style", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_INTERNAL));
		for (int64_t i = 0; i < (int64_t)it->second.main_chain.size(); i++) {
			p_list->push_back(PropertyInfo(Variant::OBJECT, it->first.to_lower() + "/" + String::num_int64(i), PROPERTY_HINT_RESOURCE_TYPE, "TLFontFace"));
		}
		p_list->push_back(PropertyInfo(Variant::OBJECT, it->first.to_lower() + "/" + String::num_int64(it->second.main_chain.size()), PROPERTY_HINT_RESOURCE_TYPE, "TLFontFace"));

		for (auto sit = it->second.linked_src_chain.begin(); sit != it->second.linked_src_chain.end(); ++sit) {
			char tag[5] = "";
			hb_tag_to_string(hb_script_to_iso15924_tag(sit->first), tag);
			for (int64_t i = 0; i < (int64_t)sit->second.size(); i++) {
				p_list->push_back(PropertyInfo(Variant::OBJECT, it->first.to_lower() + "/script/" + tag + "/" + String::num_int64(i), PROPERTY_HINT_RESOURCE_TYPE, "TLFontFace"));
			}
			p_list->push_back(PropertyInfo(Variant::OBJECT, it->first.to_lower() + "/script/" + tag + "/" + String::num_int64(sit->second.size()), PROPERTY_HINT_RESOURCE_TYPE, "TLFontFace"));
			p_list->push_back(PropertyInfo(Variant::NIL, it->first.to_lower() + "/script/" + tag + "/" + "_remove_script", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_INTERNAL));
		}
		p_list->push_back(PropertyInfo(Variant::NIL, it->first.to_lower() + "/" + "_add_script", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_INTERNAL));
		for (auto sit = it->second.linked_lang_chain.begin(); sit != it->second.linked_lang_chain.end(); ++sit) {
			for (int64_t i = 0; i < (int64_t)sit->second.size(); i++) {
				p_list->push_back(PropertyInfo(Variant::OBJECT, it->first.to_lower() + "/lang/" + hb_language_to_string(sit->first) + "/" + String::num_int64(i), PROPERTY_HINT_RESOURCE_TYPE, "TLFontFace"));
			}
			p_list->push_back(PropertyInfo(Variant::OBJECT, it->first.to_lower() + "/lang/" + hb_language_to_string(sit->first) + "/" + String::num_int64(sit->second.size()), PROPERTY_HINT_RESOURCE_TYPE, "TLFontFace"));
			p_list->push_back(PropertyInfo(Variant::NIL, it->first.to_lower() + "/lang/" + hb_language_to_string(sit->first) + "/" + "_remove_lang", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_INTERNAL));
		}
		p_list->push_back(PropertyInfo(Variant::NIL, it->first.to_lower() + "/" + "_add_lang", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_INTERNAL));
		p_list->push_back(PropertyInfo(Variant::NIL, it->first.to_lower() + "/" + "_remove_style", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_INTERNAL));
	}
	p_list->push_back(PropertyInfo(Variant::NIL, "_new_style", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_INTERNAL));
}

void TLFontFamily::_bind_methods() {

	ClassDB::bind_method(D_METHOD("_font_changed"), &TLFontFamily::_font_changed);

	ClassDB::bind_method(D_METHOD("add_style", "style"), &TLFontFamily::add_style);
	ClassDB::bind_method(D_METHOD("remove_style", "style"), &TLFontFamily::remove_style);
	ClassDB::bind_method(D_METHOD("has_style", "style"), &TLFontFamily::has_style);

	ClassDB::bind_method(D_METHOD("add_script", "style", "script"), &TLFontFamily::add_script);
	ClassDB::bind_method(D_METHOD("remove_script", "style", "script"), &TLFontFamily::remove_script);

	ClassDB::bind_method(D_METHOD("add_language", "style", "language"), &TLFontFamily::add_language);
	ClassDB::bind_method(D_METHOD("remove_language", "style", "language"), &TLFontFamily::remove_language);

	ClassDB::bind_method(D_METHOD("add_face", "style", "ref"), &TLFontFamily::add_face);
	ClassDB::bind_method(D_METHOD("add_face_unlinked", "style", "ref"), &TLFontFamily::add_face_unlinked);
	ClassDB::bind_method(D_METHOD("add_face_for_script", "style", "ref", "script"), &TLFontFamily::add_face_for_script);
	ClassDB::bind_method(D_METHOD("add_face_for_language", "style", "ref", "lang"), &TLFontFamily::add_face_for_language);

	ClassDB::bind_method(D_METHOD("get_face", "style"), &TLFontFamily::_get_face);
	ClassDB::bind_method(D_METHOD("get_face_for_script", "style", "script"), &TLFontFamily::_get_face_for_script);
	ClassDB::bind_method(D_METHOD("get_face_for_language", "style", "lang"), &TLFontFamily::_get_face_for_language);
}

#else

bool TLFontFamily::_set(String p_name, Variant p_value) {

	String name = p_name;
	PoolStringArray tokens = name.split("/");
	if (tokens.size() >= 2) {
		String style = tokens[0];
		if (tokens.size() == 2) {
			int64_t index = (int64_t)tokens[1].to_int();
			if (index == (int64_t)styles[style.to_upper()].main_chain.size()) {
				Ref<TLFontFace> face = p_value;
				if (face.is_valid()) {
					_inc_ref(face);
					styles[style.to_upper()].main_chain.push_back(face);
					return true;
				}
			} else if ((index >= 0) && (index < (int64_t)styles[style.to_upper()].main_chain.size())) {
				Ref<TLFontFace> face = p_value;
				if (face.is_null()) {
					_dec_ref(styles[style.to_upper()].main_chain[index]);
					styles[style.to_upper()].main_chain.erase(styles[style.to_upper()].main_chain.begin() + index);
					return true;
				} else {
					_dec_ref(styles[style.to_upper()].main_chain[index]);
					_inc_ref(face);
					styles[style.to_upper()].main_chain[index] = face;
					return true;
				}
			}
		} else if (tokens.size() == 4) {
			int64_t index = (int64_t)tokens[3].to_int();
			if (tokens[1] == "script") {
				//add script
				hb_script_t scr = hb_script_from_string(tokens[2].ascii().get_data(), -1);
				if (index == (int64_t)styles[style.to_upper()].linked_src_chain[scr].size()) {
					Ref<TLFontFace> face = p_value;
					if (face.is_valid()) {
						_inc_ref(face);
						styles[style.to_upper()].linked_src_chain[scr].push_back(face);
						return true;
					}
				} else if ((index >= 0) && (index < (int64_t)styles[style.to_upper()].linked_src_chain[scr].size())) {
					Ref<TLFontFace> face = p_value;
					if (face.is_null()) {
						_dec_ref(styles[style.to_upper()].linked_src_chain[scr][index]);
						styles[style.to_upper()].linked_src_chain[scr].erase(styles[style.to_upper()].linked_src_chain[scr].begin() + index);
						return true;
					} else {
						_dec_ref(styles[style.to_upper()].linked_src_chain[scr][index]);
						_inc_ref(face);
						styles[style.to_upper()].linked_src_chain[scr][index] = face;
						return true;
					}
				}
			} else if (tokens[1] == "lang") {
				//add lang
				hb_language_t lang = hb_language_from_string(tokens[2].ascii().get_data(), -1);
				if (index == (int64_t)styles[style.to_upper()].linked_lang_chain[lang].size()) {
					Ref<TLFontFace> face = p_value;
					if (face.is_valid()) {
						_inc_ref(face);
						styles[style.to_upper()].linked_lang_chain[lang].push_back(face);
						return true;
					}
				} else if ((index >= 0) && (index < (int64_t)styles[style.to_upper()].linked_lang_chain[lang].size())) {
					Ref<TLFontFace> face = p_value;
					if (face.is_null()) {
						_dec_ref(styles[style.to_upper()].linked_lang_chain[lang][index]);
						styles[style.to_upper()].linked_lang_chain[lang].erase(styles[style.to_upper()].linked_lang_chain[lang].begin() + index);
						return true;
					} else {
						_dec_ref(styles[style.to_upper()].linked_lang_chain[lang][index]);
						_inc_ref(face);
						styles[style.to_upper()].linked_lang_chain[lang][index] = face;
						return true;
					}
				}
			}
		}
	}

	return false;
}

Variant TLFontFamily::_get(String p_name) const {

	String name = p_name;

	PoolStringArray tokens = name.split("/");
	if (tokens.size() >= 2) {
		String style = tokens[0];
		if (styles.count(style.to_upper()) > 0) {
			if (tokens.size() == 2) {
				int64_t index = (int64_t)tokens[1].to_int();
				if ((index >= 0) && (index < (int64_t)styles.at(style.to_upper()).main_chain.size())) {
					return styles.at(style.to_upper()).main_chain[index];
				}
			} else if (tokens.size() == 4) {
				int64_t index = (int64_t)tokens[3].to_int();
				if (tokens[1] == "script") {
					//add script
					hb_script_t scr = hb_script_from_string(tokens[2].ascii().get_data(), -1);
					if (styles.at(style.to_upper()).linked_src_chain.count(scr) > 0) {
						if ((index >= 0) && (index < styles.at(style.to_upper()).linked_src_chain.at(scr).size())) {
							return styles.at(style.to_upper()).linked_src_chain.at(scr)[index];
						}
					}
				} else if (tokens[1] == "lang") {
					//add lang
					hb_language_t lang = hb_language_from_string(tokens[2].ascii().get_data(), -1);
					if (styles.at(style.to_upper()).linked_lang_chain.count(lang) > 0) {
						if ((index >= 0) && (index < styles.at(style.to_upper()).linked_lang_chain.at(lang).size())) {
							return styles.at(style.to_upper()).linked_lang_chain.at(lang)[index];
						}
					}
				}
			}
		}
	}

	return Variant();
}

Array TLFontFamily::_get_property_list() const {

	Array ret;
	for (auto it = styles.begin(); it != styles.end(); ++it) {

		// {
		// 	Dictionary prop;
		// 	prop["name"] = it->first.to_lower() + "/" + "_prev_style";
		// 	prop["type"] = GlobalConstants::TYPE_NIL;
		// 	prop["hint"] = GlobalConstants::PROPERTY_HINT_NONE;
		// 	prop["hint_string"] = "";
		// 	prop["usage"] = GlobalConstants::PROPERTY_USAGE_EDITOR;
		// 	ret.push_back(prop);
		// }

		for (int i = 0; i < it->second.main_chain.size(); i++) {
			Dictionary prop;
			prop["name"] = it->first.to_lower() + "/" + String::num_int64(i);
			prop["type"] = GlobalConstants::TYPE_OBJECT;
			prop["hint"] = GlobalConstants::PROPERTY_HINT_RESOURCE_TYPE;
			prop["hint_string"] = "TLFontFace";
			prop["usage"] = GlobalConstants::PROPERTY_USAGE_NOEDITOR | GlobalConstants::PROPERTY_USAGE_STORAGE;
			ret.push_back(prop);
		}
		// {
		// 	Dictionary prop;
		// 	prop["name"] = it->first.to_lower() + "/" + String::num_int64(it->second.main_chain.size());
		// 	prop["type"] = GlobalConstants::TYPE_OBJECT;
		// 	prop["hint"] = GlobalConstants::PROPERTY_HINT_RESOURCE_TYPE;
		// 	prop["hint_string"] = "TLFontFace";
		// 	prop["usage"] = GlobalConstants::PROPERTY_USAGE_EDITOR;
		// 	ret.push_back(prop);
		// }

		for (auto sit = it->second.linked_src_chain.begin(); sit != it->second.linked_src_chain.end(); ++sit) {
			char tag[5] = "";
			hb_tag_to_string(hb_script_to_iso15924_tag(sit->first), tag);
			for (int i = 0; i < sit->second.size(); i++) {
				Dictionary prop;
				prop["name"] = it->first.to_lower() + "/script/" + tag + "/" + String::num_int64(i);
				prop["type"] = GlobalConstants::TYPE_OBJECT;
				prop["hint"] = GlobalConstants::PROPERTY_HINT_RESOURCE_TYPE;
				prop["hint_string"] = "TLFontFace";
				prop["usage"] = GlobalConstants::PROPERTY_USAGE_NOEDITOR | GlobalConstants::PROPERTY_USAGE_STORAGE;
				ret.push_back(prop);
			}
			// {
			// 	Dictionary prop;
			// 	prop["name"] = it->first.to_lower() + "/script/" + tag + "/" + String::num_int64(sit->second.size());
			// 	prop["type"] = GlobalConstants::TYPE_OBJECT;
			// 	prop["hint"] = GlobalConstants::PROPERTY_HINT_RESOURCE_TYPE;
			// 	prop["hint_string"] = "TLFontFace";
			// 	prop["usage"] = GlobalConstants::PROPERTY_USAGE_EDITOR | GlobalConstants::PROPERTY_USAGE_STORAGE;
			// 	ret.push_back(prop);
			// }
			// {
			// 	Dictionary prop;
			// 	prop["name"] = it->first.to_lower() + "/script/" + tag + "/" + "_remove_script";
			// 	prop["type"] = GlobalConstants::TYPE_NIL;
			// 	prop["hint"] = GlobalConstants::PROPERTY_HINT_NONE;
			// 	prop["hint_string"] = "";
			// 	prop["usage"] = GlobalConstants::PROPERTY_USAGE_EDITOR;
			// 	ret.push_back(prop);
			// }
		}
		// {
		// 	Dictionary prop;
		// 	prop["name"] = it->first.to_lower() + "/" + "_add_script";
		// 	prop["type"] = GlobalConstants::TYPE_NIL;
		// 	prop["hint"] = GlobalConstants::PROPERTY_HINT_NONE;
		// 	prop["hint_string"] = "";
		// 	prop["usage"] = GlobalConstants::PROPERTY_USAGE_EDITOR;
		// 	ret.push_back(prop);
		// }
		for (auto sit = it->second.linked_lang_chain.begin(); sit != it->second.linked_lang_chain.end(); ++sit) {

			for (int i = 0; i < sit->second.size(); i++) {
				Dictionary prop;
				prop["name"] = it->first.to_lower() + "/lang/" + hb_language_to_string(sit->first) + "/" + String::num_int64(i);
				prop["type"] = GlobalConstants::TYPE_OBJECT;
				prop["hint"] = GlobalConstants::PROPERTY_HINT_RESOURCE_TYPE;
				prop["hint_string"] = "TLFontFace";
				prop["usage"] = GlobalConstants::PROPERTY_USAGE_NOEDITOR | GlobalConstants::PROPERTY_USAGE_STORAGE;
				ret.push_back(prop);
			}
			// {
			// 	Dictionary prop;
			// 	prop["name"] = it->first.to_lower() + "/lang/" + hb_language_to_string(sit->first) + "/" + String::num_int64(sit->second.size());
			// 	prop["type"] = GlobalConstants::TYPE_OBJECT;
			// 	prop["hint"] = GlobalConstants::PROPERTY_HINT_RESOURCE_TYPE;
			// 	prop["hint_string"] = "TLFontFace";
			// 	prop["usage"] = GlobalConstants::PROPERTY_USAGE_EDITOR | GlobalConstants::PROPERTY_USAGE_STORAGE;
			// 	ret.push_back(prop);
			// }
			// {
			// 	Dictionary prop;
			// 	prop["name"] = it->first.to_lower() + "/lang/" + hb_language_to_string(sit->first) + "/" + "_remove_script";
			// 	prop["type"] = GlobalConstants::TYPE_NIL;
			// 	prop["hint"] = GlobalConstants::PROPERTY_HINT_NONE;
			// 	prop["hint_string"] = "";
			// 	prop["usage"] = GlobalConstants::PROPERTY_USAGE_EDITOR;
			// 	ret.push_back(prop);
			// }
		}
		// {
		// 	Dictionary prop;
		// 	prop["name"] = it->first.to_lower() + "/" + "_add_lang";
		// 	prop["type"] = GlobalConstants::TYPE_NIL;
		// 	prop["hint"] = GlobalConstants::PROPERTY_HINT_NONE;
		// 	prop["hint_string"] = "";
		// 	prop["usage"] = GlobalConstants::PROPERTY_USAGE_EDITOR;
		// 	ret.push_back(prop);
		// }
		// {
		// 	Dictionary prop;
		// 	prop["name"] = it->first.to_lower() + "/" + "_remove_style";
		// 	prop["type"] = GlobalConstants::TYPE_NIL;
		// 	prop["hint"] = GlobalConstants::PROPERTY_HINT_NONE;
		// 	prop["hint_string"] = "";
		// 	prop["usage"] = GlobalConstants::PROPERTY_USAGE_EDITOR;
		// 	ret.push_back(prop);
		// }
	}
	// {
	// 	Dictionary prop;
	// 	prop["name"] = "_new_style";
	// 	prop["type"] = GlobalConstants::TYPE_NIL;
	// 	prop["hint"] = GlobalConstants::PROPERTY_HINT_NONE;
	// 	prop["hint_string"] = "";
	// 	prop["usage"] = GlobalConstants::PROPERTY_USAGE_EDITOR;
	// 	ret.push_back(prop);
	// }
	return ret;
}

void TLFontFamily::_register_methods() {

	register_method("_font_changed", &TLFontFamily::_font_changed);

	register_method("add_style", &TLFontFamily::add_style);
	register_method("remove_style", &TLFontFamily::remove_style);
	register_method("has_style", &TLFontFamily::has_style);

	register_method("add_script", &TLFontFamily::add_script);
	register_method("remove_script", &TLFontFamily::remove_script);

	register_method("add_language", &TLFontFamily::add_language);
	register_method("remove_language", &TLFontFamily::remove_language);

	register_method("add_face", &TLFontFamily::add_face);
	register_method("add_face_unlinked", &TLFontFamily::add_face_unlinked);
	register_method("add_face_for_script", &TLFontFamily::add_face_for_script);
	register_method("add_face_for_language", &TLFontFamily::add_face_for_language);

	register_method("_get_property_list", &TLFontFamily::_get_property_list);
	register_method("_get", &TLFontFamily::_get);
	register_method("_set", &TLFontFamily::_set);

	register_method("get_face", &TLFontFamily::_get_face);
	register_method("get_face_for_script", &TLFontFamily::_get_face_for_script);
	register_method("get_face_for_language", &TLFontFamily::_get_face_for_language);
}

#endif
