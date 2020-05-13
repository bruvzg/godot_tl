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
			ERR_PRINT("Can't open ICU data file: \"" + p_path + "\"");
			return false;
		}

		UErrorCode err = U_ZERO_ERROR;

		//ICU data found
		size_t len = file->get_len();
		PackedByteArray icu_pool = file->get_buffer(len);
		file->close();

		icu_data = (uint8_t *)std::malloc(len);
		if (!icu_data)
			return false;
		std::memcpy(icu_data, icu_pool.ptr(), len);

		udata_setCommonData(icu_data, &err);
		if (U_FAILURE(err)) {
			ERR_PRINT(u_errorName(err));
			std::free(icu_data);
			icu_data = NULL;
			return false;
		}

		err = U_ZERO_ERROR;
		u_init(&err);
		if (U_FAILURE(err)) {
			ERR_PRINT(u_errorName(err));
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

void TLICUDataLoader::set_data_path(String p_resource_path) {

	path = p_resource_path;
	load(path);
}

String TLICUDataLoader::get_data_path() const {

	return path;
}

#ifdef GODOT_MODULE
void TLICUDataLoader::_bind_methods() {

	ClassDB::bind_method(D_METHOD("load", "resource_path"), &TLICUDataLoader::load);
	ClassDB::bind_method(D_METHOD("get_data_path"), &TLICUDataLoader::get_data_path);
	ClassDB::bind_method(D_METHOD("set_data_path", "path"), &TLICUDataLoader::set_data_path);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "data_path", PROPERTY_HINT_FILE, "*.dat"), "set_data_path", "get_data_path");
}
#else
void TLICUDataLoader::_register_methods() {

	register_method("load", &TLICUDataLoader::load);
	register_method("get_data_path", &TLICUDataLoader::get_data_path);
	register_method("set_data_path", &TLICUDataLoader::set_data_path);

	register_property<TLICUDataLoader, String>("data_path", &TLICUDataLoader::set_data_path, &TLICUDataLoader::get_data_path, String(), GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_FILE, String("*.dat"));
}
#endif
