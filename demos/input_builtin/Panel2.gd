extends ColorRect

export var point = Vector2()
export var brk = false
export var jst = false

func _draw():
	var tpc = get_parent().get_parent().get_node("TLRichTextEdit")
	tpc.debug_draw(get_canvas_item(), Vector2(70, 35), point, brk, jst)
	tpc.debug_draw_as_hex(get_canvas_item(), Vector2(70, 105), point, brk, jst)
	tpc.debug_draw_logical_as_hex(get_canvas_item(), Vector2(70, 140), point, brk, jst)
