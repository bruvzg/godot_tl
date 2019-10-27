/*************************************************************************/
/*  tl_dynamic_font.cpp                                                  */
/*************************************************************************/

/*
This file is based in part on Godot's DynamicFont implementation, licensed under MIT license.
For more information, see https://github.com/godotengine/godot/blob/master/LICENSE.txt
For original source, see https://github.com/godotengine/godot/blob/master/scene/resources/dynamic_font.cpp
*/

#ifdef GODOT_MODULE
#include "core/bind/core_bind.h"
#include "servers/visual_server.h"
#define File _File
#else
#include <File.hpp>
#include <GlobalConstants.hpp>
#include <Texture.hpp>
#include <VisualServer.hpp>
#endif

#include "tl_dynamic_font.hpp"
#include FT_STROKER_H

_ALWAYS_INLINE_ unsigned int TLDynamicFontFaceAtSize::_next_power_of_2(unsigned int x) {

	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;

	return ++x;
}

unsigned long TLDynamicFontFaceAtSize::ft_stream_io(FT_Stream p_stream, unsigned long p_offset, unsigned char *p_buffer, unsigned long p_count) {

	File *file = reinterpret_cast<File *>(p_stream->descriptor.pointer);

	if ((uint64_t)file->get_position() != p_offset) {
		file->seek(p_offset);
	}

	if (p_count == 0)
		return 0;

	PoolByteArray buffer = file->get_buffer(p_count);
	memcpy(p_buffer, buffer.read().ptr(), buffer.size());

	return buffer.size();
}

void TLDynamicFontFaceAtSize::ft_stream_close(FT_Stream p_stream) {

	File *file = reinterpret_cast<File *>(p_stream->descriptor.pointer);

	file->close();
	memdelete(file);
}

TLDynamicFontFaceAtSize::TLDynamicFontFaceAtSize() {

	hinting = DF_HINTING_NORMAL;
	force_autohinter = false;
	txt_flags = Texture::FLAG_VIDEO_SURFACE;
	scale_color_font = 1.0f;
	oversampling = 1.0f;

	loaded = false;
	ft_size = 0;
	ascent = 0.0f;
	descent = 0.0f;
	height = 0.0f;
	hb_font = NULL;
	os2 = NULL;
}

TLDynamicFontFaceAtSize::~TLDynamicFontFaceAtSize() {

	clear_cache();
	if (hb_font) {
		hb_font_destroy(hb_font);
		hb_font = NULL;
	}
	if (loaded) {
		FT_Done_FreeType(ft_library);
	}
	loaded = false;
}

void TLDynamicFontFaceAtSize::clear_cache() {

	texture_cache.clear();
	glyph_cache.clear();
	glyph_outline_cache.clear();
}

float TLDynamicFontFaceAtSize::get_glyph_scale() const {

	return scale_color_font / oversampling;
}

void TLDynamicFontFaceAtSize::draw_glyph(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate) const {

	if (loaded) {
		if (glyph_cache.count(p_codepoint) == 0)
			const_cast<TLDynamicFontFaceAtSize *>(this)->update_glyph(p_codepoint);

		if (glyph_cache.count(p_codepoint) == 0)
			return;

		const Glyph &gl = glyph_cache.at(p_codepoint);
		if (gl.valid) {

			Color modulate = p_modulate;
			if (FT_HAS_COLOR(ft_face)) {
				modulate.r = modulate.g = modulate.b = 1.0;
			}

			VisualServer::get_singleton()->canvas_item_add_texture_rect_region(p_canvas_item, Rect2(gl.align + p_pos, gl.size * Vector2(scale_color_font, scale_color_font)), texture_cache[gl.id].image->get_rid(), gl.uv, modulate, false, RID(), false);
		}
	}
}

void TLDynamicFontFaceAtSize::_draw_char(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate) const {

	//raw char for debug only, do not use
	if (loaded) {
		draw_glyph(p_canvas_item, p_pos, FT_Get_Char_Index(ft_face, p_codepoint), p_modulate);
	}
}

void TLDynamicFontFaceAtSize::draw_glyph_outline(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate) const {

	if (loaded) {
		if (glyph_outline_cache.count(p_codepoint) == 0)
			const_cast<TLDynamicFontFaceAtSize *>(this)->update_outline_glyph(p_codepoint);

		if (glyph_outline_cache.count(p_codepoint) == 0)
			return;

		const Glyph &gl = glyph_outline_cache.at(p_codepoint);
		if (gl.valid) {

			Color modulate = p_modulate;
			if (FT_HAS_COLOR(ft_face)) {
				modulate.r = modulate.g = modulate.b = 1.0;
			}

			VisualServer::get_singleton()->canvas_item_add_texture_rect_region(p_canvas_item, Rect2(gl.align + p_pos, gl.size * Vector2(scale_color_font, scale_color_font)), texture_cache[gl.id].image->get_rid(), gl.uv, modulate, false, RID(), false);
		}
	}
}

Array TLDynamicFontFaceAtSize::get_glyph_outline(const Point2 p_pos, uint32_t p_codepoint) const {

	Array ret;
	if (loaded) {
		if (FT_Load_Glyph(ft_face, p_codepoint, FT_LOAD_NO_BITMAP) != 0)
			return ret;

		FT_Glyph ft_glyph;

		if (FT_Get_Glyph(ft_face->glyph, &ft_glyph) != 0)
			return ret;

		if (ft_glyph->format != FT_GLYPH_FORMAT_OUTLINE)
			return ret;

		FT_Outline ol = ((FT_OutlineGlyph)ft_glyph)->outline;

		int prev = 0;
		for (int i = 0; i < ol.n_contours; i++) {
			PoolVector3Array contour;
			contour.resize(ol.contours[i] - prev);
			PoolVector3Array::Write w = contour.write();

			for (int j = 0; j < ol.contours[i] - prev; j++) {
				w[j] = Vector3(p_pos.x + ol.points[prev + 1 + j].x, p_pos.y + ol.points[prev + 1 + j].x, ol.tags[prev + 1 + j]);
			}
			prev = ol.contours[i];
			ret.push_back(contour);
		}

		FT_Done_Glyph(ft_glyph);
	}
	return ret;
}

