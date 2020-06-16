/*************************************************************************/
/*  tl_editor_node.cpp                                                   */
/*************************************************************************/

#include <core/os/keyboard.h>
#include <modules/svg/image_loader_svg.h>

#include "icons/icons.gen.h"
#include "tl_editor_node.h"
#include "tl_font_family_edit.hpp"

TLEditorNode *TLEditorNode::singleton = NULL;

void TLEditorNode::_notification(int p_notification) {
	switch (p_notification) {
		case Node::NOTIFICATION_ENTER_TREE:
		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
			//Register node type icons
			for (Map<String, Ref<ImageTexture>>::Element *E = type_icons.front(); E; E = E->next()) {
				editor->get_theme_base()->get_theme()->set_icon(E->key(), "EditorIcons", E->get());
			}
		}
	}
};

void TLEditorNode::_bind_methods(){
	//NOP
};

TLEditorNode::TLEditorNode(EditorNode *p_editor) {
	singleton = this;

	editor = p_editor;

	//Init icons
	for (int i = 0; i < tl_icons_count; i++) {
		Ref<ImageTexture> icon = memnew(ImageTexture);
		Ref<Image> img = memnew(Image);

		//No dark theme invert
		ImageLoaderSVG::create_image_from_string(img, tl_icons_sources[i], EDSCALE, true, false);
		icon->create_from_image(img);
		if (String(tl_icons_names[i]).begins_with("TL")) {
			type_icons[tl_icons_names[i]] = icon; //TL*
		}
	}

	Ref<EditorInspectorPluginTLFontFamily> inspector_plugin;
	inspector_plugin.instance();

	EditorInspector::add_inspector_plugin(inspector_plugin);
};

TLEditorNode::~TLEditorNode() {
	singleton = NULL;
};
