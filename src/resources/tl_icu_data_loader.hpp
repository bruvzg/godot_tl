/*************************************************************************/
/*  tl_icu_data_loader.hpp                                               */
/*************************************************************************/

#ifndef TL_ICU_DATA_LOADER_H
#define TL_ICU_DATA_LOADER_H

#include "tl_core.hpp"

#ifdef GODOT_MODULE
#include "core/resource.h"
#endif

#include <unicode/uclean.h>
#include <unicode/udata.h>

using namespace godot;

class TLICUDataLoader : public Resource {
	GODOT_CLASS(TLICUDataLoader, Resource);

	static uint8_t *icu_data;

public:
	TLICUDataLoader();
	~TLICUDataLoader();

	void _init();

	bool load(String p_path);
	static void unload();

#ifdef GODOT_MODULE
	static void _bind_methods();
#else
	static void _register_methods();
#endif
};

#endif /*TL_ICU_DATA_LOADER*/