TLDynamicFontFaceAtSize::TexturePosition TLDynamicFontFaceAtSize::find_texture_pos_for_glyph(int p_color_size, Image::Format p_image_format, int p_width, int p_height) {
	TexturePosition ret;
	ret.index = -1;
	ret.x = 0;
	ret.y = 0;

	int mw = p_width;
	int mh = p_height;

	for (size_t i = 0; i < texture_cache.size(); i++) {

		const GlyphTexture &gl = texture_cache[i];

		if (gl.image->get_format() != p_image_format)
			continue;

		if (mw > gl.texture_size || mh > gl.texture_size) //too big for this texture
			continue;

		ret.y = 0x7FFFFFFF;
		ret.x = 0;

		for (int j = 0; j < gl.texture_size - mw; j++) {

			int max_y = 0;

			for (int k = j; k < j + mw; k++) {

				int y = gl.offsets[k];
				if (y > max_y)
					max_y = y;
			}

			if (max_y < ret.y) {
				ret.y = max_y;
				ret.x = j;
			}
		}

		if (ret.y == 0x7FFFFFFF || ret.y + mh > gl.texture_size)
			continue; //fail, could not fit it here

		ret.index = i;
		break;
	}

	if (ret.index == -1) {
		//could not find texture to fit, create one
		ret.x = 0;
		ret.y = 0;

		int texsize = MAX(ft_size * oversampling * 12, 512);
		if (mw > texsize)
			texsize = mw; //special case, adapt to it?
		if (mh > texsize)
			texsize = mh; //special case, adapt to it?

		texsize = _next_power_of_2(texsize);
		texsize = MIN(texsize, 4096);

		GlyphTexture tex;
		tex.texture_size = texsize;
		tex.imgdata.resize(texsize * texsize * p_color_size); //grayscale alpha

		//zero texture
		PoolByteArray::Write w = tex.imgdata.write();
		ERR_FAIL_COND_V(texsize * texsize * p_color_size > tex.imgdata.size(), ret);
		for (int i = 0; i < texsize * texsize * p_color_size; i++) {
			w[i] = 0;
		}

		tex.offsets.resize(texsize);
		for (int i = 0; i < texsize; i++) //zero offsets
			tex.offsets[i] = 0;

		texture_cache.push_back(tex);
		ret.index = texture_cache.size() - 1;
	}

	return ret;
}

TLDynamicFontFaceAtSize::Glyph TLDynamicFontFaceAtSize::bitmap_to_glyph(FT_Bitmap p_bitmap, int p_yofs, int p_xofs) {

	int rect_margin = 1; //const

	int w = p_bitmap.width;
	int h = p_bitmap.rows;

	int mw = w + rect_margin * 2;
	int mh = h + rect_margin * 2;

	ERR_FAIL_COND_V(mw > 4096, Glyph());
	ERR_FAIL_COND_V(mh > 4096, Glyph());

	int color_size = p_bitmap.pixel_mode == FT_PIXEL_MODE_BGRA ? 4 : 2;
	Image::Format require_format = color_size == 4 ? Image::FORMAT_RGBA8 : Image::FORMAT_LA8;

	TexturePosition tex_pos = find_texture_pos_for_glyph(color_size, require_format, mw, mh);
	ERR_FAIL_COND_V(tex_pos.index < 0, Glyph());

	//fit character in char texture

	GlyphTexture &tex = texture_cache[tex_pos.index];

	{
		PoolByteArray::Write wr = tex.imgdata.write();

		for (int i = 0; i < h; i++) {
			for (int j = 0; j < w; j++) {

				int ofs = ((i + tex_pos.y + rect_margin) * tex.texture_size + j + tex_pos.x + rect_margin) * color_size;
				ERR_FAIL_COND_V(ofs >= tex.imgdata.size(), Glyph());
				switch (p_bitmap.pixel_mode) {
					case FT_PIXEL_MODE_MONO: {
						int byte = i * p_bitmap.pitch + (j >> 3);
						int bit = 1 << (7 - (j % 8));
						wr[ofs + 0] = 255; //grayscale as 1
						wr[ofs + 1] = (p_bitmap.buffer[byte] & bit) ? 255 : 0;
					} break;
					case FT_PIXEL_MODE_GRAY:
						wr[ofs + 0] = 255; //grayscale as 1
						wr[ofs + 1] = p_bitmap.buffer[i * p_bitmap.pitch + j];
						break;
					case FT_PIXEL_MODE_BGRA: {
						int ofs_color = i * p_bitmap.pitch + (j << 2);
						wr[ofs + 2] = p_bitmap.buffer[ofs_color + 0];
						wr[ofs + 1] = p_bitmap.buffer[ofs_color + 1];
						wr[ofs + 0] = p_bitmap.buffer[ofs_color + 2];
						wr[ofs + 3] = p_bitmap.buffer[ofs_color + 3];
					} break;
					// TODO: FT_PIXEL_MODE_LCD
					default:
						ERR_PRINTS("Font uses unsupported pixel format: " + String::num_int64(p_bitmap.pixel_mode));
						ERR_FAIL_V(Glyph());
						break;
				}
			}
		}
	}

	//blit to image and texture
	{

#ifdef GODOT_MODULE
		Ref<Image> image = memnew(Image(tex.texture_size, tex.texture_size, 0, require_format, tex.imgdata));
#else
		Ref<Image> image;
		image.instance();
		image->create_from_data(tex.texture_size, tex.texture_size, 0, require_format, tex.imgdata);
#endif
		tex.image->create_from_image(image, txt_flags);
	}

	// update height array

	for (int k = tex_pos.x; k < tex_pos.x + mw; k++) {
		tex.offsets[k] = tex_pos.y + mh;
	}

	Glyph gl;
	gl.align = Point2(p_xofs * scale_color_font / oversampling, ascent - p_yofs * scale_color_font / oversampling);
	gl.id = tex_pos.index;
	gl.valid = true;
	gl.uv = Rect2(tex_pos.x + rect_margin, tex_pos.y + rect_margin, w, h);
	gl.size = gl.uv.size / oversampling;

	return gl;
}

