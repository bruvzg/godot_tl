/*************************************************************************/
/*  tl_font.cpp                                                          */
/*************************************************************************/

#ifdef GODOT_MODULE
#include "servers/rendering_server.h"
#else
#include <GlobalConstants.hpp>
#include <Image.hpp>
#include <Texture2D.hpp>
#include <RenderingServer.hpp>
#endif

#include "tl_font.hpp"

Ref<ImageTexture> TLFontFace::hex_box_img_tex = NULL;

TLFontFace::TLFontFace() {

#ifdef GODOT_MODULE
	_init();
#endif
}

void TLFontFace::_init() {

	//NOP
}

TLFontFace::~TLFontFace() {

	//NOP
}

void TLFontFace::initialize_hex_font() {

	PackedByteArray hex_box_data;
	hex_box_data.resize(167);
	std::memcpy(hex_box_data.ptrw(), _hex_box_img_data, 167);

	Ref<Image> image;
	image.instance();
	image->load_png_from_buffer(hex_box_data);

	hex_box_img_tex.instance();
	hex_box_img_tex->create_from_image(image);
}

void TLFontFace::finish_hex_font() {

	hex_box_img_tex.unref();
}

void TLFontFace::_draw_small_int(RID p_canvas_item, const Point2 p_pos, uint32_t p_int, const Color p_modulate) {
	//for debug draw (4 dig)
	uint8_t a = p_int % 10000 / 1000;
	uint8_t b = p_int % 1000 / 100;
	uint8_t c = p_int % 100 / 10;
	uint8_t d = p_int % 10;

	Vector2 off = Vector2();
	if (a != 0) {
		Rect2 dest = Rect2(p_pos + off, Size2(5, 7));
		RenderingServer::get_singleton()->canvas_item_add_texture_rect_region(p_canvas_item, dest, hex_box_img_tex->get_rid(), Rect2(Point2(a * 5, 0), dest.size), p_modulate, false, RID(), RID(), Color(1, 1, 1, 1), false);
		off += Point2(6, 0);
	}
	if ((a != 0) || (b != 0)) {
		Rect2 dest = Rect2(p_pos + off, Size2(5, 7));
		RenderingServer::get_singleton()->canvas_item_add_texture_rect_region(p_canvas_item, dest, hex_box_img_tex->get_rid(), Rect2(Point2(b * 5, 0), dest.size), p_modulate, false, RID(), RID(), Color(1, 1, 1, 1), false);
		off += Point2(6, 0);
	}
	if ((a != 0) || (b != 0) || (c != 0)) {
		Rect2 dest = Rect2(p_pos + off, Size2(5, 7));
		RenderingServer::get_singleton()->canvas_item_add_texture_rect_region(p_canvas_item, dest, hex_box_img_tex->get_rid(), Rect2(Point2(c * 5, 0), dest.size), p_modulate, false, RID(), RID(), Color(1, 1, 1, 1), false);
		off += Point2(6, 0);
	}
	Rect2 dest = Rect2(p_pos + off, Size2(5, 7));
	RenderingServer::get_singleton()->canvas_item_add_texture_rect_region(p_canvas_item, dest, hex_box_img_tex->get_rid(), Rect2(Point2(d * 5, 0), dest.size), p_modulate, false, RID(), RID(), Color(1, 1, 1, 1), false);
}

