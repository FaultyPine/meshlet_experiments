

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
#define HANDMADE_MATH_IMPLEMENTATION
#include "external/HandmadeMath.h"
#undef HANDMADE_MATH_IMPLEMENTATION

#ifdef TARGET_WEB
#include "shaders/generated/wgsl/lit_model_shdc.h"
#elif defined(TARGET_DESKTOP)
#include "shaders/generated/glsl430/lit_model_shdc.h"
#endif

#include <iostream>

#include "sprite_pipeline.h"
#include "input.h"
#include "graphics_pipeline.h"
#include "log.h"
#include "camera.h"




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
    std::vector<unsigned int> meshlet_indices = {};
    std::vector<meshopt_Bounds> meshlet_bounds = {};

    sg_buffer idx_buf;
    sg_buffer vert_pos_buf;
    sg_buffer vert_normal_buf;
    sg_buffer vert_texcoord_buf;
    sg_buffer vert_meshlet_idx_buf;
};

struct RuntimeState
{
    Camera cam = {};
    GraphicsPipelineState sprite_pipeline = {};
    GraphicsPipelineState model_pipeline = {};
    MyMesh mesh = {};
};
static RuntimeState state;



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
        mesh.vert_texcoords = std::vector<float>(mesh.vert_positions.size());
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

    // meshlet stuff
    
    const size_t max_vertices = 64;
    const size_t max_triangles = 124;
    const float cone_weight = 0.0f;

    size_t max_meshlets = meshopt_buildMeshletsBound(mesh.indices.size(), max_vertices, max_triangles);
    std::vector<meshopt_Meshlet> meshlets(max_meshlets);
    // indices into original vertex buffer
    std::vector<unsigned int> meshlet_vertices(max_meshlets * max_vertices);
    std::vector<unsigned char> meshlet_triangles(max_meshlets * max_triangles * 3);

    size_t meshlet_count = meshopt_buildMeshlets(meshlets.data(), meshlet_vertices.data(), meshlet_triangles.data(), mesh.indices.data(),
        mesh.indices.size(), mesh.vert_positions.data(), mesh.vert_positions.size(), sizeof(float)*3, max_vertices, max_triangles, cone_weight);
    meshlets.resize(meshlet_count);
    LOG_INFO("built %i meshlets\n", meshlet_count);

    for (int i = 0; i < meshlets.size(); i++)
    {
        const meshopt_Meshlet& meshlet = meshlets[i];
        meshopt_Bounds bounds = meshopt_computeMeshletBounds(&meshlet_vertices[meshlet.vertex_offset], &meshlet_triangles[meshlet.triangle_offset], meshlet.triangle_count, mesh.vert_positions.data(), mesh.vert_positions.size(), sizeof(float)*3);
        mesh.meshlet_bounds.push_back(bounds);
    }

    // buffer for visualizing which vertices belong to which meshlets
    mesh.meshlet_indices.resize(mesh.vert_positions.size());
    for (unsigned int meshlet_id = 0; meshlet_id < meshlets.size(); meshlet_id++)
    {  
        const meshopt_Meshlet& meshlet = meshlets[meshlet_id];
        for (int j = 0; j < meshlet.vertex_count; j++)
        {
            unsigned int meshlet_proxy_buffer_idx = meshlet.vertex_offset + j;
            SOKOL_ASSERT(meshlet_proxy_buffer_idx < meshlet_vertices.size());
            unsigned int actual_meshlet_vert_idx = meshlet_vertices[meshlet_proxy_buffer_idx];

            SOKOL_ASSERT(actual_meshlet_vert_idx < mesh.meshlet_indices.size());
            SOKOL_ASSERT(meshlet_id <= USHRT_MAX);
            mesh.meshlet_indices[actual_meshlet_vert_idx] = meshlet_id;
        }
    }
    sg_buffer_desc buffer_desc =  
    {
        .type = sg_buffer_type::SG_BUFFERTYPE_VERTEXBUFFER,
        .data = {mesh.meshlet_indices.data(), mesh.meshlet_indices.size() * sizeof(unsigned int)},
    };
    mesh.vert_meshlet_idx_buf = sg_make_buffer(&buffer_desc);
    // end meshlet stuff

    return mesh;
}