void TLDynamicFontFaceAtSize::update_glyph(uint32_t p_codepoint) {

	if (loaded) {
		if (glyph_cache.count(p_codepoint) > 0)
			return;

		int ft_hinting;

		switch (hinting) {
			case DF_HINTING_NONE:
				ft_hinting = FT_LOAD_NO_HINTING;
				break;
			case DF_HINTING_LIGHT:
				ft_hinting = FT_LOAD_TARGET_LIGHT;
				break;
			default:
				ft_hinting = FT_LOAD_TARGET_NORMAL;
				break;
		}

		FT_GlyphSlot slot = ft_face->glyph;
		if (FT_Load_Glyph(ft_face, p_codepoint, FT_HAS_COLOR(ft_face) ? FT_LOAD_COLOR : FT_LOAD_DEFAULT | (force_autohinter ? FT_LOAD_FORCE_AUTOHINT : 0) | ft_hinting) != 0) {
			return;
		}
		if (FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL) != 0) {
			return;
		}
		glyph_cache[p_codepoint] = bitmap_to_glyph(slot->bitmap, slot->bitmap_top, slot->bitmap_left);
	}
}

void TLDynamicFontFaceAtSize::update_outline_glyph(uint32_t p_codepoint) {

	if (loaded) {
		if (glyph_outline_cache.count(p_codepoint) > 0)
			return;

		int ft_hinting;

		switch (hinting) {
			case DF_HINTING_NONE:
				ft_hinting = FT_LOAD_NO_HINTING;
				break;
			case DF_HINTING_LIGHT:
				ft_hinting = FT_LOAD_TARGET_LIGHT;
				break;
			default:
				ft_hinting = FT_LOAD_TARGET_NORMAL;
				break;
		}

		if (FT_Load_Glyph(ft_face, p_codepoint, FT_LOAD_NO_BITMAP | (force_autohinter ? FT_LOAD_FORCE_AUTOHINT : 0) | ft_hinting) != 0)
			return;

		FT_Stroker ft_stroker;
		if (FT_Stroker_New(ft_library, &ft_stroker) != 0)
			return;

		FT_Stroker_Set(ft_stroker, ft_size * oversampling, FT_STROKER_LINECAP_BUTT, FT_STROKER_LINEJOIN_ROUND, 0);
		FT_Glyph ft_glyph;
		FT_BitmapGlyph ft_glyph_bitmap;

		if (FT_Get_Glyph(ft_face->glyph, &ft_glyph) != 0)
			goto cleanup_stroker;
		if (FT_Glyph_Stroke(&ft_glyph, ft_stroker, 1) != 0)
			goto cleanup_glyph;
		if (FT_Glyph_To_Bitmap(&ft_glyph, FT_RENDER_MODE_NORMAL, 0, 1) != 0)
			goto cleanup_glyph;

		ft_glyph_bitmap = (FT_BitmapGlyph)ft_glyph;
		glyph_outline_cache[p_codepoint] = bitmap_to_glyph(ft_glyph_bitmap->bitmap, ft_glyph_bitmap->top, ft_glyph_bitmap->left);

	cleanup_glyph:
		FT_Done_Glyph(ft_glyph);
	cleanup_stroker:
		FT_Stroker_Done(ft_stroker);
	}
}

hb_font_t *TLDynamicFontFaceAtSize::get_hb_font() const {

	return loaded ? hb_font : NULL;
}

bool TLDynamicFontFaceAtSize::load(String p_resource_path, int p_size) {

	if (loaded) {
		//unload existing
		clear_cache();
		if (hb_font) {
			hb_font_destroy(hb_font);
			hb_font = NULL;
		}
		FT_Done_FreeType(ft_library);

		loaded = false;
	}

	if (FT_Init_FreeType(&ft_library) != 0) {
		ERR_PRINTS("Error initializing FreeType library");
		clear_cache();
		return false;
	}
	File *file = memnew(File);
	if (file->open(p_resource_path, File::READ) != Error::OK) {
		ERR_PRINTS("Can't open dynamic font file: \"" + p_resource_path + "\"");
		clear_cache();
		FT_Done_FreeType(ft_library);
		return false;
	}

	memset(&ft_stream, 0, sizeof(FT_StreamRec));
	ft_stream.base = NULL;
	ft_stream.size = file->get_len();
	ft_stream.pos = 0;
	ft_stream.descriptor.pointer = file;
	ft_stream.read = ft_stream_io;
	ft_stream.close = ft_stream_close;

	FT_Open_Args ft_args;
	memset(&ft_args, 0, sizeof(FT_Open_Args));
	ft_args.flags = FT_OPEN_STREAM;
	ft_args.stream = &ft_stream;
	if (FT_Open_Face(ft_library, &ft_args, 0, &ft_face) != 0) {
		ERR_PRINTS("Error loading dynamic font: \"" + p_resource_path + "\" - FreeType uninitialized");
		clear_cache();
		FT_Done_FreeType(ft_library);
		return false;
	}

	ft_size = p_size;

	if (FT_HAS_COLOR(ft_face)) {
		int best_match = 0;
		int diff = abs(p_size - ft_face->available_sizes[0].width);
		scale_color_font = (float)p_size / ft_face->available_sizes[0].width;
		for (int i = 1; i < ft_face->num_fixed_sizes; i++) {
			int ndiff = abs(p_size - ft_face->available_sizes[i].width);
			if (ndiff < diff) {
				best_match = i;
				diff = ndiff;
				scale_color_font = (float)p_size / ft_face->available_sizes[i].width;
			}
		}
		FT_Select_Size(ft_face, best_match);
		ascent = (ft_face->size->metrics.ascender / 64.0f) * scale_color_font;
		descent = (-ft_face->size->metrics.descender / 64.0f) * scale_color_font;
		height = (ft_face->size->metrics.height / 64.0f) * scale_color_font;
	} else {
		scale_color_font = 1.0f;
		FT_Set_Pixel_Sizes(ft_face, 0, p_size * oversampling);
		ascent = (ft_face->size->metrics.ascender / 64.0f) / oversampling;
		descent = (-ft_face->size->metrics.descender / 64.0f) / oversampling;
		height = (ft_face->size->metrics.height / 64.0f) / oversampling;
	}

	FT_Select_Charmap(ft_face, FT_ENCODING_UNICODE);

	hb_font = hb_ft_font_create(ft_face, NULL);
	if (!hb_font) {
		ERR_PRINTS("Error loading dynamic font: \"" + p_resource_path + "\" - HarfBuzz uninitialized");
		clear_cache();
		FT_Done_FreeType(ft_library);
		return false;
	}

	//Load os2 TTF pable
	os2 = (TT_OS2 *)FT_Get_Sfnt_Table(ft_face, FT_SFNT_OS2);

	loaded = true;

	return loaded;
}

