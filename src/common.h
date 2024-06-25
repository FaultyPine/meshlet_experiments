#pragma once

#ifdef _MSC_VER
#include <intrin.h>
#define DEBUG_BREAK __debugbreak()
#else
#define DEBUG_BREAK __builtin_trap()
#endif

#ifdef TARGET_WEB
#define SOKOL_GLES3
#elif defined(TARGET_DESKTOP)
#define SOKOL_GLCORE
#else
#error "Unsupported target right now..."
#endif

#undef SOKOL_ASSERT
#ifdef _DEBUG
#include <stdio.h>
#define SOKOL_ASSERT(x) if (!(x)) { printf("%s | %s:%i", #x, __FILE__, __LINE__); DEBUG_BREAK; }
#else
#define SOKOL_ASSERT(x)
#endif


