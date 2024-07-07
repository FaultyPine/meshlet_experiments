#include "lines_renderer.h"


#ifdef TARGET_WEB
#include "shaders/generated/wgsl/lines_shdc.h"
#elif defined(TARGET_DESKTOP)
#include "shaders/generated/glsl430/lines_shdc.h"
#endif

#define INITIAL_LINES_BUFFERSIZE 1000

LinesRenderer InitLinesRenderer()
{
    LinesRenderer renderer = {};
    sg_buffer_desc buf_desc = 
    {
        .size = INITIAL_LINES_BUFFERSIZE * sizeof(hmm_vec3) * 2, // 2 = num vert attribs
        .type = SG_BUFFERTYPE_VERTEXBUFFER,
        .usage = SG_USAGE_STREAM,
    };
    renderer.vert_buf = sg_make_buffer(&buf_desc);
    renderer.bindings =
    {
        .vertex_buffers[0] = renderer.vert_buf,
    };
    sg_pipeline_desc pipeline_desc =
    {
        .shader = sg_make_shader(lines_shader_desc(sg_query_backend())),
        .layout = 
        {
            .attrs[0] = { .format = SG_VERTEXFORMAT_FLOAT3, },
            .attrs[1] = { .format = SG_VERTEXFORMAT_FLOAT3, },
        },
        .primitive_type = SG_PRIMITIVETYPE_LINES,
    };
    renderer.pip = sg_make_pipeline(&pipeline_desc);
    return renderer;
}
void PushLine(LinesRenderer& renderer, const float a[3], const float b[3], const float color[3])
{
    renderer.points.push_back(HMM_Vec3(a[0], a[1], a[2]));
    renderer.points.push_back(HMM_Vec3(color[0], color[1], color[2]));
    renderer.points.push_back(HMM_Vec3(b[0], b[1], b[2]));
    renderer.points.push_back(HMM_Vec3(color[0], color[1], color[2]));
}
void FlushLinesRenderer(LinesRenderer& renderer, hmm_mat4 mvp)
{
    if (!renderer.points.empty())
    {
        // update gpu buf
        sg_range buf_rng = {.ptr = renderer.points.data(), .size = renderer.points.size() * sizeof(hmm_vec3)};
        sg_update_buffer(renderer.vert_buf, buf_rng);
        // draw
        sg_apply_pipeline(renderer.pip);
        sg_apply_bindings(&renderer.bindings);
        struct vs_params { hmm_mat4 mvp; };
        vs_params vs_params = {.mvp = mvp};
        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, SG_RANGE_REF(vs_params));
        SOKOL_ASSERT(renderer.points.size() % 3 == 0);
        sg_draw(0, renderer.points.size(), 1);
        // clear for next frame
        renderer.points.clear();
    }
}