std::vector<hb_script_t> TLDynamicFontFaceAtSize::unicode_scripts_supported() const {
	//DEBUG
	/*
	for (int i = 31; i >= 0; i--)
		printf("%s", ((os2->ulUnicodeRange1 >> i) & 1) ? "+" : "-");
	printf(" ");
	for (int i = 31; i >= 0; i--)
		printf("%s", ((os2->ulUnicodeRange2 >> i) & 1) ? "+" : "-");
	printf(" ");
	for (int i = 31; i >= 0; i--)
		printf("%s", ((os2->ulUnicodeRange3 >> i) & 1) ? "+" : "-");
	printf(" ");
	for (int i = 31; i >= 0; i--)
		printf("%s", ((os2->ulUnicodeRange4 >> i) & 1) ? "+" : "-");
	printf("\n");
	*/
	//DEBUG

	//https://freetype.org/freetype2/docs/reference/ft2-truetype_tables.html#tt_ucr_xxx

	std::vector<hb_script_t> ret;
	if (!os2)
		return ret;

	if ((os2->ulUnicodeRange1 & 1L << 0) || (os2->ulUnicodeRange1 & 1L << 1) || (os2->ulUnicodeRange1 & 1L << 2) || (os2->ulUnicodeRange1 & 1L << 3) || (os2->ulUnicodeRange1 & 1L << 29)) {
		ret.push_back(HB_SCRIPT_LATIN);
	}
	if ((os2->ulUnicodeRange1 & 1L << 4) || (os2->ulUnicodeRange1 & 1L << 5) || (os2->ulUnicodeRange1 & 1L << 6) || (os2->ulUnicodeRange1 & 1L << 31) || (os2->ulUnicodeRange2 & 1L << 0) || (os2->ulUnicodeRange2 & 1L << 1) || (os2->ulUnicodeRange2 & 1L << 2) || (os2->ulUnicodeRange2 & 1L << 3) || (os2->ulUnicodeRange2 & 1L << 4) || (os2->ulUnicodeRange2 & 1L << 5) || (os2->ulUnicodeRange2 & 1L << 6) || (os2->ulUnicodeRange2 & 1L << 7) || (os2->ulUnicodeRange2 & 1L << 8) || (os2->ulUnicodeRange2 & 1L << 9) || (os2->ulUnicodeRange2 & 1L << 10) || (os2->ulUnicodeRange2 & 1L << 11) || (os2->ulUnicodeRange2 & 1L << 12) || (os2->ulUnicodeRange2 & 1L << 13) || (os2->ulUnicodeRange2 & 1L << 14) || (os2->ulUnicodeRange2 & 1L << 15) || (os2->ulUnicodeRange2 & 1L << 30) || (os2->ulUnicodeRange3 & 1L << 0) || (os2->ulUnicodeRange3 & 1L << 1) || (os2->ulUnicodeRange3 & 1L << 2) || (os2->ulUnicodeRange3 & 1L << 4) || (os2->ulUnicodeRange3 & 1L << 5) || (os2->ulUnicodeRange3 & 1L << 18) || (os2->ulUnicodeRange3 & 1L << 24) || (os2->ulUnicodeRange3 & 1L << 25) || (os2->ulUnicodeRange3 & 1L << 26) || (os2->ulUnicodeRange3 & 1L << 27) || (os2->ulUnicodeRange3 & 1L << 28) || (os2->ulUnicodeRange4 & 1L << 3) || (os2->ulUnicodeRange4 & 1L << 6) || (os2->ulUnicodeRange4 & 1L << 15) || (os2->ulUnicodeRange4 & 1L << 23) || (os2->ulUnicodeRange4 & 1L << 24) || (os2->ulUnicodeRange4 & 1L << 26)) {
		//TODO: add more script sorting
		ret.push_back(HB_SCRIPT_COMMON);
	}
	if ((os2->ulUnicodeRange1 & 1L << 7) || (os2->ulUnicodeRange1 & 1L << 30)) {
		ret.push_back(HB_SCRIPT_GREEK);
	}
	if ((os2->ulUnicodeRange1 & 1L << 8)) {
		ret.push_back(HB_SCRIPT_COPTIC);
	}
	if ((os2->ulUnicodeRange1 & 1L << 9)) {
		ret.push_back(HB_SCRIPT_CYRILLIC);
	}
	if ((os2->ulUnicodeRange1 & 1L << 10)) {
		ret.push_back(HB_SCRIPT_ARMENIAN);
	}
	if ((os2->ulUnicodeRange1 & 1L << 11)) {
		ret.push_back(HB_SCRIPT_HEBREW);
	}
	if ((os2->ulUnicodeRange1 & 1L << 12)) {
		ret.push_back(HB_SCRIPT_VAI);
	}
	if ((os2->ulUnicodeRange1 & 1L << 13) || (os2->ulUnicodeRange2 & 1L << 31) || (os2->ulUnicodeRange3 & 1L << 3)) {
		ret.push_back(HB_SCRIPT_ARABIC);
	}
	if ((os2->ulUnicodeRange1 & 1L << 14)) {
		ret.push_back(HB_SCRIPT_NKO);
	}
	if ((os2->ulUnicodeRange1 & 1L << 15)) {
		ret.push_back(HB_SCRIPT_DEVANAGARI);
	}
	if ((os2->ulUnicodeRange1 & 1L << 16)) {
		ret.push_back(HB_SCRIPT_BENGALI);
	}
	if ((os2->ulUnicodeRange1 & 1L << 17)) {
		ret.push_back(HB_SCRIPT_GURMUKHI);
	}
	if ((os2->ulUnicodeRange1 & 1L << 18)) {
		ret.push_back(HB_SCRIPT_GUJARATI);
	}
	if ((os2->ulUnicodeRange1 & 1L << 19)) {
		ret.push_back(HB_SCRIPT_ORIYA);
	}
	if ((os2->ulUnicodeRange1 & 1L << 20)) {
		ret.push_back(HB_SCRIPT_TAMIL);
	}
	if ((os2->ulUnicodeRange1 & 1L << 21)) {
		ret.push_back(HB_SCRIPT_TELUGU);
	}
	if ((os2->ulUnicodeRange1 & 1L << 22)) {
		ret.push_back(HB_SCRIPT_KANNADA);
	}
	if ((os2->ulUnicodeRange1 & 1L << 23)) {
		ret.push_back(HB_SCRIPT_MALAYALAM);
	}
	if ((os2->ulUnicodeRange1 & 1L << 24)) {
		ret.push_back(HB_SCRIPT_THAI);
	}
	if ((os2->ulUnicodeRange1 & 1L << 25)) {
		ret.push_back(HB_SCRIPT_LAO);
	}
	if ((os2->ulUnicodeRange1 & 1L << 26)) {
		ret.push_back(HB_SCRIPT_GEORGIAN);
	}
	if ((os2->ulUnicodeRange1 & 1L << 27)) {
		ret.push_back(HB_SCRIPT_BALINESE);
	}
	if ((os2->ulUnicodeRange1 & 1L << 28) || (os2->ulUnicodeRange2 & 1L << 20) || (os2->ulUnicodeRange2 & 1L << 20) || (os2->ulUnicodeRange2 & 1L << 24)) {
		//HANG and JAMO
		ret.push_back(HB_SCRIPT_HANGUL);
	}
	if ((os2->ulUnicodeRange2 & 1L << 16) || (os2->ulUnicodeRange2 & 1L << 21) || (os2->ulUnicodeRange2 & 1L << 22) || (os2->ulUnicodeRange2 & 1L << 23) || (os2->ulUnicodeRange2 & 1L << 26) || (os2->ulUnicodeRange2 & 1L << 27) || (os2->ulUnicodeRange2 & 1L << 28) || (os2->ulUnicodeRange2 & 1L << 29)) {
		ret.push_back(HB_SCRIPT_HAN);
	}
	if ((os2->ulUnicodeRange2 & 1L << 17)) {
		ret.push_back(HB_SCRIPT_HIRAGANA);
	}
	if ((os2->ulUnicodeRange2 & 1L << 18)) {
		ret.push_back(HB_SCRIPT_KATAKANA);
	}
	if ((os2->ulUnicodeRange2 & 1L << 19)) {
		ret.push_back(HB_SCRIPT_BOPOMOFO);
	}
	//Bit 25 - Surrogates - Ignore
	if ((os2->ulUnicodeRange3 & 1L << 6)) {
		ret.push_back(HB_SCRIPT_TIBETAN);
	}
	if ((os2->ulUnicodeRange3 & 1L << 7)) {
		ret.push_back(HB_SCRIPT_SYRIAC);
	}
	if ((os2->ulUnicodeRange3 & 1L << 8)) {
		ret.push_back(HB_SCRIPT_THAANA);
	}
	if ((os2->ulUnicodeRange3 & 1L << 9)) {
		ret.push_back(HB_SCRIPT_SINHALA);
	}
	if ((os2->ulUnicodeRange3 & 1L << 10)) {
		ret.push_back(HB_SCRIPT_MYANMAR);
	}
	if ((os2->ulUnicodeRange3 & 1L << 11)) {
		ret.push_back(HB_SCRIPT_ETHIOPIC);
	}
	if ((os2->ulUnicodeRange3 & 1L << 12)) {
		ret.push_back(HB_SCRIPT_CHEROKEE);
	}
	if ((os2->ulUnicodeRange3 & 1L << 13)) {
		ret.push_back(HB_SCRIPT_CANADIAN_SYLLABICS);
	}
	if ((os2->ulUnicodeRange3 & 1L << 14)) {
		ret.push_back(HB_SCRIPT_OGHAM);
	}
	if ((os2->ulUnicodeRange3 & 1L << 15)) {
		ret.push_back(HB_SCRIPT_RUNIC);
	}
	if ((os2->ulUnicodeRange3 & 1L << 16)) {
		ret.push_back(HB_SCRIPT_KHMER);
	}
	if ((os2->ulUnicodeRange3 & 1L << 17)) {
		ret.push_back(HB_SCRIPT_MONGOLIAN);
	}
	if ((os2->ulUnicodeRange3 & 1L << 19)) {
		ret.push_back(HB_SCRIPT_YI);
	}
	if ((os2->ulUnicodeRange3 & 1L << 20)) {
		ret.push_back(HB_SCRIPT_TAGALOG);
		ret.push_back(HB_SCRIPT_HANUNOO);
		ret.push_back(HB_SCRIPT_TAGBANWA);
		ret.push_back(HB_SCRIPT_BUHID);
	}
	if ((os2->ulUnicodeRange3 & 1L << 21)) {
		ret.push_back(HB_SCRIPT_OLD_ITALIC);
	}
	if ((os2->ulUnicodeRange3 & 1L << 22)) {
		ret.push_back(HB_SCRIPT_GOTHIC);
	}
	if ((os2->ulUnicodeRange3 & 1L << 23)) {
		ret.push_back(HB_SCRIPT_DESERET);
	}
	if ((os2->ulUnicodeRange3 & 1L << 29)) {
		ret.push_back(HB_SCRIPT_LIMBU);
	}
	if ((os2->ulUnicodeRange3 & 1L << 30)) {
		ret.push_back(HB_SCRIPT_TAI_LE);
	}
	if ((os2->ulUnicodeRange3 & 1L << 31)) {
		ret.push_back(HB_SCRIPT_NEW_TAI_LUE);
	}
	if ((os2->ulUnicodeRange4 & 1L << 0)) {
		ret.push_back(HB_SCRIPT_BUGINESE);
	}
	if ((os2->ulUnicodeRange4 & 1L << 1)) {
		ret.push_back(HB_SCRIPT_GLAGOLITIC);
	}
	if ((os2->ulUnicodeRange4 & 1L << 2)) {
		ret.push_back(HB_SCRIPT_TIFINAGH);
	}
	if ((os2->ulUnicodeRange4 & 1L << 4)) {
		ret.push_back(HB_SCRIPT_SYLOTI_NAGRI);
	}
	if ((os2->ulUnicodeRange4 & 1L << 5)) {
		ret.push_back(HB_SCRIPT_LINEAR_B);
	}
	if ((os2->ulUnicodeRange4 & 1L << 7)) {
		ret.push_back(HB_SCRIPT_UGARITIC);
	}
	if ((os2->ulUnicodeRange4 & 1L << 8)) {
		ret.push_back(HB_SCRIPT_OLD_PERSIAN);
	}
	if ((os2->ulUnicodeRange4 & 1L << 9)) {
		ret.push_back(HB_SCRIPT_SHAVIAN);
	}
	if ((os2->ulUnicodeRange4 & 1L << 10)) {
		ret.push_back(HB_SCRIPT_OSMANYA);
	}
	if ((os2->ulUnicodeRange4 & 1L << 11)) {
		ret.push_back(HB_SCRIPT_CYPRIOT);
	}
	if ((os2->ulUnicodeRange4 & 1L << 12)) {
		ret.push_back(HB_SCRIPT_KHAROSHTHI);
	}
	if ((os2->ulUnicodeRange4 & 1L << 13)) {
		ret.push_back(HB_SCRIPT_TAI_VIET);
	}
	if ((os2->ulUnicodeRange4 & 1L << 14)) {
		ret.push_back(HB_SCRIPT_CUNEIFORM);
	}
	if ((os2->ulUnicodeRange4 & 1L << 16)) {
		ret.push_back(HB_SCRIPT_SUNDANESE);
	}
	if ((os2->ulUnicodeRange4 & 1L << 17)) {
		ret.push_back(HB_SCRIPT_LEPCHA);
	}
	if ((os2->ulUnicodeRange4 & 1L << 18)) {
		ret.push_back(HB_SCRIPT_OL_CHIKI);
	}
	if ((os2->ulUnicodeRange4 & 1L << 19)) {
		ret.push_back(HB_SCRIPT_SAURASHTRA);
	}
	if ((os2->ulUnicodeRange4 & 1L << 20)) {
		ret.push_back(HB_SCRIPT_KAYAH_LI);
	}
	if ((os2->ulUnicodeRange4 & 1L << 21)) {
		ret.push_back(HB_SCRIPT_REJANG);
	}
	if ((os2->ulUnicodeRange4 & 1L << 22)) {
		ret.push_back(HB_SCRIPT_CHAM);
	}
	if ((os2->ulUnicodeRange4 & 1L << 25)) {
		ret.push_back(HB_SCRIPT_ANATOLIAN_HIEROGLYPHS);
	}
	if (ret.size() == 0) {
		ret.push_back(HB_SCRIPT_COMMON);
	}
	//Bits 27...31 - Reserved

	return ret;
}

