

#include "common.h"

#include "external/sokol/sokol_app.h"
#include "external/sokol/sokol_log.h"
#include "external/sokol/sokol_gfx.h"
#include "external/sokol/sokol_glue.h"
#include "external/sokol/sokol_time.h"
#include "external/sokol/sokol_fetch.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "external/imgui/imgui.h"
#include "external/sokol/sokol_imgui.h"

#include "external/tinygltf/tiny_gltf.h"
#include "external/meshoptimizer/src/meshoptimizer.h"
#define HANDMADE_MATH_IMPLEMENTATION // not sure why but web build needs impl here even tho it's in external_impl.cpp...?
#include "external/HandmadeMath.h"

#ifdef TARGET_WEB
#include "external/emsc.h"
#include "shaders/generated/glsl300es/lit_model_shdc.h"
#elif defined(TARGET_DESKTOP)
#include "shaders/generated/glsl430/lit_model_shdc.h"
#endif

#include <iostream>

#include "sprite_pipeline.h"
#include "input.h"
#include "graphics_pipeline.h"
#include "log.h"



struct Camera
{
    hmm_vec3 pos;
    hmm_vec3 front;
    hmm_vec3 lookat_override;
    bool should_override_lookat;
    float yaw;
    float pitch;
};


struct MyMesh
{
    struct Vertex
    {
        hmm_vec4 position;
        hmm_vec4 normal;
        hmm_vec2 texcoord;
    };
    std::vector<int> indices = {};
    std::vector<float> vert_positions = {};
    std::vector<float> vert_normals = {};
    std::vector<float> vert_texcoords = {};

    sg_buffer idx_buf;
    sg_buffer vert_pos_buf;
    sg_buffer vert_normal_buf;
    sg_buffer vert_texcoord_buf;
};

struct RuntimeState
{
    Camera cam = {};
    GraphicsPipelineState sprite_pipeline = {};
    GraphicsPipelineState model_pipeline = {};
    MyMesh mesh = {};
};
static RuntimeState state;


hmm_vec2 window_dimensions = {1280, 720};

hmm_vec2 GetWindowDimensions()
{
    return window_dimensions;
}
float GetWindowWidth() { return window_dimensions.X; }
float GetWindowHeight() { return window_dimensions.Y; }


int tick(double time, void* userdata);




