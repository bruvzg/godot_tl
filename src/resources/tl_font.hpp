/*************************************************************************/
/*  tl_font.hpp                                                          */
/*************************************************************************/

#ifndef TL_FONT_FACE_HPP
#define TL_FONT_FACE_HPP

#include "tl_core.hpp"

#ifdef GODOT_MODULE
#include "core/resource.h"
#include "scene/resources/texture.h"
#else
#include <Image.hpp>
#include <ImageTexture.hpp>
#include <RID.hpp>
#endif

#include <vector>

#include <hb.h>

using namespace godot;

//http://font.gohu.org/ (WTFPL) based 5x7 hex number font for char code box drawing
static const unsigned char _hex_box_img_data[167] = {
	0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x07, 0x01, 0x03, 0x00, 0x00, 0x00, 0xA5, 0x54, 0x58, 0xA1, 0x00, 0x00, 0x00, 0x06, 0x50, 0x4C, 0x54, 0x45, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xA5, 0xD9, 0x9F, 0xDD, 0x00, 0x00, 0x00, 0x01, 0x74, 0x52, 0x4E, 0x53, 0x00, 0x40, 0xE6, 0xD8, 0x66, 0x00, 0x00, 0x00, 0x4F, 0x49, 0x44, 0x41, 0x54, 0x08, 0xD7, 0x63, 0x28, 0x94, 0x79, 0x58, 0x7B, 0xBF, 0x78, 0xEE, 0xF3, 0xEA, 0xFF, 0x0C, 0xDD, 0xCA, 0xC2, 0x4E, 0x8C, 0x3D, 0xC9, 0x12, 0xC7, 0x04, 0x18, 0xE6, 0x32, 0x89, 0x56, 0x1F, 0x02, 0x32, 0xDD, 0x04, 0x18, 0x56, 0xB2, 0x64, 0xB2, 0x29, 0x15, 0xFF, 0x7F, 0xE1, 0x7E, 0x8F, 0xE1, 0x24, 0x87, 0x7C, 0x9B, 0x4A, 0x07, 0x58, 0xB4, 0x53, 0x50, 0xD0, 0x0D, 0xC4, 0x04, 0xAA, 0x2D, 0xB4, 0x7B, 0x68, 0x79, 0xA4, 0x78, 0xF1, 0xF3, 0xEA, 0x0F, 0x00, 0x5F, 0x2A, 0x1C, 0xFE, 0x51, 0xD4, 0xC9, 0xA3, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
};

class TLFontFace : public Resource {
	GODOT_CLASS(TLFontFace, Resource);

protected:
	//Ref<TLFontFace> fallback;
	static Ref<ImageTexture> hex_box_img_tex;
	String path;

public:
	TLFontFace();
	virtual ~TLFontFace();

	void _init();

	//Internal methods
	static void initialize_hex_font();
	static void finish_hex_font();

	virtual hb_font_t *get_hb_font(int p_size) const;
	virtual float get_glyph_scale(int p_size) const;

	static void draw_hexbox(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate);
	static void _draw_small_int(RID p_canvas_item, const Point2 p_pos, uint32_t p_int, const Color p_modulate);

	virtual void draw_glyph(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate, int p_size) const;
	virtual void _draw_char(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate, int p_size) const; //raw char for debug only, do not use
	virtual void draw_glyph_outline(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate, int p_size) const;
	virtual Array get_glyph_outline(const Point2 p_pos, uint32_t p_codepoint, int p_size) const;

	Array _unicode_scripts_supported() const;
	virtual std::vector<hb_script_t> unicode_scripts_supported() const;

	//GDNative methods
	virtual bool load(String p_resource_path);

	virtual void set_font_path(String p_resource_path);
	virtual String get_font_path() const;

	virtual double get_ascent(int p_size) const;
	virtual double get_descent(int p_size) const;
	virtual double get_height(int p_size) const;

	virtual int get_base_size() const;

#ifdef GODOT_MODULE
	static void _bind_methods();
#else
	static void _register_methods();
#endif
};

#endif /*TL_FONT_FACE_HPP*/
