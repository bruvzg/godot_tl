/*************************************************************************/
/*  tl_font_family.cpp                                                   */
/*************************************************************************/

#include "resources/tl_font_family.hpp"

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

/* Predefined Style:
	Regular
	Italic
	Bold
	BoldItalic
*/

static const char *_scripts[] = {
	"Arab",
	"Armn",
	"Beng",
	"Cyrl",
	"Deva",
	"Geor",
	"Grek",
	"Gujr",
	"Guru",
	"Hang",
	"Hani",
	"Hebr",
	"Hira",
	"Knda",
	"Kana",
	"Laoo",
	"Latn",
	"Mlym",
	"Orya",
	"Taml",
	"Telu",
	"Thai",
	"Tibt",
	"Bopo",
	"Brai",
	"Cans",
	"Cher",
	"Ethi",
	"Khmr",
	"Mong",
	"Mymr",
	"Ogam",
	"Runr",
	"Sinh",
	"Syrc",
	"Thaa",
	"Yiii",
	"Dsrt",
	"Goth",
	"Ital",
	"Buhd",
	"Hano",
	"Tglg",
	"Tagb",
	"Cprt",
	"Limb",
	"Linb",
	"Osma",
	"Shaw",
	"Tale",
	"Ugar",
	"Bugi",
	"Copt",
	"Glag",
	"Khar",
	"Talu",
	"Xpeo",
	"Sylo",
	"Tfng",
	"Bali",
	"Xsux",
	"Nkoo",
	"Phag",
	"Phnx",
	"Cari",
	"Cham",
	"Kali",
	"Lepc",
	"Lyci",
	"Lydi",
	"Olck",
	"Rjng",
	"Saur",
	"Sund",
	"Vaii",
	"Avst",
	"Bamu",
	"Egyp",
	"Armi",
	"Phli",
	"Prti",
	"Java",
	"Kthi",
	"Lisu",
	"Mtei",
	"Sarb",
	"Orkh",
	"Samr",
	"Lana",
	"Tavt",
	"Batk",
	"Brah",
	"Mand",
	"Cakm",
	"Merc",
	"Mero",
	"Plrd",
	"Shrd",
	"Sora",
	"Takr",
	"Bass",
	"Aghb",
	"Dupl",
	"Elba",
	"Gran",
	"Khoj",
	"Sind",
	"Lina",
	"Mahj",
	"Mani",
	"Mend",
	"Modi",
	"Mroo",
	"Nbat",
	"Narb",
	"Perm",
	"Hmng",
	"Palm",
	"Pauc",
	"Phlp",
	"Sidd",
	"Tirh",
	"Wara",
	"Ahom",
	"Hluw",
	"Hatr",
	"Mult",
	"Hung",
	"Sgnw",
	"Adlm",
	"Bhks",
	"Marc",
	"Osge",
	"Tang",
	"Newa",
	"Gonm",
	"Nshu",
	"Soyo",
	"Zanb",
	"Dogr",
	"Gong",
	"Rohg",
	"Maka",
	"Medf",
	"Sogo",
	"Sogd",
	"Elym",
	"Nand",
	"Hmnp",
	"Wcho",
	NULL
};

#ifdef GODOT_MODULE

bool TLFontFamily::_set(const StringName &p_name, const Variant &p_value) {

	String str = p_name;
	String sty = str.get_slicec('/', 0);
	if ((sty == String("regular")) || (sty == String("bold")) || (sty == String("bolditalic")) || (sty == String("italic"))) {
		String ty = str.get_slicec('/', 1);
		if (ty == String("base")) {
			set_face(sty, p_value);
			return true;
		} else if (ty == String("scripts")) {
			String scr = str.get_slicec('/', 2);
			if (scr != String()) {
				set_liked_face_for_script(sty, scr, p_value);
				return true;
			}
		}
	}

	return false;
}