MyMesh GltfPrimitiveToMesh(const tinygltf::Model& model, const tinygltf::Primitive& prim)
{
    MyMesh mesh = {};
    int indices_accessor = prim.indices;
    if (indices_accessor != -1)
    {
        int indices_buf_view_idx = model.accessors[indices_accessor].bufferView;
        tinygltf::BufferView indices_bufview = model.bufferViews[indices_buf_view_idx];
        SOKOL_ASSERT(indices_bufview.target == TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER);
        const std::vector<unsigned char>& gltf_indices = model.buffers[indices_bufview.buffer].data;
        mesh.indices.resize(indices_bufview.byteLength / 4);
        memcpy(mesh.indices.data(), gltf_indices.data() + indices_bufview.byteOffset, indices_bufview.byteLength);
    }
    if (prim.attributes.contains("POSITION"))
    {
        int accessor = prim.attributes.at("POSITION");
        int bufferview = model.accessors[accessor].bufferView;
        tinygltf::BufferView bufview = model.bufferViews[bufferview];
        tinygltf::Buffer buf = model.buffers[bufview.buffer];
        mesh.vert_positions.resize(bufview.byteLength / 4);
        memcpy(mesh.vert_positions.data(), buf.data.data() + bufview.byteOffset, bufview.byteLength);
    }
    if (prim.attributes.contains("NORMAL"))
    {
        int accessor = prim.attributes.at("NORMAL");
        int bufferview = model.accessors[accessor].bufferView;
        tinygltf::BufferView bufview = model.bufferViews[bufferview];
        tinygltf::Buffer buf = model.buffers[bufview.buffer];
        mesh.vert_normals.resize(bufview.byteLength / 4);
        memcpy(mesh.vert_normals.data(), buf.data.data() + bufview.byteOffset, bufview.byteLength);
    }
    if (prim.attributes.contains("TEXCOORD_0"))
    {
        int accessor = prim.attributes.at("TEXCOORD_0");
        int bufferview = model.accessors[accessor].bufferView;
        tinygltf::BufferView bufview = model.bufferViews[bufferview];
        tinygltf::Buffer buf = model.buffers[bufview.buffer];
        mesh.vert_texcoords.resize(bufview.byteLength / 4);
        memcpy(mesh.vert_texcoords.data(), buf.data.data() + bufview.byteOffset, bufview.byteLength);
    }
    else
    {
        mesh.vert_texcoords = std::vector<float>(mesh.vert_positions.size(), 0.0);
    }

    sg_buffer_desc buf_desc = 
    {
        .type = sg_buffer_type::SG_BUFFERTYPE_INDEXBUFFER,
        .data = {mesh.indices.data(), mesh.indices.size() * sizeof(int)},
    };
    mesh.idx_buf = sg_make_buffer(buf_desc);

    buf_desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
    buf_desc.data = {mesh.vert_positions.data(), mesh.vert_positions.size() * sizeof(float)};
    mesh.vert_pos_buf = sg_make_buffer(buf_desc);
    buf_desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
    buf_desc.data = {mesh.vert_normals.data(), mesh.vert_normals.size() * sizeof(float)};
    mesh.vert_normal_buf = sg_make_buffer(buf_desc);
    
    buf_desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
    buf_desc.data = {mesh.vert_texcoords.data(), mesh.vert_texcoords.size() * sizeof(float)};
    mesh.vert_texcoord_buf = sg_make_buffer(buf_desc);

    return mesh;
}

void init_model_pipeline()
{
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, "res/dragon.glb"); // for binary glTF(.glb)
    if (!warn.empty()) 
    {
        printf("Warn: %s\n", warn.c_str());
    }
    if (!err.empty()) 
    {
        printf("Err: %s\n", err.c_str());
    }
    if (!ret) 
    {
        printf("Failed to parse glTF\n");
        return;
    }

    MyMesh& mesh = state.mesh;
    mesh = GltfPrimitiveToMesh(model, model.meshes.at(0).primitives.at(0));
    /*
    
    const size_t max_vertices = 64;
    const size_t max_triangles = 124;
    const float cone_weight = 0.0f;

    size_t max_meshlets = meshopt_buildMeshletsBound(indices.size(), max_vertices, max_triangles);
    std::vector<meshopt_Meshlet> meshlets(max_meshlets);
    // indices into original vertex buffer
    std::vector<unsigned int> meshlet_vertices(max_meshlets * max_vertices);
    std::vector<unsigned char> meshlet_triangles(max_meshlets * max_triangles * 3);

    size_t meshlet_count = meshopt_buildMeshlets(meshlets.data(), meshlet_vertices.data(), meshlet_triangles.data(), indices.data(),
        indices.size(), attrib.vertices.data(), attrib.vertices.size(), sizeof(float)*3, max_vertices, max_triangles, cone_weight);
    meshlets.resize(meshlet_count);

    LOG_INFO("built %i meshlets\n", meshlet_count);
    */

    sg_bindings bindings =
    {
        .index_buffer = mesh.idx_buf,
        .vertex_buffers[0] = mesh.vert_pos_buf,
        .vertex_buffers[1] = mesh.vert_normal_buf,
        .vertex_buffers[2] = mesh.vert_texcoord_buf,
    };
    sg_pipeline_desc pipeline_desc = 
    {
        .shader = sg_make_shader(lit_model_shader_desc(sg_query_backend())),
        .index_type = SG_INDEXTYPE_UINT32,
        .layout = 
        {
            .attrs = 
            {
                [ATTR_vs_position] = {.format = sg_vertex_format::SG_VERTEXFORMAT_FLOAT3, .buffer_index = 0},
                [ATTR_vs_normal] = {.format = sg_vertex_format::SG_VERTEXFORMAT_FLOAT3, .buffer_index = 1},
                [ATTR_vs_texcoord] = {.format = sg_vertex_format::SG_VERTEXFORMAT_FLOAT2, .buffer_index = 2},
            },
        },
        .depth = 
        {
            .compare = SG_COMPAREFUNC_LESS,
            .write_enabled = true,
        },
        .cull_mode = SG_CULLMODE_BACK,
        .face_winding = sg_face_winding::SG_FACEWINDING_CCW,
    };
    sg_pipeline pipeline = sg_make_pipeline(&pipeline_desc);
    state.model_pipeline.pip = pipeline;
    state.model_pipeline.bind = bindings;
}

