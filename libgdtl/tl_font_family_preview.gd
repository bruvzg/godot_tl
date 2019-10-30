tool
extends VBoxContainer

const TLShapedString = preload("res://addons/libgdtl/classes/tl_shaped_string.gdns")

var preview
var strctx
var ctl

func _ff_changed(p_text):
	strctx.set_text(p_text)
	preview.update()

func _redraw():
	VisualServer.canvas_item_add_rect(preview.get_canvas_item(), Rect2(Vector2(), preview.get_size()), Color(0.8, 0.8, 0.8, 1))
	strctx.draw(preview.get_canvas_item(), Vector2((preview.get_rect().size.x - strctx.get_width()) / 2, (preview.get_rect().size.y - strctx.get_height()) / 2 + strctx.get_ascent()), Color(0, 0, 0, 1))

func set_ff(p_ff, p_style):
	strctx.set_base_font(p_ff);
	strctx.set_base_font_style(p_style);

func _init():
	strctx = TLShapedString.new()
	strctx.set_base_font_size(18.0)
	strctx.set_text("Etaoin shrdlu")
	preview = Control.new();
	preview.set_custom_minimum_size(Vector2(0, 50))
	preview.connect("draw", self, "_redraw")
	add_child(preview)
	ctl = LineEdit.new()
	ctl.set_text("Etaoin shrdlu")
	ctl.connect("text_changed", self, "_ff_changed")
	add_child(ctl)
