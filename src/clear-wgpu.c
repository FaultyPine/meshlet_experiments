//------------------------------------------------------------------------------
//  clear-wgpu.c
//  Simple draw loop, clear default framebuffer.
//------------------------------------------------------------------------------
#define SOKOL_IMPL
#define SOKOL_WGPU
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_log.h"

#include "wgpu/wgpu_entry.h"

#include "wgpu/wgpu_entry_emsc.c"
#include "wgpu/wgpu_entry_swapchain.c"
#include "wgpu/wgpu_entry.c"

// emcc src/clear-wgpu.c -Iexternal -s USE_WEBGPU=1 -DTARGET_WEB --shell-file=configs/index.html -o app.html

static sg_pass_action pass_action;

static void init(void) {
    sg_setup(&(sg_desc){
        .environment = wgpu_environment(),
        .logger.func = slog_func,
    });
    pass_action = (sg_pass_action) {
        .colors[0] = {
            .load_action = SG_LOADACTION_CLEAR,
            .clear_value = { 1.0f, 0.0f, 0.0f, 1.0f }
        }
    };
}

static void frame(void) {
    float g = pass_action.colors[0].clear_value.g + 0.01f;
    pass_action.colors[0].clear_value.g = (g > 1.0f) ? 0.0f : g;
    sg_begin_pass(&(sg_pass){ .action = pass_action, .swapchain = wgpu_swapchain() });
    sg_end_pass();
    sg_commit();
}

static void shutdown(void) {
    sg_shutdown();
}

int main() {
    wgpu_start(&(wgpu_desc_t){
        .init_cb = init,
        .frame_cb = frame,
        .shutdown_cb = shutdown,
        .width = 640,
        .height = 480,
        .title = "clear-wgpu"
    });
    return 0;
}