void TLFontFace::draw_hexbox(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate) {

	uint8_t a = p_codepoint & 0x0F;
	uint8_t b = (p_codepoint >> 4) & 0x0F;
	uint8_t c = (p_codepoint >> 8) & 0x0F;
	uint8_t d = (p_codepoint >> 12) & 0x0F;
	uint8_t e = (p_codepoint >> 16) & 0x0F;
	uint8_t f = (p_codepoint >> 20) & 0x0F;

	float w = (p_codepoint <= 0xFF) ? 11 : ((p_codepoint <= 0xFFFF) ? 17 : 23);

	RenderingServer::get_singleton()->canvas_item_add_rect(p_canvas_item, Rect2(p_pos + Point2(1, 0), Size2(1, 20)), p_modulate);
	RenderingServer::get_singleton()->canvas_item_add_rect(p_canvas_item, Rect2(p_pos + Point2(w, 0), Size2(1, 20)), p_modulate);
	RenderingServer::get_singleton()->canvas_item_add_rect(p_canvas_item, Rect2(p_pos + Point2(1, 0), Size2(w, 1)), p_modulate);
	RenderingServer::get_singleton()->canvas_item_add_rect(p_canvas_item, Rect2(p_pos + Point2(1, 20), Size2(w, 1)), p_modulate);

	if (p_codepoint <= 0xFF) {
		Rect2 dest = Rect2(p_pos + Point2(4, 3), Size2(5, 7));
		RenderingServer::get_singleton()->canvas_item_add_texture_rect_region(p_canvas_item, dest, hex_box_img_tex->get_rid(), Rect2(dest.position - p_pos + Point2(b * 5 - 4, -3), dest.size), p_modulate, false, RID(), RID(), Color(1, 1, 1, 1), false);
		dest = Rect2(p_pos + Point2(4, 11), Size2(5, 7));
		RenderingServer::get_singleton()->canvas_item_add_texture_rect_region(p_canvas_item, dest, hex_box_img_tex->get_rid(), Rect2(dest.position - p_pos + Point2(a * 5 - 4, -11), dest.size), p_modulate, false, RID(), RID(), Color(1, 1, 1, 1), false);
	} else if (p_codepoint <= 0xFFFF) {
		Rect2 dest = Rect2(p_pos + Point2(4, 3), Size2(5, 7));
		RenderingServer::get_singleton()->canvas_item_add_texture_rect_region(p_canvas_item, dest, hex_box_img_tex->get_rid(), Rect2(dest.position - p_pos + Point2(d * 5 - 4, -3), dest.size), p_modulate, false, RID(), RID(), Color(1, 1, 1, 1), false);
		dest = Rect2(p_pos + Point2(10, 3), Size2(5, 7));
		RenderingServer::get_singleton()->canvas_item_add_texture_rect_region(p_canvas_item, dest, hex_box_img_tex->get_rid(), Rect2(dest.position - p_pos + Point2(c * 5 - 10, -3), dest.size), p_modulate, false, RID(), RID(), Color(1, 1, 1, 1), false);
		dest = Rect2(p_pos + Point2(4, 11), Size2(5, 7));
		RenderingServer::get_singleton()->canvas_item_add_texture_rect_region(p_canvas_item, dest, hex_box_img_tex->get_rid(), Rect2(dest.position - p_pos + Point2(b * 5 - 4, -11), dest.size), p_modulate, false, RID(), RID(), Color(1, 1, 1, 1), false);
		dest = Rect2(p_pos + Point2(10, 11), Size2(5, 7));
		RenderingServer::get_singleton()->canvas_item_add_texture_rect_region(p_canvas_item, dest, hex_box_img_tex->get_rid(), Rect2(dest.position - p_pos + Point2(a * 5 - 10, -11), dest.size), p_modulate, false, RID(), RID(), Color(1, 1, 1, 1), false);
	} else {
		Rect2 dest = Rect2(p_pos + Point2(4, 3), Size2(5, 7));
		RenderingServer::get_singleton()->canvas_item_add_texture_rect_region(p_canvas_item, dest, hex_box_img_tex->get_rid(), Rect2(dest.position - p_pos + Point2(f * 5 - 4, -3), dest.size), p_modulate, false, RID(), RID(), Color(1, 1, 1, 1), false);
		dest = Rect2(p_pos + Point2(10, 3), Size2(5, 7));
		RenderingServer::get_singleton()->canvas_item_add_texture_rect_region(p_canvas_item, dest, hex_box_img_tex->get_rid(), Rect2(dest.position - p_pos + Point2(e * 5 - 10, -3), dest.size), p_modulate, false, RID(), RID(), Color(1, 1, 1, 1), false);
		dest = Rect2(p_pos + Point2(16, 3), Size2(5, 7));
		RenderingServer::get_singleton()->canvas_item_add_texture_rect_region(p_canvas_item, dest, hex_box_img_tex->get_rid(), Rect2(dest.position - p_pos + Point2(d * 5 - 16, -3), dest.size), p_modulate, false, RID(), RID(), Color(1, 1, 1, 1), false);
		dest = Rect2(p_pos + Point2(4, 11), Size2(5, 7));
		RenderingServer::get_singleton()->canvas_item_add_texture_rect_region(p_canvas_item, dest, hex_box_img_tex->get_rid(), Rect2(dest.position - p_pos + Point2(c * 5 - 4, -11), dest.size), p_modulate, false, RID(), RID(), Color(1, 1, 1, 1), false);
		dest = Rect2(p_pos + Point2(10, 11), Size2(5, 7));
		RenderingServer::get_singleton()->canvas_item_add_texture_rect_region(p_canvas_item, dest, hex_box_img_tex->get_rid(), Rect2(dest.position - p_pos + Point2(b * 5 - 10, -11), dest.size), p_modulate, false, RID(), RID(), Color(1, 1, 1, 1), false);
		dest = Rect2(p_pos + Point2(16, 11), Size2(5, 7));
		RenderingServer::get_singleton()->canvas_item_add_texture_rect_region(p_canvas_item, dest, hex_box_img_tex->get_rid(), Rect2(dest.position - p_pos + Point2(a * 5 - 16, -11), dest.size), p_modulate, false, RID(), RID(), Color(1, 1, 1, 1), false);
	}
}

void TLFontFace::draw_glyph(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate, int p_size) const {

	WARN_PRINT("Not implemented, pure virtual function call!");
}

