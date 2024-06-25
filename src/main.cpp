

#include "common.h"

#define SOKOL_IMPL
#include "external/sokol/sokol_app.h"
#include "external/sokol/sokol_log.h"
#include "external/sokol/sokol_gfx.h"
#include "external/sokol/sokol_glue.h"
#include "external/sokol/sokol_time.h"
#include "external/sokol/sokol_fetch.h"
#undef SOKOL_IMPL

#ifdef TARGET_WEB
#include "emsc.h"
#include "shaders/generated/glsl300es/lit_model_shdc.h"
#elif defined(TARGET_DESKTOP)
#include "shaders/generated/glsl430/lit_model_shdc.h"
#endif

#define _CRT_SECURE_NO_WARNINGS.
#define TINYOBJLOADER_IMPLEMENTATION
#include "external/tinyobjloader.h"

#define HANDMADE_MATH_IMPLEMENTATION
#include "HandmadeMath.h"

#include "sprite_pipeline.h"

#include "log.h"

#include <iostream>

static sprite_pipeline_state sprite_pipeline = {};
static struct 
{
    sg_pipeline pip;
    sg_bindings bind;
} model_pipeline_state;

static hmm_vec2 window_dimensions = {1280, 720};

hmm_vec2 GetWindowDimensions()
{
    return window_dimensions;
}
float GetWindowWidth() { return window_dimensions.X; }
float GetWindowHeight() { return window_dimensions.Y; }

int tick(double time, void* userdata);

static int num_model_elements = 0; // tmp
void init_model_pipeline()
{
    #if 0
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = "C:/Dev/TestModels/"; // Path to material files

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile("C:/Dev/TestModels/cow.obj", reader_config)) {
        if (!reader.Error().empty()) 
        {
            LOG_INFO("TinyObjReader: %s", reader.Error().c_str());
        }
    }

    if (!reader.Warning().empty()) 
    {
        LOG_INFO("TinyObjReader: %s", reader.Warning().c_str());
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    const float* verts = attrib.vertices.data();
    const size_t verts_size = attrib.vertices.size() * sizeof(tinyobj::real_t);
    const float* normals = attrib.normals.data();
    const float* texcoords = attrib.texcoords.data();
    #endif
    LOG_INFO("INIT MODEL PIPELINE\n", 0);
     float vertices[] = {
        -1.0, -1.0, -1.0,   1.0, 0.0, 0.0, 1.0,
         1.0, -1.0, -1.0,   1.0, 0.0, 0.0, 1.0,
         1.0,  1.0, -1.0,   1.0, 0.0, 0.0, 1.0,
        -1.0,  1.0, -1.0,   1.0, 0.0, 0.0, 1.0,

        -1.0, -1.0,  1.0,   0.0, 1.0, 0.0, 1.0,
         1.0, -1.0,  1.0,   0.0, 1.0, 0.0, 1.0,
         1.0,  1.0,  1.0,   0.0, 1.0, 0.0, 1.0,
        -1.0,  1.0,  1.0,   0.0, 1.0, 0.0, 1.0,

        -1.0, -1.0, -1.0,   0.0, 0.0, 1.0, 1.0,
        -1.0,  1.0, -1.0,   0.0, 0.0, 1.0, 1.0,
        -1.0,  1.0,  1.0,   0.0, 0.0, 1.0, 1.0,
        -1.0, -1.0,  1.0,   0.0, 0.0, 1.0, 1.0,

        1.0, -1.0, -1.0,    1.0, 0.5, 0.0, 1.0,
        1.0,  1.0, -1.0,    1.0, 0.5, 0.0, 1.0,
        1.0,  1.0,  1.0,    1.0, 0.5, 0.0, 1.0,
        1.0, -1.0,  1.0,    1.0, 0.5, 0.0, 1.0,

        -1.0, -1.0, -1.0,   0.0, 0.5, 1.0, 1.0,
        -1.0, -1.0,  1.0,   0.0, 0.5, 1.0, 1.0,
         1.0, -1.0,  1.0,   0.0, 0.5, 1.0, 1.0,
         1.0, -1.0, -1.0,   0.0, 0.5, 1.0, 1.0,

        -1.0,  1.0, -1.0,   1.0, 0.0, 0.5, 1.0,
        -1.0,  1.0,  1.0,   1.0, 0.0, 0.5, 1.0,
         1.0,  1.0,  1.0,   1.0, 0.0, 0.5, 1.0,
         1.0,  1.0, -1.0,   1.0, 0.0, 0.5, 1.0
    };
   uint32_t indices[] = {
        0, 1, 2,  0, 2, 3,
        6, 5, 4,  7, 6, 4,
        8, 9, 10,  8, 10, 11,
        14, 13, 12,  15, 14, 12,
        16, 17, 18,  16, 18, 19,
        22, 21, 20,  23, 22, 20
    };

    // std::vector<int> indices = {};
    // for (uint32_t i = 0; i < shapes.size(); i++)
    // {
    //     const tinyobj::shape_t& shape = shapes.at(i);
    //     for (auto& idx_t : shape.mesh.indices)
    //     {
    //         int idx = idx_t.vertex_index;
    //         indices.push_back(idx);
    //     }
    // }
    num_model_elements = sizeof(indices)/sizeof(indices[0]);
    const size_t indices_size = num_model_elements * sizeof(int);

    sg_buffer_desc vertex_buf_desc = {
        .type = sg_buffer_type::SG_BUFFERTYPE_VERTEXBUFFER,
        // .data = {verts, verts_size},
        .data = SG_RANGE(vertices),
    };
    sg_buffer vertex_buf = sg_make_buffer(vertex_buf_desc);
    sg_buffer_desc index_buf_desc = {
        .type = sg_buffer_type::SG_BUFFERTYPE_INDEXBUFFER,
        // .data = {indices.data(), indices_size},
        .data = SG_RANGE(indices),
    };
    sg_buffer index_buf = sg_make_buffer(index_buf_desc);

    sg_bindings bindings =
    {
        .index_buffer = index_buf,
        .vertex_buffers[0] = vertex_buf,
    };
    sg_pipeline_desc pipeline_desc = 
    {
        .shader = sg_make_shader(lit_model_shader_desc(sg_query_backend())),
        .index_type = SG_INDEXTYPE_UINT32,
        .layout = 
        {
            // .attrs = 
            // {
            //     [ATTR_vs_position].format = sg_vertex_format::SG_VERTEXFORMAT_FLOAT3,
            //     [ATTR_vs_normal].format = sg_vertex_format::SG_VERTEXFORMAT_FLOAT3,
            //     [ATTR_vs_texcoord].format = sg_vertex_format::SG_VERTEXFORMAT_FLOAT2,
            // },
            .attrs = 
            {
                [0].format = sg_vertex_format::SG_VERTEXFORMAT_FLOAT3,
                [1].format = sg_vertex_format::SG_VERTEXFORMAT_FLOAT4,
            },
        },
        .depth = 
        {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
        },
        .cull_mode = SG_CULLMODE_BACK,
    };
    sg_pipeline pipeline = sg_make_pipeline(&pipeline_desc);
    model_pipeline_state.pip = pipeline;
    model_pipeline_state.bind = bindings;
}

