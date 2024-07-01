
#include "common.h"

#define SOKOL_IMPL
#include "external/sokol/sokol_app.h"
#include "external/sokol/sokol_log.h"
#include "external/sokol/sokol_gfx.h"
#include "external/sokol/sokol_glue.h"
#include "external/sokol/sokol_time.h"
#include "external/sokol/sokol_fetch.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "external/imgui/imgui.h"

// imgui source
#include "external/imgui/imgui_draw.cpp"
#include "external/imgui/imgui_tables.cpp"
#include "external/imgui/imgui_widgets.cpp"
#include "external/imgui/imgui.cpp"
// 
#include "external/sokol/sokol_imgui.h"
#undef SOKOL_IMPL


#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/tinygltf/tiny_gltf.h"


#define HANDMADE_MATH_IMPLEMENTATION
#include "external/HandmadeMath.h"

#include "external/meshoptimizer/src/clusterizer.cpp"