void TLFontFace::_draw_char(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate, int p_size) const {

	//raw char for debug only, do not use
	WARN_PRINT("Not implemented, pure virtual function call!");
}

void TLFontFace::draw_glyph_outline(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate, int p_size) const {

	WARN_PRINT("Not implemented, pure virtual function call!");
}

Array TLFontFace::get_glyph_outline(const Point2 p_pos, uint32_t p_codepoint, int p_size) const {
	return Array();
}

float TLFontFace::get_glyph_scale(int p_size) const {

	return 1.0f;
}

hb_font_t *TLFontFace::get_hb_font(int p_size) const {

	return NULL;
}

bool TLFontFace::load(String p_resource_path) {

	WARN_PRINT("Not implemented, pure virtual function call!");
	return false;
}

Array TLFontFace::_unicode_scripts_supported() const {

	Array ret;

	std::vector<hb_script_t> _sup = unicode_scripts_supported();
	for (int i = 0; i < _sup.size(); i++) {
		char tag[5] = "";
		hb_tag_to_string(hb_script_to_iso15924_tag(_sup[i]), tag);
		ret.push_back(String(tag).to_upper());
	}

	return ret;
}

std::vector<hb_script_t> TLFontFace::unicode_scripts_supported() const {

	return std::vector<hb_script_t>();
}

double TLFontFace::get_ascent(int p_size) const {

	return 0.0f;
}

double TLFontFace::get_descent(int p_size) const {

	return 0.0f;
}

double TLFontFace::get_height(int p_size) const {

	return 0.0f;
}

int TLFontFace::get_base_size() const {

	return 0;
}

void TLFontFace::set_font_path(String p_resource_path) {

	WARN_PRINT("Not implemented, pure virtual function call!");
	path = p_resource_path;
}

String TLFontFace::get_font_path() const {

	return path;
}

#ifdef GODOT_MODULE
void TLFontFace::_bind_methods() {

	ClassDB::bind_method(D_METHOD("load", "resource_path"), &TLFontFace::load);

	ClassDB::bind_method(D_METHOD("draw_glyph", "canvas_item", "pos", "codepoint", "modulate", "size"), &TLFontFace::draw_glyph);
	ClassDB::bind_method(D_METHOD("draw_glyph_outline", "canvas_item", "pos", "codepoint", "modulate", "size"), &TLFontFace::draw_glyph_outline);
	ClassDB::bind_method(D_METHOD("get_glyph_outline", "pos", "codepoint", "size"), &TLFontFace::get_glyph_outline);

	ClassDB::bind_method(D_METHOD("get_ascent", "size"), &TLFontFace::get_ascent);
	ClassDB::bind_method(D_METHOD("get_descent", "size"), &TLFontFace::get_descent);
	ClassDB::bind_method(D_METHOD("get_height", "size"), &TLFontFace::get_height);

	ClassDB::bind_method(D_METHOD("get_base_size"), &TLFontFace::get_base_size);

	ClassDB::bind_method(D_METHOD("get_font_path"), &TLFontFace::get_font_path);
	ClassDB::bind_method(D_METHOD("set_font_path", "path"), &TLFontFace::set_font_path);

	ClassDB::bind_method(D_METHOD("unicode_scripts_supported"), &TLFontFace::_unicode_scripts_supported);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "font_path", PROPERTY_HINT_FILE, "*.ttf,*.otf"), "set_font_path", "get_font_path");
}

#else

void TLFontFace::_register_methods() {

	register_method("load", &TLFontFace::load);

	register_method("draw_glyph", &TLFontFace::draw_glyph);
	register_method("draw_glyph_outline", &TLFontFace::draw_glyph_outline);
	register_method("get_glyph_outline", &TLFontFace::get_glyph_outline);

	register_method("get_ascent", &TLFontFace::get_ascent);
	register_method("get_descent", &TLFontFace::get_descent);
	register_method("get_height", &TLFontFace::get_height);

	register_method("get_base_size", &TLFontFace::get_base_size);

	register_method("get_font_path", &TLFontFace::get_font_path);
	register_method("set_font_path", &TLFontFace::set_font_path);

	register_method("set_texture_flags", &TLFontFace::set_texture_flags);
	register_method("get_texture_flags", &TLFontFace::get_texture_flags);

	register_method("unicode_scripts_supported", &TLFontFace::_unicode_scripts_supported);

	register_property<TLFontFace, String>("font_path", &TLFontFace::set_font_path, &TLFontFace::get_font_path, String(), GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_FILE, String("*.ttf,*.otf"));
	register_property<TLFontFace, int>("texture_flags", &TLFontFace::set_texture_flags, &TLFontFace::get_texture_flags, Texture2D::FLAG_VIDEO_SURFACE, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_FLAGS, String("Mipmaps,Repeat,Filter,Anisotropic Linear,Convert to Linear,Mirrored Repeat,Video Surface"));
}
#endif