static void init() 
{
    stm_setup();
    sfetch_desc_t sfetch_desc = { .logger.func = slog_func };
    sfetch_setup(&sfetch_desc);
    sg_environment env = {};
    #ifdef TARGET_WEB
    emsc_init("#canvas", EMSC_ANTIALIAS, (int)window_dimensions.X, (int)window_dimensions.Y);
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
    simgui_desc_t imguidesc = { .logger = slog_func };
    simgui_setup(&imguidesc);

    init_model_pipeline();

    state.sprite_pipeline = init_sprite_pipeline();

    state.cam.pos = {0, 0.5, 2.5};

    #ifdef TARGET_WEB
    // hand off control to browser loop
    emscripten_request_animation_frame_loop(tick, 0);
    #endif
}


hmm_mat4 GetCameraViewMatrix(const Camera& cam)
{
    hmm_vec3 center = HMM_AddVec3(cam.pos, cam.front);
    if (cam.should_override_lookat)
    {
        center = cam.lookat_override;
    }
    hmm_mat4 view = HMM_LookAt(cam.pos, center, HMM_Vec3(0.0f, 1.0f, 0.0f));
    return view;
}

hmm_vec3 GetNormalizedLookDir(float yaw, float pitch) 
{
    hmm_vec3 direction = HMM_Vec3(0,0,0);
    direction.X = cos(HMM_ToRadians(yaw)) * cos(HMM_ToRadians(pitch));
    direction.Y = sin(HMM_ToRadians(pitch));
    direction.Z = sin(HMM_ToRadians(yaw)) * cos(HMM_ToRadians(pitch));
    return HMM_NormalizeVec3(direction);
}

