extends ColorRect

export var point = Vector2()
export var brk = false
export var jst = false

func _draw():
	var tpc = get_parent().get_node("TLProtoControl")
	tpc.debug_draw(get_canvas_item(), Vector2(70, rect_size.y / 5), point, brk, jst)
	tpc.debug_draw_as_hex(get_canvas_item(), Vector2(70, 3 * rect_size.y / 5), point, brk, jst)
	tpc.debug_draw_logical_as_hex(get_canvas_item(), Vector2(70, 4 * rect_size.y / 5), point, brk, jst)
