
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

int draw(double time, void* userdata);

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

    float vertices[] = 
    {
         0.0f,  0.5f, 0.5f,     1.0f, 0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.5f,     0.0f, 1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 1.0f, 1.0f
    };
    state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc)
    {
        .data = SG_RANGE(vertices),
    });

    state.pip = sg_make_pipeline(&(sg_pipeline_desc)
    {
        .shader = sg_make_shader(triangle_shader_desc(sg_query_backend())),
        .layout = 
        {
            .attrs = 
            {
                [ATTR_vs_position].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_color0].format = SG_VERTEXFORMAT_FLOAT4
            }
        },
    });

    state.pass_action = (sg_pass_action) 
    {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value={0.0f, 0.0f, 0.0f, 1.0f } }
    };

    #ifdef TARGET_WEB
    // hand off control to browser loop
    emscripten_request_animation_frame_loop(draw, 0);
    #endif
}

int draw(double time, void* userdata)
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
    sg_draw(0, 3, 1);
    sg_end_pass();
    sg_commit();
    return true;
}

void tick()
{
    
}

void frame() 
{
    double timeSec = stm_sec(stm_now());
    tick();
    draw(timeSec, 0);
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
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .logger.func = slog_func,
    };
}