bool TLFontFamily::_get(const StringName &p_name, Variant &r_ret) const {

	String str = p_name;
	String sty = str.get_slicec('/', 0);
	if ((sty == String("regular")) || (sty == String("bold")) || (sty == String("bolditalic")) || (sty == String("italic"))) {
		String ty = str.get_slicec('/', 1);
		if (ty == String("base")) {
			r_ret = get_face(sty);
			return true;
		} else if (ty == String("scripts")) {
			String scr = str.get_slicec('/', 2);
			if (scr != String()) {
				r_ret = get_liked_face_for_script(sty, scr);
				return true;
			}
		}
	}

	return false;
}
void TLFontFamily::_get_property_list(List<PropertyInfo> *p_list) const {

	p_list->push_back(PropertyInfo(Variant::OBJECT, "regular/base", PROPERTY_HINT_RESOURCE_TYPE, "TLFontFace"));
	p_list->push_back(PropertyInfo(Variant::OBJECT, "bold/base", PROPERTY_HINT_RESOURCE_TYPE, "TLFontFace"));
	p_list->push_back(PropertyInfo(Variant::OBJECT, "bolditalic/base", PROPERTY_HINT_RESOURCE_TYPE, "TLFontFace"));
	p_list->push_back(PropertyInfo(Variant::OBJECT, "italic/base", PROPERTY_HINT_RESOURCE_TYPE, "TLFontFace"));

	for (int i = 0; _scripts[i] != NULL; i++) {
		p_list->push_back(PropertyInfo(Variant::OBJECT, "regular/scripts/" + String(_scripts[i]), PROPERTY_HINT_RESOURCE_TYPE, "TLFontFace"));
		p_list->push_back(PropertyInfo(Variant::OBJECT, "bold/scripts/" + String(_scripts[i]), PROPERTY_HINT_RESOURCE_TYPE, "TLFontFace"));
		p_list->push_back(PropertyInfo(Variant::OBJECT, "bolditalic/scripts/" + String(_scripts[i]), PROPERTY_HINT_RESOURCE_TYPE, "TLFontFace"));
		p_list->push_back(PropertyInfo(Variant::OBJECT, "italic/scripts/" + String(_scripts[i]), PROPERTY_HINT_RESOURCE_TYPE, "TLFontFace"));
	}
}

void TLFontFamily::_bind_methods() {

	ClassDB::bind_method(D_METHOD("remove_style", "style"), &TLFontFamily::remove_style);
	ClassDB::bind_method(D_METHOD("has_style", "style"), &TLFontFamily::has_style);

	ClassDB::bind_method(D_METHOD("set_face", "style", "ref"), &TLFontFamily::set_face);

	ClassDB::bind_method(D_METHOD("get_face", "style"), &TLFontFamily::get_face);

	ClassDB::bind_method(D_METHOD("set_liked_face_for_script", "style", "script", "ref"), &TLFontFamily::set_liked_face_for_script);
	ClassDB::bind_method(D_METHOD("get_liked_face_for_script", "style", "script"), &TLFontFamily::get_liked_face_for_script);
}

#else

bool TLFontFamily::_set(String p_name, Variant p_value) {

	String str = p_name;
	String sty = str.split('/')[0];
	if ((sty == String("regular")) || (sty == String("bold")) || (sty == String("bolditalic")) || (sty == String("italic"))) {
		String ty = str.split('/')[1];
		if (ty == String("base")) {
			set_face(sty, p_value);
			return true;
		} else if (ty == String("scripts")) {
			String scr = str.split('/')[2];
			if (scr != String()) {
				set_liked_face_for_script(sty, scr, p_value);
				return true;
			}
		}
	}

	return false;
}

Variant TLFontFamily::_get(String p_name) {

	String str = p_name;
	String sty = str.split('/')[0];
	if ((sty == String("regular")) || (sty == String("bold")) || (sty == String("bolditalic")) || (sty == String("italic"))) {
		String ty = str.split('/')[1];
		if (ty == String("base")) {
			return get_face(sty);
		} else if (ty == String("scripts")) {
			String scr = str.split('/')[2];
			if (scr != String()) {
				return get_liked_face_for_script(sty, scr);
			}
		}
	}

	return Variant();
}

