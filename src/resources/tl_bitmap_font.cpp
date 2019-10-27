/*************************************************************************/
/*  tl_bitmap_font.cpp                                                   */
/*************************************************************************/

/*
This file is based in part on Godot's BitmapFont implementation, licensed under MIT license.
For more information, see https://github.com/godotengine/godot/blob/master/LICENSE.txt
For original source, see https://github.com/godotengine/godot/blob/master/scene/resources/font.cpp
*/

#include "tl_bitmap_font.hpp"

#ifdef GODOT_MODULE
#include "core/bind/core_bind.h"
#include "servers/visual_server.h"
#define File _File
#else
#include <File.hpp>
#include <ResourceLoader.hpp>
#include <Texture.hpp>
#include <VisualServer.hpp>
#endif

/*************************************************************************/
/*  hb_bmp_font_t HarfBuzz Bitmap font interface                         */
/*************************************************************************/

struct hb_bmp_font_t {
	TLBitmapFontFaceAtSize *bm_face;
	bool unref; /* Whether to destroy bm_face when done. */
};

static hb_bmp_font_t *_hb_bmp_font_create(TLBitmapFontFaceAtSize *bm_face, bool unref) {
	hb_bmp_font_t *bm_font = reinterpret_cast<hb_bmp_font_t *>(calloc(1, sizeof(hb_bmp_font_t)));

	if (!bm_font)
		return nullptr;

	bm_font->bm_face = bm_face;
	bm_font->unref = unref;

	return bm_font;
}

static void _hb_bmp_font_destroy(void *data) {
	hb_bmp_font_t *bm_font = reinterpret_cast<hb_bmp_font_t *>(data);
	free(bm_font);
}

static hb_bool_t hb_bmp_get_nominal_glyph(hb_font_t *font, void *font_data, hb_codepoint_t unicode, hb_codepoint_t *glyph, void *user_data) {
	const hb_bmp_font_t *bm_font = reinterpret_cast<const hb_bmp_font_t *>(font_data);

	if (!bm_font->bm_face)
		return false;

	if (!bm_font->bm_face->font->has_glyph(unicode)) {
		if (bm_font->bm_face->font->has_glyph(0xF000u + unicode)) {
			*glyph = 0xF000u + unicode;
			return true;
		} else {
			return false;
		}
	}

	*glyph = unicode;
	return true;
}

static hb_position_t hb_bmp_get_glyph_h_advance(hb_font_t *font, void *font_data, hb_codepoint_t glyph, void *user_data) {
	const hb_bmp_font_t *bm_font = reinterpret_cast<const hb_bmp_font_t *>(font_data);

	if (!bm_font->bm_face)
		return 0;

	if (!bm_font->bm_face->font->has_glyph(glyph))
		return 0;

	return bm_font->bm_face->font->get_glyph_advance(glyph, bm_font->bm_face->size) * 64;
}

static hb_position_t hb_bmp_get_glyph_h_kerning(hb_font_t *font, void *font_data, hb_codepoint_t left_glyph, hb_codepoint_t right_glyph, void *user_data) {
	const hb_bmp_font_t *bm_font = reinterpret_cast<const hb_bmp_font_t *>(font_data);

	if (!bm_font->bm_face)
		return 0;

	if (!bm_font->bm_face->font->has_glyph(left_glyph))
		return 0;

	if (!bm_font->bm_face->font->has_glyph(right_glyph))
		return 0;

	return bm_font->bm_face->font->get_kerning_pair(left_glyph, right_glyph, bm_font->bm_face->size) * 64;
}

static hb_bool_t hb_bmp_get_glyph_v_origin(hb_font_t *font, void *font_data, hb_codepoint_t glyph, hb_position_t *x, hb_position_t *y, void *user_data) {
	const hb_bmp_font_t *bm_font = reinterpret_cast<const hb_bmp_font_t *>(font_data);

	if (!bm_font->bm_face)
		return false;

	if (!bm_font->bm_face->font->has_glyph(glyph))
		return false;

	*x = bm_font->bm_face->font->get_glyph_align(glyph, bm_font->bm_face->size).x * 64;
	*y = bm_font->bm_face->font->get_glyph_align(glyph, bm_font->bm_face->size).y * 64;

	return true;
}