double TLDynamicFontFaceAtSize::get_ascent() const {

	return loaded ? ascent : 0.0f;
}

double TLDynamicFontFaceAtSize::get_descent() const {

	return loaded ? descent : 0.0f;
}

double TLDynamicFontFaceAtSize::get_height() const {

	return loaded ? height : 0.0f;
}

void TLDynamicFontFaceAtSize::set_texture_flags(int p_flags) {

	if (txt_flags != p_flags) {
		txt_flags = p_flags;
		if (loaded) {
			for (int64_t i = 0; i < texture_cache.size(); i++) {
				Ref<ImageTexture> &tex = texture_cache[i].image;
				if (!tex.is_null())
					tex->set_flags(txt_flags);
			}
		}
	}
}

void TLDynamicFontFaceAtSize::set_hinting(int p_hinting) {

	if (hinting != p_hinting) {
		hinting = (DynamicFaceHinting)p_hinting;
		if (loaded) {
			clear_cache();
		}
	}
}

void TLDynamicFontFaceAtSize::set_force_autohinter(bool p_force) {

	if (force_autohinter != p_force) {
		force_autohinter = p_force;
		if (loaded) {
			clear_cache();
		}
	}
}

void TLDynamicFontFaceAtSize::set_oversampling(float p_oversampling) {

	if (oversampling != p_oversampling) {
		oversampling = p_oversampling;
		if (loaded) {
			clear_cache();
		}
	}
}

