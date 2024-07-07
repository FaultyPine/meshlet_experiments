#pragma once


typedef unsigned char u8;
typedef char s8;
typedef unsigned short u16;
typedef short s16;
typedef unsigned int u32;
typedef int s32;
typedef long long s64;
typedef unsigned long long u64;
typedef float f32;
typedef double f64;


#ifdef _MSC_VER
#include <intrin.h>
#define DEBUG_BREAK __debugbreak()
#else
#define DEBUG_BREAK __builtin_trap()
#endif

#ifdef TARGET_WEB
#define SOKOL_WGPU
#elif defined(TARGET_DESKTOP)
#define SOKOL_GLCORE
#else
#error "Unsupported target right now..."
#endif

#undef SOKOL_ASSERT
#include <stdio.h>
#define SOKOL_ASSERT(x) if (!(x)) { printf("%s | %s:%i", #x, __FILE__, __LINE__); DEBUG_BREAK; }


#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) ( sizeof((arr))/sizeof((arr)[0]) )
#endif