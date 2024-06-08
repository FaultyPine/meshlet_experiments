#pragma once

#ifdef _MSC_VER
#include <intrin.h>
#define DEBUG_BREAK __debugbreak()
#else
#define DEBUG_BREAK __builtin_trap()
#endif

#define SOKOL_IMPL
#undef SOKOL_ASSERT
#ifdef _DEBUG
#include <stdio.h>
#define SOKOL_ASSERT(x) if (!(x)) { printf("%s | %s:%i", #x, __FILE__, __LINE__); DEBUG_BREAK; }
#else
#define SOKOL_ASSERT(x)
#endif
#include "external/sokol/sokol_app.h"
#include "external/sokol/sokol_log.h"
#include "external/sokol/sokol_gfx.h"
#include "external/sokol/sokol_glue.h"
#include "external/sokol/sokol_time.h"

