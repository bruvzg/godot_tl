/*************************************************************************/
/*  tl_icu_data_loader.cpp                                               */
/*************************************************************************/

#include "resources/tl_icu_data_loader.hpp"

#ifdef GODOT_MODULE
#include "core/bind/core_bind.h"
#define File _File
#else
#include <ArrayMesh.hpp>
#include <File.hpp>
#include <Mesh.hpp>
#include <SurfaceTool.hpp>
#endif

uint8_t *TLICUDataLoader::icu_data = NULL;

TLICUDataLoader::TLICUDataLoader() {

#ifdef GODOT_MODULE
	_init();
#endif
}

void TLICUDataLoader::_init() {

	//NOP
}

TLICUDataLoader::~TLICUDataLoader() {

	//NOP
}

bool TLICUDataLoader::load(String p_path) {

	if (!icu_data) {
		Ref<File> file;
		file.instance();
		if (file->open(p_path, File::READ) != Error::OK) {
			ERR_PRINTS("Can't open ICU data file: \"" + p_path + "\"");
			return false;
		}

		UErrorCode err = U_ZERO_ERROR;

		//ICU data found
		size_t len = file->get_len();
		PoolByteArray icu_pool = file->get_buffer(len);
		file->close();

		icu_data = (uint8_t *)std::malloc(len);
		if (!icu_data)
			return false;
		std::memcpy(icu_data, icu_pool.read().ptr(), len);

		udata_setCommonData(icu_data, &err);
		if (U_FAILURE(err)) {
			ERR_PRINTS(u_errorName(err));
			std::free(icu_data);
			icu_data = NULL;
			return false;
		}

		err = U_ZERO_ERROR;
		u_init(&err);
		if (U_FAILURE(err)) {
			ERR_PRINTS(u_errorName(err));
			std::free(icu_data);
			icu_data = NULL;
			return false;
		}
	}

	return true;
}

void TLICUDataLoader::unload() {

	if (icu_data) {
		std::free(icu_data);
		icu_data = NULL;
	}
}

#ifdef GODOT_MODULE
void TLICUDataLoader::_bind_methods() {

	ClassDB::bind_method(D_METHOD("load", "resource_path"), &TLICUDataLoader::load);
}
#else
void TLICUDataLoader::_register_methods() {

	register_method("load", &TLICUDataLoader::load);
}
#endif