void init_model_pipeline()
{
    sg_features features = sg_query_features();
    LOG_INFO("Storage buffer support: %i\n", features.storage_buffer);
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
    
    sg_bindings bindings =
    {
        .vertex_buffers[0] = mesh.vert_pos_buf,
        .vertex_buffers[1] = mesh.vert_normal_buf,
        .vertex_buffers[2] = mesh.vert_texcoord_buf,
        .vertex_buffers[3] = mesh.vert_meshlet_idx_buf,
        .index_buffer = mesh.idx_buf,
    };
    sg_pipeline_desc pipeline_desc = 
    {
        .shader = sg_make_shader(lit_model_shader_desc(sg_query_backend())),
        .layout = 
        {
            .attrs = 
            {
                [ATTR_vs_position] = 
                {
                    .buffer_index = 0,
                    .format = sg_vertex_format::SG_VERTEXFORMAT_FLOAT3, 
                },
                [ATTR_vs_normal] = 
                {
                    .buffer_index = 1,
                    .format = sg_vertex_format::SG_VERTEXFORMAT_FLOAT3, 
                },
                [ATTR_vs_texcoord] = 
                {
                    .buffer_index = 2,
                    .format = sg_vertex_format::SG_VERTEXFORMAT_FLOAT2, 
                },
                [ATTR_vs_meshlet_idx] = 
                {
                    .buffer_index = 3,
                    .format = sg_vertex_format::SG_VERTEXFORMAT_UBYTE4, 
                },
            },
        },
        .depth = 
        {
            .compare = SG_COMPAREFUNC_LESS,
            .write_enabled = true,
        },
        .index_type = SG_INDEXTYPE_UINT32,
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
    sg_desc appdesc = 
    {
        .logger.func = slog_func,
        .environment = sglue_environment(),
    };
    sg_setup(&appdesc);
    simgui_desc_t imguidesc = { .logger = slog_func };
    simgui_setup(&imguidesc);

    init_model_pipeline();

    //state.sprite_pipeline = init_sprite_pipeline();

    InitializeCamera(state.cam);
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
    sg_pass_action pass_action =
    {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value={ 0.0f, 0.0f, 0.0f, 1.0f } }
    };
    sg_pass pass = { .action = pass_action, .swapchain = sglue_swapchain() };
    sg_begin_pass(&pass);
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Cam"))
        {
            if (ImGui::MenuItem("Reset to default position"))
            {
                state.cam.pos = {0, 0.5, 2.5};
            }
            ImGui::SliderFloat("Camera orbit speed", &state.cam.orbit_speed, 0.0f, 20.0f);
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    // models
    hmm_mat4 proj = HMM_Perspective(60.0f, sapp_widthf() / sapp_heightf(), 0.01f, 100.0f);
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
    if (state.sprite_pipeline.pip.id)
    {
        sg_apply_pipeline(state.sprite_pipeline.pip);
        sg_apply_bindings(&state.sprite_pipeline.bind);
        sg_draw(0, 6, 1);
    }

    simgui_render();
    sg_end_pass();
    sg_commit();
}


void frame_sokol_cb() 
{
    sfetch_dowork();
    CameraTick(state.cam);
    draw();
}

void cleanup() 
{
    simgui_shutdown();
    sfetch_shutdown();
    sg_shutdown();
}

void OnKeyDownEvent(int key)
{
    switch (key)
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
    else if (event->type == SAPP_EVENTTYPE_KEY_DOWN)
    {
        OnKeyDownEvent(event->key_code);
    }
    else if (event->type == SAPP_EVENTTYPE_RESIZED)
    {
        // window resize
        LOG_INFO("New window dimensions: %ix%i\n", event->window_width, event->window_height);
    }
    else if (event->type == SAPP_EVENTTYPE_MOUSE_SCROLL)
    {
        constexpr float zoom_multiplier = 0.05f;
        float scroll_vert_dist = event->scroll_y * zoom_multiplier;
        state.cam.pos = HMM_AddVec3(state.cam.pos, HMM_MultiplyVec3f(state.cam.front, scroll_vert_dist));
    }
    InputSystemOnEvent(event);
}

sapp_desc sokol_main(int argc, char* argv[]) 
{
    sapp_desc app = 
    {
        .init_cb = init,
        .frame_cb = frame_sokol_cb,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .width = (int)640,
        .height = (int)480,
        .window_title = "Playground",
        .icon.sokol_default = true,
        .logger.func = slog_func,
    };
    return app;
}