Array TLFontFamily::_get_property_list() {

	Array ret;

	{
		Dictionary prop;
		prop["name"] = String("regular/base");
		prop["type"] = GlobalConstants::TYPE_OBJECT;
		prop["hint"] = GlobalConstants::PROPERTY_HINT_RESOURCE_TYPE;
		prop["hint_string"] = "TLFontFace";
		prop["usage"] = GlobalConstants::PROPERTY_USAGE_NOEDITOR;
		ret.push_back(prop);
	}
	{
		Dictionary prop;
		prop["name"] = String("bold/base");
		prop["type"] = GlobalConstants::TYPE_OBJECT;
		prop["hint"] = GlobalConstants::PROPERTY_HINT_RESOURCE_TYPE;
		prop["hint_string"] = "TLFontFace";
		prop["usage"] = GlobalConstants::PROPERTY_USAGE_NOEDITOR;
		ret.push_back(prop);
	}
	{
		Dictionary prop;
		prop["name"] = String("bolditalic/base");
		prop["type"] = GlobalConstants::TYPE_OBJECT;
		prop["hint"] = GlobalConstants::PROPERTY_HINT_RESOURCE_TYPE;
		prop["hint_string"] = "TLFontFace";
		prop["usage"] = GlobalConstants::PROPERTY_USAGE_NOEDITOR;
		ret.push_back(prop);
	}
	{
		Dictionary prop;
		prop["name"] = String("italic/base");
		prop["type"] = GlobalConstants::TYPE_OBJECT;
		prop["hint"] = GlobalConstants::PROPERTY_HINT_RESOURCE_TYPE;
		prop["hint_string"] = "TLFontFace";
		prop["usage"] = GlobalConstants::PROPERTY_USAGE_NOEDITOR;
		ret.push_back(prop);
	}

	for (int i = 0; _scripts[i] != NULL; i++) {

		{
			Dictionary prop;
			prop["name"] = "regular/scripts/" + String(_scripts[i]);
			prop["type"] = GlobalConstants::TYPE_OBJECT;
			prop["hint"] = GlobalConstants::PROPERTY_HINT_RESOURCE_TYPE;
			prop["hint_string"] = "TLFontFace";
			prop["usage"] = GlobalConstants::PROPERTY_USAGE_NOEDITOR;
			ret.push_back(prop);
		}
		{
			Dictionary prop;
			prop["name"] = "bold/scripts/" + String(_scripts[i]);
			prop["type"] = GlobalConstants::TYPE_OBJECT;
			prop["hint"] = GlobalConstants::PROPERTY_HINT_RESOURCE_TYPE;
			prop["hint_string"] = "TLFontFace";
			prop["usage"] = GlobalConstants::PROPERTY_USAGE_NOEDITOR;
			ret.push_back(prop);
		}
		{
			Dictionary prop;
			prop["name"] = "bolditalic/scripts/" + String(_scripts[i]);
			prop["type"] = GlobalConstants::TYPE_OBJECT;
			prop["hint"] = GlobalConstants::PROPERTY_HINT_RESOURCE_TYPE;
			prop["hint_string"] = "TLFontFace";
			prop["usage"] = GlobalConstants::PROPERTY_USAGE_NOEDITOR;
			ret.push_back(prop);
		}
		{
			Dictionary prop;
			prop["name"] = "italic/scripts/" + String(_scripts[i]);
			prop["type"] = GlobalConstants::TYPE_OBJECT;
			prop["hint"] = GlobalConstants::PROPERTY_HINT_RESOURCE_TYPE;
			prop["hint_string"] = "TLFontFace";
			prop["usage"] = GlobalConstants::PROPERTY_USAGE_NOEDITOR;
			ret.push_back(prop);
		}
	}

	return ret;
}

void TLFontFamily::_register_methods() {

	register_method("remove_style", &TLFontFamily::remove_style);
	register_method("has_style", &TLFontFamily::has_style);

	register_method("set_face", &TLFontFamily::set_face);

	register_method("get_face", &TLFontFamily::get_face);

	register_method("set_liked_face_for_script", &TLFontFamily::set_liked_face_for_script);
	register_method("get_liked_face_for_script", &TLFontFamily::get_liked_face_for_script);

	register_method("_get_property_list", &TLFontFamily::_get_property_list);
	register_method("_get", &TLFontFamily::_get);
	register_method("_set", &TLFontFamily::_set);
}

#endif
