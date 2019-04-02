tool
extends EditorPlugin

func _enter_tree():
    add_custom_type("TLICUDataLoader", "Resource", preload("res://addons/libgdtl/classes/tl_icu_data_loader.gdns"), preload("res://addons/libgdtl/icons/icon_t_l_i_c_u_data_loader.svg"))
    add_custom_type("TLFontFace", "Resource", preload("res://addons/libgdtl/classes/tl_font_face.gdns"), preload("res://addons/libgdtl/icons/icon_t_l_font_face.svg"))
    add_custom_type("TLBitmapFontFace", "TLFontFace", preload("res://addons/libgdtl/classes/tl_bitmap_font_face.gdns"), preload("res://addons/libgdtl/icons/icon_t_l_bitmap_font_face.svg"))
    add_custom_type("TLDynamicFontFace", "TLFontFace", preload("res://addons/libgdtl/classes/tl_dynamic_font_face.gdns"), preload("res://addons/libgdtl/icons/icon_t_l_dynamic_font_face.svg"))
    add_custom_type("TLFontFamily", "Resource", preload("res://addons/libgdtl/classes/tl_font_family.gdns"), preload("res://addons/libgdtl/icons/icon_t_l_font_family.svg"))
    add_custom_type("TLShapedString", "Resource", preload("res://addons/libgdtl/classes/tl_shaped_string.gdns"), preload("res://addons/libgdtl/icons/icon_t_l_shaped_string.svg"))
    add_custom_type("TLShapedAttributedString", "TLShapedString", preload("res://addons/libgdtl/classes/tl_shaped_attributed_string.gdns"), preload("res://addons/libgdtl/icons/icon_t_l_shaped_attributed_string.svg"))
    add_custom_type("TLShapedParagraph", "Resource", preload("res://addons/libgdtl/classes/tl_shaped_paragraph.gdns"), preload("res://addons/libgdtl/icons/icon_t_l_shaped_paragraph.svg"))
    add_custom_type("TLProtoControl", "Control", preload("res://addons/libgdtl/classes/tl_proto_control.gdns"), preload("res://addons/libgdtl/icons/icon_t_l_proto_control.svg"))
    add_custom_type("TLProtoControlSelection", "Reference", preload("res://addons/libgdtl/classes/tl_proto_control_selection.gdns"), preload("res://addons/libgdtl/icons/icon_t_l_proto_control_selection.svg"))
    add_autoload_singleton("TLConstants", "res://addons/libgdtl/constants.gd")

func _exit_tree():
    remove_custom_type("TLICUDataLoader")
    remove_custom_type("TLFontFace")
    remove_custom_type("TLBitmapFontFace")
    remove_custom_type("TLDynamicFontFace")
    remove_custom_type("TLFontFamily")
    remove_custom_type("TLShapedString")
    remove_custom_type("TLShapedAttributedString")
    remove_custom_type("TLShapedParagraph")
    remove_custom_type("TLProtoControl")
    remove_custom_type("TLProtoControlSelection")