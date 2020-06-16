/*************************************************************************/
/*  tl_editor_node.h                                                     */
/*************************************************************************/

#ifndef TL_EDITOR_NODE_H
#define TL_EDITOR_NODE_H

#include <core/map.h>
#include <core/resource.h>
#include <editor/editor_export.h>
#include <editor/editor_node.h>

#include "tl_core.hpp"

class TLEditorNode : public Node {
	GODOT_SUBCLASS(TLEditorNode, Object)

	EditorNode *editor;

	Map<String, Ref<ImageTexture>> type_icons;

	static TLEditorNode *singleton;

protected:
	void _notification(int p_notification);
	static void _bind_methods();

public:
	_FORCE_INLINE_ static TLEditorNode *get_singleton() { return singleton; }

	TLEditorNode(EditorNode *p_editor);
	~TLEditorNode();
};

#endif //TL_EDITOR_NODE_H
