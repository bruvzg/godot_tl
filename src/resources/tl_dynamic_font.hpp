/*************************************************************************/
/*  tl_dynamic_font.hpp                                                  */
/*************************************************************************/

#ifndef TL_DYNAMIC_FONT_FACE_HPP
#define TL_DYNAMIC_FONT_FACE_HPP

#include "tl_font.hpp"

#include <cmath>
#include <map>
#include <vector>

#include <ft2build.h>
#include <hb-ft.h>

#include FT_TRUETYPE_TABLES_H

using namespace godot;

enum DynamicFaceHinting {
	DF_HINTING_NONE = 0,
	DF_HINTING_LIGHT = 1,
	DF_HINTING_NORMAL = 2
};

class TLDynamicFontFaceAtSize {

protected:
	//Common FreeType data
	bool loaded;
	FT_Library ft_library;
	FT_StreamRec ft_stream;

	float oversampling;
	DynamicFaceHinting hinting;
	bool force_autohinter;
	int txt_flags;

	static unsigned long ft_stream_io(FT_Stream p_stream, unsigned long p_offset, unsigned char *p_buffer, unsigned long p_count);
	static void ft_stream_close(FT_Stream p_stream);

	//Texture cache
	struct GlyphTexture {
		PoolByteArray imgdata;
		//Image::Format format;
		//RID rid;
		Ref<ImageTexture> image;
		std::vector<int> offsets;
		int texture_size;

		GlyphTexture() {
			image.instance();
			texture_size = 0;
		};
	};

	struct TexturePosition {
		int index;
		int x;
		int y;
	};

	struct Glyph {
		bool valid;

		Point2 align;
		Size2 size;
		Rect2 uv;
		size_t id;

		Glyph() {
			valid = false;
			id = 0;
		}
	};

	TexturePosition find_texture_pos_for_glyph(int p_color_size, Image::Format p_image_format, int p_width, int p_height);
	Glyph bitmap_to_glyph(FT_Bitmap p_bitmap, int p_yofs, int p_xofs);
	void clear_cache();

	//FreeType data for size
	FT_Face ft_face;
	hb_font_t *hb_font;

	int ft_size;
	float scale_color_font;

	TT_OS2 *os2;

	float ascent;
	float descent;
	float height;

	std::vector<GlyphTexture> texture_cache;
	std::map<uint32_t, Glyph> glyph_cache;
	std::map<uint32_t, Glyph> glyph_outline_cache;

	void update_glyph(uint32_t p_codepoint);
	void update_outline_glyph(uint32_t p_codepoint);

	_ALWAYS_INLINE_ unsigned int _next_power_of_2(unsigned int x);

public:
	TLDynamicFontFaceAtSize();
	~TLDynamicFontFaceAtSize();

	hb_font_t *get_hb_font() const;
	float get_glyph_scale() const;

	void draw_glyph(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate) const;
	void _draw_char(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate) const; //raw char for debug only, do not use
	void draw_glyph_outline(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate) const;
	Array get_glyph_outline(const Point2 p_pos, uint32_t p_codepoint) const;

	std::vector<hb_script_t> unicode_scripts_supported() const;
	//Array unicode_scripts_supported() const;
	//bool unicode_range_supported(uint8_t p_bank, uint32_t p_range) const;

	bool load(String p_resource_path, int p_size, const uint8_t *p_font_mem, int p_font_mem_size);

	double get_ascent() const;
	double get_descent() const;
	double get_height() const;

	void set_texture_flags(int p_flags);
	void set_hinting(int p_hinting);
	void set_force_autohinter(bool p_force);
	void set_oversampling(float p_oversampling);
};

class TLDynamicFontFace : public TLFontFace {
	GODOT_SUBCLASS(TLDynamicFontFace, TLFontFace);

protected:
	float oversampling;
	DynamicFaceHinting hinting;
	bool force_autohinter;
	int txt_flags;

	const uint8_t *font_mem;
	int font_mem_size;

	mutable std::map<int, TLDynamicFontFaceAtSize *> sizes;

public:
	TLDynamicFontFace();
	virtual ~TLDynamicFontFace();

	void _init();

	virtual hb_font_t *get_hb_font(int p_size) const override;
	virtual float get_glyph_scale(int p_size) const override;

	virtual void draw_glyph(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate, int p_size) const override;
	virtual void _draw_char(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate, int p_size) const override; //raw char for debug only, do not use
	virtual void draw_glyph_outline(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate, int p_size) const override;
	virtual Array get_glyph_outline(const Point2 p_pos, uint32_t p_codepoint, int p_size) const override;

	virtual std::vector<hb_script_t> unicode_scripts_supported() const override;

	//GDNative methods
	virtual bool load(String p_resource_path) override;

	virtual void set_font_ptr(const uint8_t *p_font_mem, int p_font_mem_size);
	virtual void set_font_path(String p_resource_path) override;

	virtual double get_ascent(int p_size) const override;
	virtual double get_descent(int p_size) const override;
	virtual double get_height(int p_size) const override;

	virtual void set_texture_flags(int p_flags) override;
	virtual int get_texture_flags() const override;

	void set_hinting(int p_hinting);
	int get_hinting() const;

	void set_force_autohinter(bool p_force);
	bool get_force_autohinter() const;

	void set_oversampling(float p_oversampling);
	float get_oversampling() const;

	bool has_graphite() const;

#ifdef GODOT_MODULE
	static void _bind_methods();
#else
	static void _register_methods();
#endif
};

#ifdef GODOT_MODULE
VARIANT_ENUM_CAST(DynamicFaceHinting);
#endif

#endif /*TL_DYNAMIC_FONT_FACE_HPP*/
