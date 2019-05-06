/*************************************************************************/
/*  tl_bitmap_font.hpp                                                   */
/*************************************************************************/

#ifndef TL_BITMAP_FONT_FACE_HPP
#define TL_BITMAP_FONT_FACE_HPP

#include "tl_font.hpp"

#include <cmath>
#include <map>
#include <vector>

using namespace godot;

class TLBitmapFontFace;

struct TLBitmapFontFaceAtSize {

	const TLBitmapFontFace *font;
	hb_font_t *hb_font;
	int size;

	TLBitmapFontFaceAtSize();
	~TLBitmapFontFaceAtSize();
};

class TLBitmapFontFace : public TLFontFace {
	GODOT_SUBCLASS(TLBitmapFontFace, TLFontFace);

protected:
	bool loaded;

	float ascent;
	float descent;
	float height;

	int bmp_size;

	struct Glyph {
		Point2 align;
		Rect2 uv;
		size_t id;
		float advance;
	};

	struct KerningPairKey {
		union {
			struct {
				uint32_t A, B;
			};
			uint64_t pair;
		};
		_ALWAYS_INLINE_ bool operator<(const KerningPairKey &p_r) const { return pair < p_r.pair; }
	};

	void clear_cache();

	std::vector<Ref<Texture> > texture_cache;
	std::map<uint32_t, Glyph> glyph_cache;
	std::map<KerningPairKey, int> kerning_map;

	int txt_flags;

	mutable std::map<int, TLBitmapFontFaceAtSize *> sizes;

public:
	TLBitmapFontFace();
	virtual ~TLBitmapFontFace();

	void _init();

	virtual hb_font_t *get_hb_font(int p_size) const override;

	virtual void _draw_char(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate, int p_size) const override; //raw char for debug only, do not use
	virtual void draw_glyph(RID p_canvas_item, const Point2 p_pos, uint32_t p_codepoint, const Color p_modulate, int p_size) const override;

	virtual std::vector<hb_script_t> unicode_scripts_supported() const;

	//GDNative methods
	virtual bool load(String p_resource_path) override;

	virtual void set_font_path(String p_resource_path) override;

	virtual double get_ascent(int p_size) const override;
	virtual double get_descent(int p_size) const override;
	virtual double get_height(int p_size) const override;

	virtual int get_base_size() const override;

	virtual void set_texture_flags(int p_flags) override;
	virtual int get_texture_flags() const override;

	//HB internals
	bool has_glyph(uint32_t p_codepoint) const;
	float get_glyph_advance(uint32_t p_codepoint, int p_size) const;
	Point2 get_glyph_align(uint32_t p_codepoint, int p_size) const;
	Size2 get_glyph_size(uint32_t p_codepoint, int p_size) const;
	float get_kerning_pair(uint32_t p_codepoint_l, uint32_t p_codepoint_r, int p_size) const;
};

HB_BEGIN_DECLS
HB_EXTERN hb_font_t *hb_bmp_font_create(TLBitmapFontFaceAtSize *bm_face, hb_destroy_func_t destroy);
HB_END_DECLS

#endif /*TL_BITMAP_FONT_FACE_HPP*/
