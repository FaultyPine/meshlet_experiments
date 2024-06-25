#pragma once

#include "external/sokol/sokol_gfx.h"

struct sprite_pipeline_state 
{
    sg_pipeline pip;
    sg_bindings bind;
};

sprite_pipeline_state init_sprite_pipeline();
