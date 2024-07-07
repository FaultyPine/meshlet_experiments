#pragma once

#include "common.h"
#include <vector>
#include "external/sokol/sokol_gfx.h"
#include "external/HandmadeMath.h"


struct LinesRenderer
{
    sg_pipeline pip = {};
    sg_bindings bindings = {};
    std::vector<hmm_vec3> points = {};
    sg_buffer vert_buf = {};
};

LinesRenderer InitLinesRenderer();
void PushLine(LinesRenderer& renderer, const float a[3], const float b[3], const float color[3]);
void FlushLinesRenderer(LinesRenderer& renderer, hmm_mat4 mvp);