static hb_bool_t hb_bmp_get_glyph_extents(hb_font_t *font, void *font_data, hb_codepoint_t glyph, hb_glyph_extents_t *extents, void *user_data) {
	const hb_bmp_font_t *bm_font = reinterpret_cast<const hb_bmp_font_t *>(font_data);

	if (!bm_font->bm_face)
		return false;

	if (!bm_font->bm_face->font->has_glyph(glyph))
		return false;

	extents->x_bearing = 0;
	extents->y_bearing = 0;
	extents->width = bm_font->bm_face->font->get_glyph_size(glyph, bm_font->bm_face->size).x * 64;
	extents->height = bm_font->bm_face->font->get_glyph_size(glyph, bm_font->bm_face->size).y * 64;

	return true;
}

static hb_bool_t hb_bmp_get_font_h_extents(hb_font_t *font, void *font_data, hb_font_extents_t *metrics, void *user_data) {
	const hb_bmp_font_t *bm_font = reinterpret_cast<const hb_bmp_font_t *>(font_data);

	if (!bm_font->bm_face)
		return false;

	metrics->ascender = bm_font->bm_face->font->get_ascent(bm_font->bm_face->size);
	metrics->descender = bm_font->bm_face->font->get_descent(bm_font->bm_face->size);
	metrics->line_gap = 0;

	return true;
}

static hb_font_funcs_t *_hb_bmp_get_font_funcs(void) {

	hb_font_funcs_t *funcs = hb_font_funcs_create();

	hb_font_funcs_set_font_h_extents_func(funcs, hb_bmp_get_font_h_extents, nullptr, nullptr);
	//hb_font_funcs_set_font_v_extents_func (funcs, hb_bmp_get_font_v_extents, nullptr, nullptr);
	hb_font_funcs_set_nominal_glyph_func(funcs, hb_bmp_get_nominal_glyph, nullptr, nullptr);
	//hb_font_funcs_set_variation_glyph_func (funcs, hb_bmp_get_variation_glyph, nullptr, nullptr);
	hb_font_funcs_set_glyph_h_advance_func(funcs, hb_bmp_get_glyph_h_advance, nullptr, nullptr);
	//hb_font_funcs_set_glyph_v_advance_func (funcs, hb_bmp_get_glyph_v_advance, nullptr, nullptr);
	//hb_font_funcs_set_glyph_h_origin_func (funcs, hb_bmp_get_glyph_h_origin, nullptr, nullptr);
	hb_font_funcs_set_glyph_v_origin_func(funcs, hb_bmp_get_glyph_v_origin, nullptr, nullptr);
	hb_font_funcs_set_glyph_h_kerning_func(funcs, hb_bmp_get_glyph_h_kerning, nullptr, nullptr);
	//hb_font_funcs_set_glyph_v_kerning_func (funcs, hb_bmp_get_glyph_v_kerning, nullptr, nullptr);
	hb_font_funcs_set_glyph_extents_func(funcs, hb_bmp_get_glyph_extents, nullptr, nullptr);
	//hb_font_funcs_set_glyph_contour_point_func (funcs, hb_bmp_get_glyph_contour_point, nullptr, nullptr);
	//hb_font_funcs_set_glyph_name_func (funcs, hb_bmp_get_glyph_name, nullptr, nullptr);
	//hb_font_funcs_set_glyph_from_name_func (funcs, hb_bmp_get_glyph_from_name, nullptr, nullptr);

	hb_font_funcs_make_immutable(funcs);

	return funcs;
}

static void _hb_bmp_font_set_funcs(hb_font_t *font, TLBitmapFontFaceAtSize *bm_face, bool unref) {
	hb_font_set_funcs(font, _hb_bmp_get_font_funcs(), _hb_bmp_font_create(bm_face, unref), _hb_bmp_font_destroy);
}