/*************************************************************************/
/*  TLDynamicFontFace                                                     */
/*************************************************************************/

TLDynamicFontFace::TLDynamicFontFace() {

#ifdef GODOT_MODULE
	_init();
#endif
}

void TLDynamicFontFace::_init() {

	hinting = DF_HINTING_NORMAL;
	force_autohinter = false;
	txt_flags = Texture::FLAG_VIDEO_SURFACE;
	oversampling = 1.0f;
}

TLDynamicFontFace::~TLDynamicFontFace() {

	for (auto it = sizes.begin(); it != sizes.end(); ++it) {
		delete it->second;
	}
	sizes.clear();
}

hb_font_t *TLDynamicFontFace::get_hb_font(int p_size) const {

	if (sizes.count(p_size) > 0) {
		return sizes.at(p_size)->get_hb_font();
	} else {
		TLDynamicFontFaceAtSize *f_at_s = new TLDynamicFontFaceAtSize();
		f_at_s->set_texture_flags(txt_flags);
		f_at_s->set_hinting(hinting);
		f_at_s->set_force_autohinter(force_autohinter);
		f_at_s->set_oversampling(oversampling);

		if (f_at_s->load(path, p_size)) {
			sizes[p_size] = f_at_s;
			return f_at_s->get_hb_font();
		}
	}
	return NULL;
}

