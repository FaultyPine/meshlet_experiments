
#ifdef TARGET_WEB
#define SOKOL_GLES3
#elif defined(TARGET_DESKTOP)
#define SOKOL_GLCORE
#else
#error "Unsupported target right now..."
#endif
#include "common.h"

#ifdef TARGET_WEB
#include "emsc.h"
#include "shaders/generated/glsl300es/triangle_shdc.h"
#elif defined(TARGET_DESKTOP)
#include "shaders/generated/glsl430/triangle_shdc.h"
#endif

#include "log.h"


static struct 
{
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
} state;

int tick(double time, void* userdata);

static void init() 
{
    stm_setup();
    sg_environment env = {};
    #ifdef TARGET_WEB
    emsc_init("#canvas", EMSC_ANTIALIAS);
    env = emsc_environment();
    #elif defined(TARGET_DESKTOP)
    env = sglue_environment();
    #endif
    sg_setup(&(sg_desc)
    {
        .environment = env,
        .logger.func = slog_func,
    });

    static const float tex_quad[] = { 
        // top left is 0,0  bottom right is 1,1
        // pos      // tex
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 

        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f
    };

    sg_image_desc img_desc = {0};
    img_desc = _sg_image_desc_defaults(&img_desc);
    #define IMG_WIDTH 256
    #define IMG_HEIGHT 256
    img_desc.height = IMG_HEIGHT;
    img_desc.width = IMG_WIDTH;

    uint32_t img_data[IMG_WIDTH * IMG_HEIGHT];

    for (int j = 0; j < IMG_HEIGHT; j++) {
        for (int i = 0; i < IMG_WIDTH; i++) {
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
    smp_desc = _sg_sampler_desc_defaults(&smp_desc);
    sg_sampler smp = sg_make_sampler(&smp_desc);

    state.bind = (sg_bindings)
    {
        .vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc)
        {
            .data = SG_RANGE(tex_quad),
        }),
        .fs = 
        {
            .images[SLOT_tex] = img,
            .samplers[SLOT_smp] = smp,
        }
    };
    

    state.pip = sg_make_pipeline(&(sg_pipeline_desc)
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
    });

    state.pass_action = (sg_pass_action) 
    {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value={0.0f, 0.0f, 0.0f, 1.0f } }
    };

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

    sg_begin_pass(&(sg_pass){ .action = state.pass_action, .swapchain = swapchain });
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);
    //sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_tex, tex);
    sg_draw(0, 6, 1);
    sg_end_pass();
    sg_commit();

}

int tick(double time, void* userdata)
{
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
}

sapp_desc sokol_main(int argc, char* argv[]) 
{
    return (sapp_desc) 
    {
        .width = 1280,
        .height = 720,
        .init_cb = init,
        .frame_cb = frame_sokol_cb,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .logger.func = slog_func,
    };
}