hb_font_t *hb_bmp_font_create(TLBitmapFontFaceAtSize *bm_face, hb_destroy_func_t destroy) {
	hb_font_t *font;
	hb_face_t *face = hb_face_create(NULL, 0);

	font = hb_font_create(face);
	hb_face_destroy(face);
	_hb_bmp_font_set_funcs(font, bm_face, false);
	return font;
}

/*************************************************************************/
/*  TLBitmapFontFaceAtSize                                               */
/*************************************************************************/

TLBitmapFontFaceAtSize::TLBitmapFontFaceAtSize() {

	hb_font = NULL;
	font = NULL;
	size = 0;
}

TLBitmapFontFaceAtSize::~TLBitmapFontFaceAtSize() {

	if (hb_font) {
		hb_font_destroy(hb_font);
		hb_font = NULL;
	}
}

/*************************************************************************/
/*  TLBitmapFontFace                                                     */
/*************************************************************************/

TLBitmapFontFace::TLBitmapFontFace() {

#ifdef GODOT_MODULE
	_init();
#endif
}

void TLBitmapFontFace::_init() {

	txt_flags = Texture::FLAG_VIDEO_SURFACE;
	bmp_size = 0;
	ascent = 0.0f;
	descent = 0.0f;
	height = 0.0f;
	loaded = false;
}

TLBitmapFontFace::~TLBitmapFontFace() {

	clear_cache();
	loaded = false;
}

float TLBitmapFontFace::get_glyph_advance(uint32_t p_codepoint, int p_size) const {

	if (!loaded)
		const_cast<TLBitmapFontFace *>(this)->load(path);
	float scale = (float)p_size / bmp_size;
	if (loaded && (glyph_cache.count(p_codepoint) > 0)) {
		return glyph_cache.at(p_codepoint).advance * scale;
	} else {
		return 0.0f;
	}
}

Point2 TLBitmapFontFace::get_glyph_align(uint32_t p_codepoint, int p_size) const {

	if (!loaded)
		const_cast<TLBitmapFontFace *>(this)->load(path);
	float scale = (float)p_size / bmp_size;
	if (loaded && (glyph_cache.count(p_codepoint) > 0)) {
		return glyph_cache.at(p_codepoint).align * scale;
	} else {
		return Point2();
	}
}

Size2 TLBitmapFontFace::get_glyph_size(uint32_t p_codepoint, int p_size) const {

	if (!loaded)
		const_cast<TLBitmapFontFace *>(this)->load(path);
	float scale = (float)p_size / bmp_size;
	if (loaded && (glyph_cache.count(p_codepoint) > 0)) {
		return glyph_cache.at(p_codepoint).uv.size * scale;
	} else {
		return Size2();
	}
}

float TLBitmapFontFace::get_kerning_pair(uint32_t p_codepoint_l, uint32_t p_codepoint_r, int p_size) const {

	if (!loaded)
		const_cast<TLBitmapFontFace *>(this)->load(path);

	float scale = (float)p_size / bmp_size;
	if (loaded) {
		KerningPairKey kpk;
		kpk.A = p_codepoint_l;
		kpk.B = p_codepoint_r;

		auto search = kerning_map.find(kpk);
		if (search != kerning_map.end()) {
			return search->second * scale;
		}
	}
	return 0.0f;
}

bool TLBitmapFontFace::has_glyph(uint32_t p_codepoint) const {

	if (!loaded)
		const_cast<TLBitmapFontFace *>(this)->load(path);

	return (loaded && (glyph_cache.count(p_codepoint) > 0));
}

void TLBitmapFontFace::_draw_char(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate, int p_size) const {

	draw_glyph(p_canvas_item, p_pos, p_codepoint, p_modulate, p_size);
}

