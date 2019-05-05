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
				if (index < data->main_chain.size()) {
					return data->main_chain[index];
				}
			} break;
			case SCRIPT_CHAIN: {
				if (index < data->linked_src_chain.at(script).size()) {
					return data->linked_src_chain.at(script)[index];
				}
			} break;
			case LANG_CHAIN: {
				if (index < data->linked_lang_chain.at(lang).size()) {
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
			if (index < data->main_chain.size() - 1) {
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
			if (index < data->linked_src_chain.at(script).size() - 1) {
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
			if (index < data->linked_lang_chain.at(lang).size() - 1) {
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

void TLFontFamily::remove_style(String p_style) {

	styles.erase(p_style.to_upper());
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
	for (int i = 0; i < scripts.size(); i++) {
		if (scripts[i] != HB_SCRIPT_COMMON) {
			//add to scr chain
			styles[p_style.to_upper()].linked_src_chain[scripts[i]].push_back(p_ref);
		} else {
			//add to main chain
			styles[p_style.to_upper()].main_chain.push_back(p_ref);
		}
	}

#ifdef GODOT_MODULE
	_change_notify();
#endif
}

/*
void TLFontFamily::set_face(String p_style, Ref<TLFontFace> p_ref) {

	if (p_ref.is_valid() && !cast_to<TLFontFace>(*p_ref)) {
		ERR_PRINTS("Type mismatch");
		return;
	}

	if (!p_ref.is_valid()) {
		if (styles.count(p_style.to_upper()) == 0)
			return;
	}

	styles[p_style.to_upper()].main = p_ref;
#ifdef GODOT_MODULE
	_change_notify();
#endif
}

Ref<TLFontFace> TLFontFamily::get_face(String p_style) const {

	if (styles.count(p_style.to_upper()) > 0)
		return styles.at(p_style.to_upper()).main;

	return Ref<TLFontFace>();
}
*/

void TLFontFamily::add_face_for_script(String p_style, Ref<TLFontFace> p_ref, String p_script) {

	if (p_ref.is_valid() && !cast_to<TLFontFace>(*p_ref)) {
		ERR_PRINTS("Type mismatch");
		return;
	}

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

	hb_language_t lang = hb_language_from_string(p_lang.ascii().get_data(), -1);
	styles[p_style.to_upper()].linked_lang_chain[lang].push_back(p_ref);

#ifdef GODOT_MODULE
	_change_notify();
#endif
}

/*
void TLFontFamily::set_liked_face_for_script(String p_style, String p_script, Ref<TLFontFace> p_ref) {

	if (p_ref.is_valid() && !cast_to<TLFontFace>(*p_ref)) {
		ERR_PRINTS("Type mismatch");
		return;
	}

	hb_script_t scr = hb_script_from_string(p_script.ascii().get_data(), -1);

	if (!p_ref.is_valid()) {
		if (styles.count(p_style.to_upper()) == 0)
			return;
		if (styles[p_style.to_upper()].linked.count(scr) == 0)
			return;
	}

	styles[p_style.to_upper()].linked[scr] = p_ref;
#ifdef GODOT_MODULE
	_change_notify();
#endif
}

Ref<TLFontFace> TLFontFamily::_get_liked_face_for_script(String p_style, hb_script_t p_script) const {

	if (styles.count(p_style.to_upper()) > 0)
		if (styles.at(p_style.to_upper()).linked.count(p_script) > 0)
			return styles.at(p_style.to_upper()).linked.at(p_script);

	return Ref<TLFontFace>();
}

Ref<TLFontFace> TLFontFamily::get_liked_face_for_script(String p_style, String p_script) const {

	hb_script_t scr = hb_script_from_string(p_script.ascii().get_data(), -1);
	if (styles.count(p_style.to_upper()) > 0)
		if (styles.at(p_style.to_upper()).linked.count(scr) > 0)
			return styles.at(p_style.to_upper()).linked.at(scr);

	return Ref<TLFontFace>();
}
*/

#ifdef GODOT_MODULE

bool TLFontFamily::_set(const StringName &p_name, const Variant &p_value) {

	String name = p_name;
	Vector<String> tokens = name.split("/");
	if (tokens.size() >= 2) {
		String style = tokens[0];

		if (tokens.size() == 2) {
			int index = (int)tokens[1].to_int();
			if (index == styles[style.to_upper()].main_chain.size()) {
				styles[style.to_upper()].main_chain.push_back(p_value);
				return true;
			} else if ((index >= 0) && (index < styles[style.to_upper()].main_chain.size())) {
				styles[style.to_upper()].main_chain[index] = p_value;
				return true;
			}
		} else if (tokens.size() == 4) {
			int index = (int)tokens[3].to_int();
			if (tokens[1] == "script") {
				//add script
				hb_script_t scr = hb_script_from_string(tokens[2].ascii().get_data(), -1);
				if (index == styles[style.to_upper()].linked_src_chain[scr].size()) {
					styles[style.to_upper()].linked_src_chain[scr].push_back(p_value);
					return true;
				} else if ((index >= 0) && (index < styles[style.to_upper()].linked_src_chain[scr].size())) {
					styles[style.to_upper()].linked_src_chain[scr][index] = p_value;
					return true;
				}
			} else if (tokens[1] == "lang") {
				//add lang
				hb_language_t lang = hb_language_from_string(tokens[2].ascii().get_data(), -1);
				if (index == styles[style.to_upper()].linked_lang_chain[lang].size()) {
					styles[style.to_upper()].linked_lang_chain[lang].push_back(p_value);
					return true;
				} else if ((index >= 0) && (index < styles[style.to_upper()].linked_lang_chain[lang].size())) {
					styles[style.to_upper()].linked_lang_chain[lang][index] = p_value;
					return true;
				}
			}
		}
	}

	return false;
}

bool TLFontFamily::_get(const StringName &p_name, Variant &r_ret) const {

	String name = p_name;

	Vector<String> tokens = name.split("/");
	if ((tokens.size() >= 2) && (styles.count(tokens[0]))) {
		String style = tokens[0];
		if (styles.count(style.to_upper()) > 0) {
			if (tokens.size() == 2) {
				int index = (int)tokens[1].to_int();
				if ((index >= 0) && (index < styles.at(style.to_upper()).main_chain.size())) {
					r_ret = styles.at(style.to_upper()).main_chain[index];
					return true;
				}

			} else if (tokens.size() == 4) {
				int index = (int)tokens[3].to_int();
				if (tokens[1] == "script") {
					//add script
					hb_script_t scr = hb_script_from_string(tokens[2].ascii().get_data(), -1);
					if (styles.at(style.to_upper()).linked_src_chain.count(scr) > 0) {
						if ((index >= 0) && (index < styles.at(style.to_upper()).linked_src_chain.at(scr).size())) {
							r_ret = styles.at(style.to_upper()).linked_src_chain.at(scr)[index];
							return true;
						}
					}

				} else if (tokens[1] == "lang") {
					//add lang
					hb_language_t lang = hb_language_from_string(tokens[2].ascii().get_data(), -1);
					if (styles.at(style.to_upper()).linked_lang_chain.count(lang) > 0) {
						if ((index >= 0) && (index < styles.at(style.to_upper()).linked_lang_chain.at(lang).size())) {
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

		for (int i = 0; i < it->second.main_chain.size(); i++) {
			p_list->push_back(PropertyInfo(Variant::OBJECT, it->first.to_lower() + "/" + String::num_int64(i), PROPERTY_HINT_RESOURCE_TYPE, "TLFontFace", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_STORAGE));
		}

		for (auto sit = it->second.linked_src_chain.begin(); sit != it->second.linked_src_chain.end(); ++sit) {
			for (int i = 0; i < sit->second.size(); i++) {
				char tag[5] = "";
				hb_tag_to_string(hb_script_to_iso15924_tag(sit->first), tag);
				p_list->push_back(PropertyInfo(Variant::OBJECT, it->first.to_lower() + "/script/" + tag + "/" + String::num_int64(i), PROPERTY_HINT_RESOURCE_TYPE, "TLFontFace", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_STORAGE));
			}
		}
		for (auto sit = it->second.linked_lang_chain.begin(); sit != it->second.linked_lang_chain.end(); ++sit) {
			for (int i = 0; i < sit->second.size(); i++) {
				p_list->push_back(PropertyInfo(Variant::OBJECT, it->first.to_lower() + "/lang/" + hb_language_to_string(sit->first) + "/" + String::num_int64(i), PROPERTY_HINT_RESOURCE_TYPE, "TLFontFace", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_STORAGE));
			}
		}
	}
}

void TLFontFamily::_bind_methods() {

	ClassDB::bind_method(D_METHOD("remove_style", "style"), &TLFontFamily::remove_style);
	ClassDB::bind_method(D_METHOD("has_style", "style"), &TLFontFamily::has_style);

	ClassDB::bind_method(D_METHOD("add_face", "style", "ref"), &TLFontFamily::add_face);

	//ClassDB::bind_method(D_METHOD("set_face", "style", "ref"), &TLFontFamily::set_face);
	//ClassDB::bind_method(D_METHOD("get_face", "style"), &TLFontFamily::get_face);

	ClassDB::bind_method(D_METHOD("add_face_for_script", "style", "ref", "script"), &TLFontFamily::add_face_for_script);
	ClassDB::bind_method(D_METHOD("add_face_for_language", "style", "ref", "lang"), &TLFontFamily::add_face_for_language);

	//ClassDB::bind_method(D_METHOD("set_liked_face_for_script", "style", "script", "ref"), &TLFontFamily::set_liked_face_for_script);
	//ClassDB::bind_method(D_METHOD("get_liked_face_for_script", "style", "script"), &TLFontFamily::get_liked_face_for_script);
}

#else

bool TLFontFamily::_set(String p_name, Variant p_value) {

	String name = p_name;
	PoolStringArray tokens = name.split("/");
	if (tokens.size() >= 2) {
		String style = tokens[0];

		if (tokens.size() == 2) {
			int index = (int)tokens[1].to_int();
			if (index == styles[style.to_upper()].main_chain.size()) {
				styles[style.to_upper()].main_chain.push_back(p_value);
				return true;
			} else if ((index >= 0) && (index < styles[style.to_upper()].main_chain.size())) {
				styles[style.to_upper()].main_chain[index] = p_value;
				return true;
			}
		} else if (tokens.size() == 4) {
			int index = (int)tokens[3].to_int();
			if (tokens[1] == "script") {
				//add script
				hb_script_t scr = hb_script_from_string(tokens[2].ascii().get_data(), -1);
				if (index == styles[style.to_upper()].linked_src_chain[scr].size()) {
					styles[style.to_upper()].linked_src_chain[scr].push_back(p_value);
					return true;
				} else if ((index >= 0) && (index < styles[style.to_upper()].linked_src_chain[scr].size())) {
					styles[style.to_upper()].linked_src_chain[scr][index] = p_value;
					return true;
				}
			} else if (tokens[1] == "lang") {
				//add lang
				hb_language_t lang = hb_language_from_string(tokens[2].ascii().get_data(), -1);
				if (index == styles[style.to_upper()].linked_lang_chain[lang].size()) {
					styles[style.to_upper()].linked_lang_chain[lang].push_back(p_value);
					return true;
				} else if ((index >= 0) && (index < styles[style.to_upper()].linked_lang_chain[lang].size())) {
					styles[style.to_upper()].linked_lang_chain[lang][index] = p_value;
					return true;
				}
			}
		}
	}

	return false;
}

Variant TLFontFamily::_get(String p_name) const {

	String name = p_name;

	PoolStringArray tokens = name.split("/");
	if ((tokens.size() >= 2) && (styles.count(tokens[0]))) {
		String style = tokens[0];
		if (styles.count(style.to_upper()) > 0) {
			if (tokens.size() == 2) {
				int index = (int)tokens[1].to_int();
				if ((index >= 0) && (index < styles.at(style.to_upper()).main_chain.size())) {
					return styles.at(style.to_upper()).main_chain[index];
				}

			} else if (tokens.size() == 4) {
				int index = (int)tokens[3].to_int();
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

		for (int i = 0; i < it->second.main_chain.size(); i++) {
			Dictionary prop;
			prop["name"] = it->first.to_lower() + "/" + String::num_int64(i);
			prop["type"] = GlobalConstants::TYPE_OBJECT;
			prop["hint"] = GlobalConstants::PROPERTY_HINT_RESOURCE_TYPE;
			prop["hint_string"] = "TLFontFace";
			prop["usage"] = GlobalConstants::PROPERTY_USAGE_NOEDITOR | GlobalConstants::PROPERTY_USAGE_STORAGE;
			ret.push_back(prop);
		}

		for (auto sit = it->second.linked_src_chain.begin(); sit != it->second.linked_src_chain.end(); ++sit) {
			for (int i = 0; i < sit->second.size(); i++) {
				char tag[5] = "";
				hb_tag_to_string(hb_script_to_iso15924_tag(sit->first), tag);

				Dictionary prop;
				prop["name"] = it->first.to_lower() + "/script/" + tag + "/" + String::num_int64(i);
				prop["type"] = GlobalConstants::TYPE_OBJECT;
				prop["hint"] = GlobalConstants::PROPERTY_HINT_RESOURCE_TYPE;
				prop["hint_string"] = "TLFontFace";
				prop["usage"] = GlobalConstants::PROPERTY_USAGE_NOEDITOR | GlobalConstants::PROPERTY_USAGE_STORAGE;
				ret.push_back(prop);
			}
		}
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
		}
	}
	return ret;
}

void TLFontFamily::_register_methods() {

	register_method("remove_style", &TLFontFamily::remove_style);
	register_method("has_style", &TLFontFamily::has_style);

	register_method("add_face", &TLFontFamily::add_face);

	//register_method("set_face", &TLFontFamily::set_face);
	//register_method("get_face", &TLFontFamily::get_face);

	register_method("add_face_for_script", &TLFontFamily::add_face_for_script);
	register_method("add_face_for_language", &TLFontFamily::add_face_for_language);

	//register_method("set_liked_face_for_script", &TLFontFamily::set_liked_face_for_script);
	//register_method("get_liked_face_for_script", &TLFontFamily::get_liked_face_for_script);

	register_method("_get_property_list", &TLFontFamily::_get_property_list);
	register_method("_get", &TLFontFamily::_get);
	register_method("_set", &TLFontFamily::_set);
}

#endif