static void init() 
{
    stm_setup();
    sfetch_desc_t sfetch_desc = { .logger.func = slog_func };
    sfetch_setup(&sfetch_desc);
    sg_environment env = {};
    #ifdef TARGET_WEB
    emsc_init("#canvas", EMSC_ANTIALIAS);
    env = emsc_environment();
    #elif defined(TARGET_DESKTOP)
    env = sglue_environment();
    #endif
    sg_desc appdesc = 
    {
        .environment = env,
        .logger.func = slog_func,
    };
    sg_setup(&appdesc);

    LOG_INFO("Setup complete\n", 1);

    init_model_pipeline();

    sprite_pipeline = init_sprite_pipeline();

    #ifdef TARGET_WEB
    // hand off control to browser loop
    emscripten_request_animation_frame_loop(tick, 0);
    #endif
}


void draw()
{
    sg_swapchain swapchain = {};
    #ifdef TARGET_WEB
    swapchain = emsc_swapchain();
    #elif defined(TARGET_DESKTOP)
    swapchain = sglue_swapchain();
    #endif
    sg_pass_action pass_action =
    {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value={0.0f, 0.0f, 0.0f, 1.0f } }
    };
    sg_pass pass = { .action = pass_action, .swapchain = swapchain };
    sg_begin_pass(&pass);

    // models
    hmm_mat4 proj = HMM_Perspective(60.0f, GetWindowWidth() / GetWindowHeight(), 0.01f, 100.0f);
    hmm_mat4 view = HMM_LookAt(HMM_Vec3(0.0f, 0.0f, 15.0f), HMM_Vec3(0.0f, 0.0f, 0.0f), HMM_Vec3(0.0f, 1.0f, 0.0f));
    
    // rotated model matrix
    static float ry = 0.0f;
    ry += 1.0f;
    hmm_mat4 rym = HMM_Rotate(ry, HMM_Vec3(0.0f, 1.0f, 0.0f));
    hmm_mat4 model = rym;
    hmm_mat4 mvp = HMM_MultiplyMat4(HMM_MultiplyMat4(proj, view), model);
    struct vs_params
    {
        hmm_mat4 mvp;
    };
    vs_params vs_params_instance = { .mvp = mvp };
    sg_apply_pipeline(model_pipeline_state.pip);
    sg_apply_bindings(&model_pipeline_state.bind);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, SG_RANGE_REF(vs_params_instance));
    sg_draw(0, num_model_elements, 1);

    // sprites
    const bool draw_sprites = true;
    if (draw_sprites)
    {
        sg_apply_pipeline(sprite_pipeline.pip);
        sg_apply_bindings(&sprite_pipeline.bind);
        sg_draw(0, 6, 1);
    }

    sg_end_pass();
    sg_commit();
}

int tick(double time, void* userdata)
{
    sfetch_dowork();
    draw();
    return true;
}

void frame_sokol_cb() 
{
    #ifndef EMSCRIPTEN
    // only in desktop builds we should manually call our per-frame func
    // in web builds the browser does it for us - emscripten_request_animation_frame_loop
    double timeSec = stm_sec(stm_now());
    tick(timeSec, 0);
    #endif
}

void cleanup() 
{
    sg_shutdown();
    sfetch_shutdown();
}


void event(const sapp_event* event)
{
    LOG_INFO("mousex: %f\n", event->mouse_x); 
    LOG_INFO("mousey: %f\n", event->mouse_y); 
    if (event->type == SAPP_EVENTTYPE_KEY_DOWN)
    {
        switch (event->key_code)
        {
            case SAPP_KEYCODE_ESCAPE:
            {
                sapp_quit();
            } break;
            default: {} break;
        }
    }
    if (event->type == SAPP_EVENTTYPE_RESIZED)
    {
        // window resize
        window_dimensions = {(float)event->window_width, (float)event->window_height};
        LOG_INFO("New window dimensions: %ix%i\n", event->window_width, event->window_height);
    }
}

sapp_desc sokol_main(int argc, char* argv[]) 
{
    sapp_desc app = 
    {
        .width = (int)window_dimensions.X,
        .height = (int)window_dimensions.Y,
        .init_cb = init,
        .frame_cb = frame_sokol_cb,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .logger.func = slog_func,
    };
    return app;
}