void TLBitmapFontFace::draw_glyph(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate, int p_size) const {

	if (!loaded)
		const_cast<TLBitmapFontFace *>(this)->load(path);

	float scale = (float)p_size / bmp_size;
	if (loaded && (glyph_cache.count(p_codepoint) > 0)) {

		const Glyph &gl = glyph_cache.at(p_codepoint);

		ERR_FAIL_COND(gl.id >= texture_cache.size());

		VisualServer::get_singleton()->canvas_item_add_texture_rect_region(p_canvas_item, Rect2(p_pos + gl.align * scale, gl.uv.size * scale), texture_cache[gl.id]->get_rid(), gl.uv, p_modulate, false, RID(), false);
	} else {
		WARN_PRINTS("Font not loaded or glyph not found!")
	}
}

hb_font_t *TLBitmapFontFace::get_hb_font(int p_size) const {

	if (!loaded)
		const_cast<TLBitmapFontFace *>(this)->load(path);

	if (sizes.count(p_size) > 0) {
		return sizes.at(p_size)->hb_font;
	} else {
		TLBitmapFontFaceAtSize *f_at_s = new TLBitmapFontFaceAtSize();

		f_at_s->font = this;
		f_at_s->size = p_size;
		f_at_s->hb_font = hb_bmp_font_create(f_at_s, NULL);
		if (f_at_s->hb_font) {
			sizes[p_size] = f_at_s;
			return f_at_s->hb_font;
		}
	}
	return NULL;
}

void TLBitmapFontFace::clear_cache() {

	texture_cache.clear();
	glyph_cache.clear();
	kerning_map.clear();
	for (auto it = sizes.begin(); it != sizes.end(); ++it) {
		delete it->second;
	}
	sizes.clear();
}

void TLBitmapFontFace::set_font_path(String p_resource_path) {

	if (path != p_resource_path) {
		load(p_resource_path);
	}
}

