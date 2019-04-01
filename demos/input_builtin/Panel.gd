extends Panel

export var gldata = []
var scale = 3

func _draw():
	draw_rect(Rect2(Vector2(), rect_size), Color(0, 0, 0))
	var x = Vector2(10, rect_size.y / 2)
	if gldata.size() > 0:
		var s = 1.0 / (gldata.size() / 7)
		var n = s * (rect_size.x - 20)
		for i in range(gldata.size() / 7):
			var face = gldata[i*7 + 0]
			var fsize = gldata[i*7 + 1]
			var asc = gldata[i*7 + 2] * scale
			var dsc = gldata[i*7 + 3] * scale
			var off = gldata[i*7 + 5] * scale
	
			face.draw_glyph(get_canvas_item(), x + off - Vector2(0, asc), gldata[i*7 + 4], Color(1 - (s*i), 1 - (s*i), 1 - (s*i), 0.5), fsize * scale)
	
			draw_circle(x + off, 3, Color(1, 0, 0, 0.45))
			draw_line(x + off, x, Color(1, 0, 0))
			draw_line(Vector2(0, rect_size.y / 2 - asc), Vector2(rect_size.x, rect_size.y / 2 - asc), Color(0, 1, 0, 0.2))
			draw_line(Vector2(0, rect_size.y / 2), Vector2(rect_size.x, rect_size.y / 2), Color(0, 1, 0, 0.2))
			draw_line(Vector2(0, rect_size.y / 2 + dsc), Vector2(rect_size.x, rect_size.y / 2 + dsc), Color(0, 1, 0, 0.2))
			draw_circle(x, 3, Color(0, 0, 1, 0.45))
			draw_line(x + gldata[i*7 + 6] * scale, x, Color(0, 0, 1))
			x = x + gldata[i*7 + 6] * scale
			draw_string(get_font("font"), Vector2(10 + n * i, rect_size.y - 34), "%d" % [gldata[i*7 + 4]], Color(1, 1, 1))
			draw_string(get_font("font"), Vector2(10 + n * i, rect_size.y - 22), "%2.1f; %2.1f" % [gldata[i*7 + 6].x, gldata[i*7 + 6].y], Color(0, 0, 1))
			draw_string(get_font("font"), Vector2(10 + n * i, rect_size.y - 10), "%2.1f; %2.1f" % [gldata[i*7 + 5].x, gldata[i*7 + 5].y], Color(1, 0, 0))
			face.draw_glyph(get_canvas_item(), Vector2(10 + n * i, rect_size.y - 80 - dsc / scale), gldata[i*7 + 4], Color(1 - (s*i), 1 - (s*i), 1 - (s*i), 0.5), fsize)

func _on_HSlider_value_changed(value):
	scale = value
	update()
