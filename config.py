from SCons.Script import BoolVariable, Dir, Environment, File, SCons, Variables

def can_build(env, platform):
    return True

def configure(env):
    envvars = Variables()
    envvars.Add(BoolVariable('builtin_runtime', 'Use the built-in libraries', True))
    envvars.Add(BoolVariable('use_graphite2', 'Enable Graphite2 complementary shaper', True))
    envvars.Add(BoolVariable('use_font_wrapper', 'Enable Godot font wrapper', False))
    envvars.Update(env)

def get_doc_classes():
    return [
            "TLICUDataLoader",
            "TLFontFace",
            "TLBitmapFontFace",
            "TLDynamicFontFace",
            "TLFontFamily",
            "TLShapedString",
            "TLShapedAttributedString",
            "TLShapedParagraph",
            "TLRichTextEdit",
            "TLRichTextEditSelection",
            "TLLabel",
            "TLLineEdit",
            "TLFontIterator",
            "TLGDFontWrapper"
    ]

def get_doc_path():
    return "doc_classes"
