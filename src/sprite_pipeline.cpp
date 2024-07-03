#include "sprite_pipeline.h"

#include "common.h"

#ifdef TARGET_WEB
#include "shaders/generated/wgsl/triangle_shdc.h"
#elif defined(TARGET_DESKTOP)
#include "shaders/generated/glsl430/triangle_shdc.h"
#endif

GraphicsPipelineState init_sprite_pipeline()
{
    GraphicsPipelineState sprite_pipeline_state = {};
    float tex_quad[] = 
    { 
        // top left is 0,0  bottom right is 1,1
        // pos      // tex
        -1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 

        -1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 0.0f
    };

    sg_image_desc img_desc = {0};
    #define IMG_WIDTH 256
    #define IMG_HEIGHT 256
    img_desc.height = IMG_HEIGHT;
    img_desc.width = IMG_WIDTH;

    uint32_t img_data[IMG_WIDTH * IMG_HEIGHT];

    for (int j = 0; j < IMG_HEIGHT; j++) 
    {
        for (int i = 0; i < IMG_WIDTH; i++) 
        {
            uint32_t r = (uint32_t)(((float)i / (IMG_WIDTH-1)) * 255.0);
            uint32_t g = (uint32_t)(((float)j / (IMG_HEIGHT-1)) * 255.0);
            uint32_t b = 0;
            uint32_t a = 0xFF;
            uint32_t col = (r << (8*3)) | (b << (8*2)) | (g << (8*1)) | a; // RBGA

            int col_idx = j * IMG_WIDTH + i;
            img_data[col_idx] = col;
        }
    }
    img_desc.data.subimage[0][0] = SG_RANGE(img_data);
    sg_image img = sg_make_image(&img_desc);
    sg_sampler_desc smp_desc = {0};
    sg_sampler smp = sg_make_sampler(&smp_desc);

    sg_buffer_desc vertex_buffer_desc = 
    {
        .data = SG_RANGE(tex_quad),
    };
    sprite_pipeline_state.bind = (sg_bindings)
    {
        .vertex_buffers[0] = sg_make_buffer(&vertex_buffer_desc),
        .fs = 
        {
            .images[SLOT_tex] = img,
            .samplers[SLOT_smp] = smp,
        }
    };
    
    sg_pipeline_desc sprite_pipeline_desc = 
    {
        .shader = sg_make_shader(triangle_shader_desc(sg_query_backend())),
        .layout = 
        {
            .attrs = 
            {
                [ATTR_vs_position].format = SG_VERTEXFORMAT_FLOAT2,
                [ATTR_vs_texcoord].format = SG_VERTEXFORMAT_FLOAT2
            },
        },
    };
    sprite_pipeline_state.pip = sg_make_pipeline(&sprite_pipeline_desc);
    return sprite_pipeline_state;
}
