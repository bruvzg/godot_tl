/*************************************************************************/
/*  tl_core.hpp                                                          */
/*************************************************************************/

#ifndef CORE_HPP
#define CORE_HPP

#include <cmath>
#include <cstdlib>
#include <cstring>

//#define DEBUG_DISPLAY_METRICS
//#define DEBUG_PRINT_RUNS

#ifdef GODOT_MODULE

namespace godot {};

#define GODOT_CLASS GDCLASS
#define GODOT_SUBCLASS GDCLASS
#define GLOBAL_CONST(m_text) m_text

#else

#include <nativescript/godot_nativescript.h>
#include <GlobalConstants.hpp>
#include <Godot.hpp>
#include <OS.hpp>
#include <Reference.hpp>
#include <Resource.hpp>

#define GLOBAL_CONST(m_text) GlobalConstants::m_text
#define memnew(type) type::_new()
#define memdelete(obj) obj->free()

#ifndef MAX
#define MAX(a, b) (a > b ? a : b)
#endif

#ifndef MIN
#define MIN(a, b) (a < b ? a : b)
#endif

#ifdef WARN_PRINTS
#undef WARN_PRINTS
#endif

#define WARN_PRINTS(m_text) godot::api->godot_print_warning(String(m_text).utf8().get_data(), __FUNCTION__, __FILE__, __LINE__);

#ifdef ERR_PRINTS
#undef ERR_PRINTS
#endif

#define ERR_PRINTS(m_text) godot::api->godot_print_error(String(m_text).utf8().get_data(), __FUNCTION__, __FILE__, __LINE__);

#ifndef _ALWAYS_INLINE_
#if defined(__GNUC__) && (__GNUC__ >= 4)
#define _ALWAYS_INLINE_ __attribute__((always_inline)) inline
#elif defined(__llvm__)
#define _ALWAYS_INLINE_ __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define _ALWAYS_INLINE_ __forceinline
#else
#define _ALWAYS_INLINE_ inline
#endif
#endif

#endif

#endif /*CORE_HPP*/