bool TLBitmapFontFace::load(String p_resource_path) {

	path = p_resource_path;
	if (loaded) {
		//unload existing
		clear_cache();

		loaded = false;
	}

	Ref<File> file;
	file.instance();
	if (file->open(p_resource_path, File::READ) != Error::OK) {
		ERR_PRINTS("Can't open bitmap font file: \"" + p_resource_path + "\"");
		emit_signal(_CHANGED);
		return false;
	}

	while (true) {

		String line = file->get_line();

		int delimiter = line.find(" ", 0);
		String type = line.substr(0, delimiter);
		int pos = delimiter + 1;
		std::map<String, String> keys;

		while (pos < line.length() && line[pos] == ' ')
			pos++;

		while (pos < line.length()) {

			int eq = line.find("=", pos);
			if (eq == -1)
				break;
			String key = line.substr(pos, eq - pos);
			int end;
			String value;
			if (line[eq + 1] == '"') {
				end = line.find("\"", eq + 2);
				if (end == -1)
					break;
				value = line.substr(eq + 2, end - 1 - eq - 1);
				pos = end + 1;
			} else {
				end = line.find(" ", eq + 1);
				if (end == -1)
					end = line.length();

				value = line.substr(eq + 1, end - eq);

				pos = end;
			}

			while (pos < line.length() && line[pos] == ' ')
				pos++;

			keys[key] = value;
		}

		if (type == "info") {
			//if (keys.count("face") > 0)
			//	font_name = keys["face"];
			if (keys.count("size") > 0) {
				bmp_size = keys["size"].to_int();
			}
		} else if (type == "common") {
			if (keys.count("lineHeight") > 0)
				height = keys["lineHeight"].to_int();
			if (keys.count("base") > 0)
				ascent = keys["base"].to_int();
		} else if (type == "page") {
			if (keys.count("file") > 0) {
				String base_dir = p_resource_path.get_base_dir();
				String file_name = base_dir.plus_file(keys["file"]);
#ifdef GODOT_MODULE
				Ref<Texture> tex = ResourceLoader::load(file_name);
#else
				Ref<Texture> tex = cast_to<Texture>(*ResourceLoader::get_singleton()->load(file_name));
#endif
				if (tex.is_null()) {
					ERR_PRINTS("Can't load bitmap font texture: \"" + file_name + "\"");
					clear_cache();
					file->close();
					emit_signal(_CHANGED);
					return false;
				} else {
					tex->set_flags(txt_flags);
					texture_cache.push_back(tex);
				}
			}
		} else if (type == "char") {
			uint32_t idx = 0;
			if (keys.count("id") > 0)
				idx = keys["id"].to_int();

			Rect2 rect;
			if (keys.count("x") > 0)
				rect.position.x = keys["x"].to_int();
			if (keys.count("y"))
				rect.position.y = keys["y"].to_int();
			if (keys.count("width") > 0)
				rect.size.width = keys["width"].to_int();
			if (keys.count("height") > 0)
				rect.size.height = keys["height"].to_int();

			Point2 ofs;
			if (keys.count("xoffset") > 0)
				ofs.x = keys["xoffset"].to_int();
			if (keys.count("yoffset") > 0)
				ofs.y = keys["yoffset"].to_int();

			int texture = 0;
			if (keys.count("page") > 0)
				texture = keys["page"].to_int();
			int advance = -1;
			if (keys.count("xadvance") > 0)
				advance = keys["xadvance"].to_int();
			if (advance < 0)
				advance = rect.size.width;

			Glyph gl;
			gl.uv = rect;
			gl.id = texture;
			gl.align = ofs;
			gl.advance = advance;

			glyph_cache[idx] = gl;
		} else if (type == "kerning") {
			uint32_t first = 0, second = 0;
			int k = 0;

			if (keys.count("first") > 0)
				first = keys["first"].to_int();
			if (keys.count("second") > 0)
				second = keys["second"].to_int();
			if (keys.count("amount") > 0)
				k = keys["amount"].to_int();

			KerningPairKey kpk;
			kpk.A = first;
			kpk.B = second;

			if ((k == 0) && (kerning_map.count(kpk) > 0)) {
				kerning_map.erase(kpk);
			} else {
				kerning_map[kpk] = -k;
			}
		}

		if (file->eof_reached())
			break;
	}

	file->close();
	loaded = true;
	emit_signal(_CHANGED);
	return loaded;
}

std::vector<hb_script_t> TLBitmapFontFace::unicode_scripts_supported() const {

	std::vector<hb_script_t> ret;
	//TODO: detect supported scripts!
	ret.push_back(HB_SCRIPT_COMMON);
	return ret;
}

double TLBitmapFontFace::get_ascent(int p_size) const {

	float scale = (float)p_size / bmp_size;
	if (!loaded)
		const_cast<TLBitmapFontFace *>(this)->load(path);
	return loaded ? ascent * scale : 0.0f;
}

double TLBitmapFontFace::get_descent(int p_size) const {

	float scale = (float)p_size / bmp_size;
	if (!loaded)
		const_cast<TLBitmapFontFace *>(this)->load(path);
	return loaded ? descent * scale : 0.0f;
}

double TLBitmapFontFace::get_height(int p_size) const {

	float scale = (float)p_size / bmp_size;
	if (!loaded)
		const_cast<TLBitmapFontFace *>(this)->load(path);
	return loaded ? height * scale : 0.0f;
}

int TLBitmapFontFace::get_base_size() const {

	if (!loaded)
		const_cast<TLBitmapFontFace *>(this)->load(path);
	return loaded ? bmp_size : 0;
}

void TLBitmapFontFace::set_texture_flags(int p_flags) {

	if (txt_flags != p_flags) {
		txt_flags = p_flags;
		if (!loaded)
			load(path);
		if (loaded) {
			for (size_t i = 0; i < texture_cache.size(); i++) {
				Ref<Texture> &tex = texture_cache[i];
				if (!tex.is_null())
					tex->set_flags(txt_flags);
			}
			emit_signal(_CHANGED);
		}
	}
}

int TLBitmapFontFace::get_texture_flags() const {

	return txt_flags;
}
