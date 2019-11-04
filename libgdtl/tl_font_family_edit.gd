tool
extends EditorInspectorPlugin

const TLFontFamily = preload("res://addons/libgdtl/classes/tl_font_family.gdns")
const TLFontFamilyPreview = preload("res://addons/libgdtl/tl_font_family_preview.gd")

func can_handle(object):
	var scr = object.get_script()
	if (scr && scr.is_class("NativeScript") && (scr.get_class_name() == "TLFontFamily" || scr.get_class_name() == "TLShapedAttributedString")):
		return true

func parse_begin(object):
	pass

func parse_category(object, category):
	pass

func parse_end():
	pass

func _new_style(p_object, p_ctl):
	if (p_object && p_ctl && p_ctl.get_text() != ""):
		p_object.add_style(p_ctl.get_text())

func _remove_style(p_object, p_style):
	if (p_object):
		p_object.remove_style(p_style)

func _new_lang(p_object, p_style, p_ctl):
	if (p_object && p_ctl && p_ctl.get_text() != ""):
		p_object.add_language(p_style, p_ctl.get_text())

func _new_script(p_object, p_style, p_ctl):
	if (p_object && p_ctl && p_ctl.get_text() != ""):
		p_object.add_script(p_style, p_ctl.get_text())

func _remove_lang(p_object, p_style, p_lang):
	if (p_object):
		p_object.remove_language(p_style, p_lang)

func _remove_script(p_object, p_style, p_script):
	if (p_object):
		p_object.remove_script(p_style, p_script)

func _clear(p_object):
	if (p_object):
		p_object.clear_attributes()

func _commit(p_object):
	if (p_object):
		p_object.commit_attribute()

func _reject(p_object):
	if (p_object):
		p_object.reject_attribute()

func parse_property(p_object, p_type, p_path, p_hint, p_hint_text, p_usage):
	if (p_path == "attribute/_commit"):
		var hbox = HBoxContainer.new()
		var rem_btn = Button.new()
		rem_btn.set_h_size_flags(Control.SIZE_EXPAND_FILL)
		rem_btn.connect("pressed", self, "_clear", [p_object])
		rem_btn.set_text("Clear")
		hbox.add_child(rem_btn)
		var cln_btn = Button.new()
		cln_btn.set_h_size_flags(Control.SIZE_EXPAND_FILL)
		cln_btn.connect("pressed", self, "_reject", [p_object])
		cln_btn.set_text("Remove")
		hbox.add_child(cln_btn)
		var new_btn = Button.new()
		new_btn.set_h_size_flags(Control.SIZE_EXPAND_FILL)
		new_btn.connect("pressed", self, "_commit", [p_object])
		new_btn.set_text("Add")
		hbox.add_child(new_btn)
		add_custom_control(hbox)
		return true
	if (p_path == "_new_style"):
		var hbox = HBoxContainer.new()
		var new_name = LineEdit.new()
		new_name.set_h_size_flags(Control.SIZE_EXPAND_FILL)
		new_name.set_placeholder("Style name")
		hbox.add_child(new_name)
		var new_btn = Button.new()
		new_btn.set_h_size_flags(Control.SIZE_EXPAND_FILL)
		new_btn.connect("pressed", self, "_new_style", [p_object, new_name])
		new_btn.set_text("Add style")
		hbox.add_child(new_btn)
		add_custom_control(hbox)
		return true
	var tokens = p_path.split("/")
	if (tokens.size() == 2):
		if (tokens[1] == "_prev_style"):
			var prev = TLFontFamilyPreview.new()
			prev.set_ff(p_object, tokens[0])
			add_custom_control(prev)
			return true
		elif (tokens[1] == "_remove_style"):
			var rem_btn = Button.new()
			rem_btn.set_text("Remove \"" + tokens[0] + "\" style")
			rem_btn.connect("pressed", self, "_remove_style", [p_object, tokens[0]])
			add_custom_control(rem_btn)
			return true
		elif (tokens[1] == "_add_lang"):
			var hbox = HBoxContainer.new()
			var new_name = LineEdit.new()
			new_name.set_h_size_flags(Control.SIZE_EXPAND_FILL)
			new_name.set_max_length(4)
			new_name.set_placeholder("ISO language code")
			hbox.add_child(new_name)
			var new_btn = Button.new()
			new_btn.set_h_size_flags(Control.SIZE_EXPAND_FILL)
			new_btn.connect("pressed", self, "_new_lang", [p_object, tokens[0], new_name])
			new_btn.set_text("Add language")
			hbox.add_child(new_btn)
			add_custom_control(hbox)
			return true
		elif (tokens[1] == "_add_script"):
			var hbox = HBoxContainer.new()
			var new_name = LineEdit.new()
			new_name.set_h_size_flags(Control.SIZE_EXPAND_FILL)
			new_name.set_max_length(4)
			new_name.set_placeholder("ISO script code")
			hbox.add_child(new_name)
			var new_btn = Button.new()
			new_btn.set_h_size_flags(Control.SIZE_EXPAND_FILL)
			new_btn.connect("pressed", self, "_new_script", [p_object, tokens[0], new_name])
			new_btn.set_text("Add script")
			hbox.add_child(new_btn)
			add_custom_control(hbox)
			return true
	elif (tokens.size() == 4):
		if (tokens[3] == "_remove_script" && tokens[1] == "script"):
			var rem_btn = Button.new()
			rem_btn.set_text("Remove \"" + tokens[2] + "\" script")
			rem_btn.connect("pressed", self, "_remove_script", [p_object, tokens[0], tokens[2]])
			add_custom_control(rem_btn)
			return true
		elif (tokens[3] == "_remove_lang" && tokens[1] == "lang"):
			var rem_btn = Button.new()
			rem_btn.set_text("Remove \"" + tokens[2] + "\" language")
			rem_btn.connect("pressed", self, "_remove_lang", [p_object, tokens[0], tokens[2]])
			add_custom_control(rem_btn)
			return true
	return false