void draw()
{
    simgui_frame_desc_t imgui_frame_desc = 
    {
        .width = sapp_width(),
        .height = sapp_height(),
        .delta_time = sapp_frame_duration(),
        .dpi_scale = sapp_dpi_scale()
    };
    simgui_new_frame(&imgui_frame_desc);
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

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Utilities"))
        {
            if (ImGui::MenuItem("Reset camera"))
            {
                state.cam.pos = HMM_Vec3(0,0,0);
                state.cam.front = HMM_Vec3(0,0,1);
                state.cam.yaw = 0;
                state.cam.pitch = 0;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    // models
    hmm_mat4 proj = HMM_Perspective(60.0f, GetWindowWidth() / GetWindowHeight(), 0.01f, 100.0f);
    hmm_mat4 view = GetCameraViewMatrix(state.cam);
    hmm_mat4 model = HMM_Mat4d(1);
    hmm_mat4 mvp = HMM_MultiplyMat4(HMM_MultiplyMat4(proj, view), model);
    struct vs_params
    {
        hmm_mat4 mvp;
    };
    vs_params vs_params_instance = { .mvp = mvp };
    if (state.model_pipeline.pip.id)
    {
        sg_apply_pipeline(state.model_pipeline.pip);
        sg_apply_bindings(&state.model_pipeline.bind);
        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, SG_RANGE_REF(vs_params_instance));
        sg_draw(0, state.mesh.indices.size(), 1);
    }

    // sprites
    const bool draw_sprites = false;
    if (draw_sprites)
    {
        sg_apply_pipeline(state.sprite_pipeline.pip);
        sg_apply_bindings(&state.sprite_pipeline.bind);
        sg_draw(0, 6, 1);
    }

    simgui_render();
    sg_end_pass();
    sg_commit();
}

void camera_tick()
{
    //cam input
    Camera& cam = state.cam;
    constexpr float speed = 0.1f;
    hmm_vec3& cam_pos = cam.pos;
    hmm_vec4 rotated = HMM_MultiplyMat4ByVec4(HMM_Rotate(0.7f, HMM_Vec3(0,1,0)), HMM_Vec4(cam_pos.X, cam_pos.Y, cam_pos.Z, 1.0));
    cam_pos = HMM_Vec3(rotated.X, rotated.Y, rotated.Z);
    hmm_vec3 camera_right = HMM_NormalizeVec3(HMM_Cross(cam.front, HMM_Vec3(0.0, 1.0, 0.0)));
    if (IsKeyDown(SAPP_KEYCODE_W))
    {
        cam_pos = HMM_AddVec3(cam_pos, HMM_MultiplyVec3f(HMM_Vec3(cam.front.X, 0.0, cam.front.Z), speed));
    }
    if (IsKeyDown(SAPP_KEYCODE_S))
    {
        cam_pos = HMM_SubtractVec3(cam_pos, HMM_MultiplyVec3f(HMM_Vec3(cam.front.X, 0.0, cam.front.Z), speed));
    }
    if (IsKeyDown(SAPP_KEYCODE_A))
    {
        cam_pos = HMM_SubtractVec3(cam_pos, HMM_MultiplyVec3f(camera_right, speed));
    }
    if (IsKeyDown(SAPP_KEYCODE_D))
    {
        cam_pos = HMM_AddVec3(cam_pos, HMM_MultiplyVec3f(camera_right, speed));
    }
    if (IsKeyDown(SAPP_KEYCODE_LEFT_SHIFT))
    {
        cam_pos.Y -= speed;
    }
    if (IsKeyDown(SAPP_KEYCODE_SPACE))
    {
        cam_pos.Y += speed;
    }

    state.cam.should_override_lookat = true;
    state.cam.lookat_override = HMM_Vec3(0,0,0);
}

int tick(double time, void* userdata)
{
    sfetch_dowork();
    camera_tick();
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
    simgui_shutdown();
    sfetch_shutdown();
    sg_shutdown();
}


void event(const sapp_event* event)
{
    if (simgui_handle_event(event))
    {
        return;
    }
    if (event->type == SAPP_EVENTTYPE_MOUSE_MOVE)
    {
        float mousedx = event->mouse_dx;
        float mousedy = event->mouse_dy;
        #define MOUSE_SENSITIVITY 0.05f
        mousedx *= MOUSE_SENSITIVITY;
        mousedy *= MOUSE_SENSITIVITY;
        state.cam.pitch -= mousedy;
        state.cam.yaw += mousedx;
        state.cam.pitch = HMM_Clamp(-89.0f, state.cam.pitch, 89.0f);
        state.cam.front = GetNormalizedLookDir(state.cam.yaw, state.cam.pitch);
    }
    else if (event->type == SAPP_EVENTTYPE_KEY_DOWN || event->type == SAPP_EVENTTYPE_CHAR)
    {
        switch (event->key_code)
        {
            case SAPP_KEYCODE_ESCAPE:
            {
                sapp_quit();
            } break;
            case SAPP_KEYCODE_TAB:
            {
                sapp_show_mouse(!sapp_mouse_shown());
                sapp_lock_mouse(!sapp_mouse_locked());
            } break;
            default: {} break;
        }
    }
    else if (event->type == SAPP_EVENTTYPE_RESIZED)
    {
        // window resize
        window_dimensions = {(float)event->window_width, (float)event->window_height};
        LOG_INFO("New window dimensions: %ix%i\n", event->window_width, event->window_height);
    }
    InputSystemOnEvent(event);
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
        .icon.sokol_default = true,
        .window_title = "Playground",
    };
    return app;
}