float TLDynamicFontFace::get_glyph_scale(int p_size) const {

	if (sizes.count(p_size) > 0) {
		return sizes.at(p_size)->get_glyph_scale();
	} else {
		TLDynamicFontFaceAtSize *f_at_s = new TLDynamicFontFaceAtSize();
		f_at_s->set_texture_flags(txt_flags);
		f_at_s->set_hinting(hinting);
		f_at_s->set_force_autohinter(force_autohinter);
		f_at_s->set_oversampling(oversampling);

		if (f_at_s->load(path, p_size)) {
			sizes[p_size] = f_at_s;
			return f_at_s->get_glyph_scale();
		}
	}
	return 1.0f;
}

void TLDynamicFontFace::draw_glyph(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate, int p_size) const {

	if (sizes.count(p_size) > 0) {
		sizes.at(p_size)->draw_glyph(p_canvas_item, p_pos, p_codepoint, p_modulate);
	} else {
		TLDynamicFontFaceAtSize *f_at_s = new TLDynamicFontFaceAtSize();
		f_at_s->set_texture_flags(txt_flags);
		f_at_s->set_hinting(hinting);
		f_at_s->set_force_autohinter(force_autohinter);
		f_at_s->set_oversampling(oversampling);

		if (f_at_s->load(path, p_size)) {
			sizes[p_size] = f_at_s;
			f_at_s->draw_glyph(p_canvas_item, p_pos, p_codepoint, p_modulate);
		}
	}
}

void TLDynamicFontFace::_draw_char(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate, int p_size) const {

	//raw char for debug only, do not use
	if (sizes.count(p_size) > 0) {
		sizes.at(p_size)->_draw_char(p_canvas_item, p_pos, p_codepoint, p_modulate);
	} else {
		TLDynamicFontFaceAtSize *f_at_s = new TLDynamicFontFaceAtSize();
		f_at_s->set_texture_flags(txt_flags);
		f_at_s->set_hinting(hinting);
		f_at_s->set_force_autohinter(force_autohinter);
		f_at_s->set_oversampling(oversampling);

		if (f_at_s->load(path, p_size)) {
			sizes[p_size] = f_at_s;
			f_at_s->_draw_char(p_canvas_item, p_pos, p_codepoint, p_modulate);
		}
	}
}

void TLDynamicFontFace::draw_glyph_outline(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate, int p_size) const {

	if (sizes.count(p_size) > 0) {
		sizes.at(p_size)->draw_glyph_outline(p_canvas_item, p_pos, p_codepoint, p_modulate);
	} else {
		TLDynamicFontFaceAtSize *f_at_s = new TLDynamicFontFaceAtSize();
		f_at_s->set_texture_flags(txt_flags);
		f_at_s->set_hinting(hinting);
		f_at_s->set_force_autohinter(force_autohinter);
		f_at_s->set_oversampling(oversampling);

		if (f_at_s->load(path, p_size)) {
			sizes[p_size] = f_at_s;
			f_at_s->draw_glyph_outline(p_canvas_item, p_pos, p_codepoint, p_modulate);
		}
	}
}

Array TLDynamicFontFace::get_glyph_outline(const Point2 p_pos, uint32_t p_codepoint, int p_size) const {

	if (sizes.count(p_size) > 0) {
		return sizes.at(p_size)->get_glyph_outline(p_pos, p_codepoint);
	} else {
		TLDynamicFontFaceAtSize *f_at_s = new TLDynamicFontFaceAtSize();
		f_at_s->set_texture_flags(txt_flags);
		f_at_s->set_hinting(hinting);
		f_at_s->set_force_autohinter(force_autohinter);
		f_at_s->set_oversampling(oversampling);

		if (f_at_s->load(path, p_size)) {
			sizes[p_size] = f_at_s;
			return f_at_s->get_glyph_outline(p_pos, p_codepoint);
		}
	}
	return Array();
}

void TLDynamicFontFace::set_font_path(String p_resource_path) {

	if (path != p_resource_path) {
		load(p_resource_path);
	}
}

bool TLDynamicFontFace::load(String p_resource_path) {

	path = p_resource_path;
	for (auto it = sizes.begin(); it != sizes.end(); ++it) {
		delete it->second;
	}
	sizes.clear();
	emit_signal(_CHANGED);
	return true;
}

std::vector<hb_script_t> TLDynamicFontFace::unicode_scripts_supported() const {

	if (sizes.size() > 0) {
		return sizes.begin()->second->unicode_scripts_supported();
	} else {
		TLDynamicFontFaceAtSize *f_at_s = new TLDynamicFontFaceAtSize();
		f_at_s->set_texture_flags(txt_flags);
		f_at_s->set_hinting(hinting);
		f_at_s->set_force_autohinter(force_autohinter);
		f_at_s->set_oversampling(oversampling);

		if (f_at_s->load(path, 16)) {
			sizes[16] = f_at_s;
			return f_at_s->unicode_scripts_supported();
		}
	}
	WARN_PRINTS("Font not loaded!")
	return std::vector<hb_script_t>();
}

double TLDynamicFontFace::get_ascent(int p_size) const {

	if (sizes.count(p_size) > 0) {
		return sizes.at(p_size)->get_ascent();
	} else {
		TLDynamicFontFaceAtSize *f_at_s = new TLDynamicFontFaceAtSize();
		f_at_s->set_texture_flags(txt_flags);
		f_at_s->set_hinting(hinting);
		f_at_s->set_force_autohinter(force_autohinter);
		f_at_s->set_oversampling(oversampling);

		if (f_at_s->load(path, p_size)) {
			sizes[p_size] = f_at_s;
			return f_at_s->get_ascent();
		}
	}
	WARN_PRINTS("Font not loaded!")
	return 0.0f;
}

double TLDynamicFontFace::get_descent(int p_size) const {

	if (sizes.count(p_size) > 0) {
		return sizes.at(p_size)->get_descent();
	} else {
		TLDynamicFontFaceAtSize *f_at_s = new TLDynamicFontFaceAtSize();
		f_at_s->set_texture_flags(txt_flags);
		f_at_s->set_hinting(hinting);
		f_at_s->set_force_autohinter(force_autohinter);
		f_at_s->set_oversampling(oversampling);

		if (f_at_s->load(path, p_size)) {
			sizes[p_size] = f_at_s;
			return f_at_s->get_descent();
		}
	}
	WARN_PRINTS("Font not loaded!")
	return 0.0f;
}

double TLDynamicFontFace::get_height(int p_size) const {

	if (sizes.count(p_size) > 0) {
		return sizes.at(p_size)->get_height();
	} else {
		TLDynamicFontFaceAtSize *f_at_s = new TLDynamicFontFaceAtSize();
		f_at_s->set_texture_flags(txt_flags);
		f_at_s->set_hinting(hinting);
		f_at_s->set_force_autohinter(force_autohinter);
		f_at_s->set_oversampling(oversampling);

		if (f_at_s->load(path, p_size)) {
			sizes[p_size] = f_at_s;
			return f_at_s->get_height();
		}
	}
	WARN_PRINTS("Font not loaded!")
	return 0.0f;
}

void TLDynamicFontFace::set_texture_flags(int p_flags) {

	if (txt_flags != p_flags) {
		txt_flags = p_flags;
		for (auto it = sizes.begin(); it != sizes.end(); ++it) {
			it->second->set_texture_flags(p_flags);
		}
		emit_signal(_CHANGED);
	}
}

int TLDynamicFontFace::get_texture_flags() const {

	return txt_flags;
}

void TLDynamicFontFace::set_hinting(int p_hinting) {

	if (hinting != (DynamicFaceHinting)p_hinting) {
		hinting = (DynamicFaceHinting)p_hinting;
		for (auto it = sizes.begin(); it != sizes.end(); ++it) {
			it->second->set_hinting(p_hinting);
		}
		emit_signal(_CHANGED);
	}
}

int TLDynamicFontFace::get_hinting() const {

	return hinting;
}

void TLDynamicFontFace::set_force_autohinter(bool p_force) {

	if (force_autohinter != p_force) {
		force_autohinter = p_force;
		for (auto it = sizes.begin(); it != sizes.end(); ++it) {
			it->second->set_force_autohinter(p_force);
		}
		emit_signal(_CHANGED);
	}
}

bool TLDynamicFontFace::get_force_autohinter() const {

	return force_autohinter;
}

void TLDynamicFontFace::set_oversampling(float p_oversampling) {

	if (oversampling != p_oversampling) {
		oversampling = p_oversampling;
		for (auto it = sizes.begin(); it != sizes.end(); ++it) {
			it->second->set_oversampling(p_oversampling);
		}
		emit_signal(_CHANGED);
	}
}

float TLDynamicFontFace::get_oversampling() const {

	return oversampling;
}

bool TLDynamicFontFace::has_graphite() const {

#ifdef HAVE_GRAPHITE2
	return true;
#else
	return false;
#endif
}

#ifdef GODOT_MODULE
void TLDynamicFontFace::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_force_autohinter", "force_autohinter"), &TLDynamicFontFace::set_force_autohinter);
	ClassDB::bind_method(D_METHOD("get_force_autohinter"), &TLDynamicFontFace::get_force_autohinter);

	ClassDB::bind_method(D_METHOD("set_hinting", "hinting"), &TLDynamicFontFace::set_hinting);
	ClassDB::bind_method(D_METHOD("get_hinting"), &TLDynamicFontFace::get_hinting);

	ClassDB::bind_method(D_METHOD("set_oversampling", "oversampling"), &TLDynamicFontFace::set_oversampling);
	ClassDB::bind_method(D_METHOD("get_oversampling"), &TLDynamicFontFace::get_oversampling);

	ClassDB::bind_method(D_METHOD("has_graphite"), &TLDynamicFontFace::has_graphite);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "force_autohinter"), "set_force_autohinter", "get_force_autohinter");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "hinting", PROPERTY_HINT_ENUM, "None,Light,Normal"), "set_hinting", "get_hinting");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "oversampling"), "set_oversampling", "get_oversampling");

	BIND_ENUM_CONSTANT(DF_HINTING_NONE);
	BIND_ENUM_CONSTANT(DF_HINTING_LIGHT);
	BIND_ENUM_CONSTANT(DF_HINTING_NORMAL);
}
#else
void TLDynamicFontFace::_register_methods() {

	register_method("set_force_autohinter", &TLDynamicFontFace::set_force_autohinter);
	register_method("get_force_autohinter", &TLDynamicFontFace::get_force_autohinter);

	register_method("set_hinting", &TLDynamicFontFace::set_hinting);
	register_method("get_hinting", &TLDynamicFontFace::get_hinting);

	register_method("set_oversampling", &TLDynamicFontFace::set_oversampling);
	register_method("get_oversampling", &TLDynamicFontFace::get_oversampling);

	register_method("has_graphite", &TLDynamicFontFace::has_graphite);

	register_property<TLDynamicFontFace, bool>("force_autohinter", &TLDynamicFontFace::set_force_autohinter, &TLDynamicFontFace::get_force_autohinter, false);
	register_property<TLDynamicFontFace, int>("hinting", &TLDynamicFontFace::set_hinting, &TLDynamicFontFace::get_hinting, DF_HINTING_NORMAL, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM, String("None,Light,Normal"));
	register_property<TLDynamicFontFace, float>("oversampling", &TLDynamicFontFace::set_oversampling, &TLDynamicFontFace::get_oversampling, 1.0f);
}

#endif
