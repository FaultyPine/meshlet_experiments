

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
#include "maths.h"

#include "lines_renderer.h"

struct MyMesh
{
    struct SOKOL_SHDC_ALIGN(16) Vertex
    { // TODO: just use the struct definition from the generated shader header
        hmm_vec4 position;
        hmm_vec4 normal;
        hmm_vec2 texcoord;
        unsigned int meshlet_idx;
        unsigned int pad;
    };
    std::vector<int> indices = {};
    std::vector<Vertex> vertex_data = {};
    // an index into meshlets also corresponds to an index into meshlet_bounds and can be referred to as a "meshlet id"
    std::vector<meshopt_Meshlet> meshlets = {};
    std::vector<meshopt_Bounds> meshlet_bounds = {};

    sg_buffer idx_buf;
    sg_buffer vertex_buf;
    sg_buffer meshlet_data_buf;
    struct MeshFlags
    {
        u8 visible : 1;
    };
    MeshFlags flags;
};

struct RuntimeState
{
    Camera cam = {};
    Camera fakeCam = {};
    GraphicsPipelineState sprite_pipeline = {};
    GraphicsPipelineState model_pipeline = {};
    MyMesh mesh = {};
    LinesRenderer lines_renderer = {};
    bool isFakeCamAttached = true;
};
static RuntimeState state;

MyMesh GltfPrimitiveToMesh(const tinygltf::Model &model, const tinygltf::Primitive &prim)
{
    MyMesh mesh = {};
    int indices_accessor = prim.indices;
    if (indices_accessor != -1)
    {
        int indices_buf_view_idx = model.accessors[indices_accessor].bufferView;
        tinygltf::BufferView indices_bufview = model.bufferViews[indices_buf_view_idx];
        SOKOL_ASSERT(indices_bufview.target == TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER);
        const std::vector<unsigned char> &gltf_indices = model.buffers[indices_bufview.buffer].data;
        mesh.indices.resize(indices_bufview.byteLength / 4);
        memcpy(mesh.indices.data(), gltf_indices.data() + indices_bufview.byteOffset, indices_bufview.byteLength);
        sg_buffer_desc buf_desc =
            {
                .size = mesh.indices.size() * sizeof(int),
                .type = sg_buffer_type::SG_BUFFERTYPE_INDEXBUFFER,
                .usage = SG_USAGE_STREAM,
                //.data = {mesh.indices.data(), mesh.indices.size() * sizeof(int)},
            };
        mesh.idx_buf = sg_make_buffer(buf_desc);
        sg_update_buffer(mesh.idx_buf, {.ptr = mesh.indices.data(), mesh.indices.size() * sizeof(int)});
    }

    struct GltfBufNView
    {
        tinygltf::BufferView view;
        tinygltf::Buffer buf;
    };
    GltfBufNView vert_positions = {};
    GltfBufNView vert_normals = {};
    GltfBufNView vert_texcoords = {};
    if (prim.attributes.contains("POSITION"))
    {
        int accessor = prim.attributes.at("POSITION");
        int bufferview = model.accessors[accessor].bufferView;
        tinygltf::BufferView bufview = model.bufferViews[bufferview];
        tinygltf::Buffer buf = model.buffers[bufview.buffer];
        vert_positions = {bufview, buf};
    }
    if (prim.attributes.contains("NORMAL"))
    {
        int accessor = prim.attributes.at("NORMAL");
        int bufferview = model.accessors[accessor].bufferView;
        tinygltf::BufferView bufview = model.bufferViews[bufferview];
        tinygltf::Buffer buf = model.buffers[bufview.buffer];
        vert_normals = {bufview, buf};
    }
    if (prim.attributes.contains("TEXCOORD_0"))
    {
        int accessor = prim.attributes.at("TEXCOORD_0");
        int bufferview = model.accessors[accessor].bufferView;
        tinygltf::BufferView bufview = model.bufferViews[bufferview];
        tinygltf::Buffer buf = model.buffers[bufview.buffer];
        vert_texcoords = {bufview, buf};
    }
    SOKOL_ASSERT(vert_positions.view.buffer != -1);
    SOKOL_ASSERT(vert_positions.view.byteLength % sizeof(float) == 0);
    int num_vert_positions = vert_positions.view.byteLength / sizeof(float);
    SOKOL_ASSERT(num_vert_positions % 3 == 0);
    int num_triangles = num_vert_positions / 3;
    for (int tri_idx = 0; tri_idx < num_triangles; tri_idx++)
    {
        MyMesh::Vertex vert = {};
        // pos
        {
            float* data = (float*)(vert_positions.buf.data.data() + vert_positions.view.byteOffset);
            float* triangle_data = data + (tri_idx * 3);
            float vx = *(triangle_data+0);
            float vy = *(triangle_data+1);
            float vz = *(triangle_data+2);
            hmm_vec4 pos = HMM_Vec4(vx,vy,vz,1.0f);
            vert.position = pos;
        }
        // nrm
        if (vert_normals.view.buffer != -1)
        {
            float* data = (float*)(vert_normals.buf.data.data() + vert_normals.view.byteOffset);
            float* triangle_data = data + (tri_idx * 3);
            float nx = *(triangle_data+0);
            float ny = *(triangle_data+1);
            float nz = *(triangle_data+2);
            hmm_vec4 nrm = HMM_Vec4(nx,ny,nz,1.0f);
            vert.normal = nrm;
        }
        // texcoord
        if (vert_texcoords.view.buffer != -1)
        {
            float* data = (float*)(vert_texcoords.buf.data.data() + vert_texcoords.view.byteOffset);
            float* triangle_data = data + (tri_idx * 2);
            float tx = *(triangle_data+0);
            float ty = *(triangle_data+1);
            hmm_vec2 texcoord = HMM_Vec2(tx,ty);
            vert.texcoord = texcoord;
        }
        mesh.vertex_data.push_back(vert);
    }

    /* #region meshlet stuff */
    const size_t max_vertices = 64;
    const size_t max_triangles = 124;
    const float cone_weight = 0.0f;

    size_t max_meshlets = meshopt_buildMeshletsBound(mesh.indices.size(), max_vertices, max_triangles);
    mesh.meshlets = std::vector<meshopt_Meshlet>(max_meshlets);
    std::vector<meshopt_Meshlet>& meshlets = mesh.meshlets;
    // indices into original vertex buffer
    std::vector<unsigned int> meshlet_vertices(max_meshlets * max_vertices);
    std::vector<unsigned char> meshlet_triangles(max_meshlets * max_triangles * 3);

    // float3 position must be at the start of the vertex data for meshopt_buildMeshlets and meshopt_computeMeshletBounds
    static_assert(offsetof(MyMesh::Vertex, position) == 0);
    size_t meshlet_count = meshopt_buildMeshlets(meshlets.data(), meshlet_vertices.data(), meshlet_triangles.data(), mesh.indices.data(),
                                                 mesh.indices.size(), (float*)mesh.vertex_data.data(), mesh.vertex_data.size(), sizeof(MyMesh::Vertex), max_vertices, max_triangles, cone_weight);
    meshlets.resize(meshlet_count);
    LOG_INFO("built %i meshlets\n", meshlet_count);

    for (int i = 0; i < meshlets.size(); i++)
    {
        const meshopt_Meshlet &meshlet = meshlets[i];
        meshopt_Bounds bounds = meshopt_computeMeshletBounds(&meshlet_vertices[meshlet.vertex_offset], &meshlet_triangles[meshlet.triangle_offset], meshlet.triangle_count, (float*)mesh.vertex_data.data(), mesh.vertex_data.size(), sizeof(MyMesh::Vertex));
        mesh.meshlet_bounds.push_back(bounds);
    }

    // buffer for visualizing which vertices belong to which meshlets
    // mesh.meshlet_indices.resize(mesh.vert_positions.size());
    for (unsigned int meshlet_id = 0; meshlet_id < meshlets.size(); meshlet_id++)
    {
        const meshopt_Meshlet &meshlet = meshlets[meshlet_id];
        for (unsigned int j = 0; j < meshlet.vertex_count; j++)
        {
            unsigned int meshlet_proxy_buffer_idx = meshlet.vertex_offset + j;
            SOKOL_ASSERT(meshlet_proxy_buffer_idx < meshlet_vertices.size());
            unsigned int actual_meshlet_vert_idx = meshlet_vertices[meshlet_proxy_buffer_idx];

            SOKOL_ASSERT(actual_meshlet_vert_idx < mesh.vertex_data.size());
            mesh.vertex_data[actual_meshlet_vert_idx].meshlet_idx = meshlet_id;
        }
    }
    /* #endregion */
    sg_buffer_desc buf_desc = 
    {
        .type = SG_BUFFERTYPE_STORAGEBUFFER,
        .data = {mesh.vertex_data.data(), mesh.vertex_data.size() * sizeof(MyMesh::Vertex)},
    };
    mesh.vertex_buf = sg_make_buffer(&buf_desc);

    buf_desc = 
    {
        .size = sizeof(meshlet_data_t) * mesh.meshlets.size(),
        .type = SG_BUFFERTYPE_STORAGEBUFFER,
        .usage = sg_usage::SG_USAGE_STREAM,
    };
    mesh.meshlet_data_buf = sg_make_buffer(&buf_desc);
    mesh.flags.visible = true;
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

    MyMesh &mesh = state.mesh;
    mesh = GltfPrimitiveToMesh(model, model.meshes.at(0).primitives.at(0));

    sg_bindings bindings =
        {
            .index_buffer = mesh.idx_buf,
            .vs.storage_buffers[SLOT_ssbo_vert_data] = mesh.vertex_buf,
            .vs.storage_buffers[SLOT_ssbo_meshlet_data] = mesh.meshlet_data_buf,
        };
    sg_pipeline_desc pipeline_desc =
        {
            .shader = sg_make_shader(lit_model_shader_desc(sg_query_backend())),
            .depth =
            {
                .compare = SG_COMPAREFUNC_LESS_EQUAL,
                .write_enabled = true,
            },
            .cull_mode = SG_CULLMODE_BACK,
            .face_winding = sg_face_winding::SG_FACEWINDING_CCW,
            .index_type = sg_index_type::SG_INDEXTYPE_UINT32,   
            .colors[0].blend = 
            {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
            },           
        };
    sg_pipeline pipeline = sg_make_pipeline(&pipeline_desc);
    state.model_pipeline.pip = pipeline;
    state.model_pipeline.bind = bindings;
}

static void init()
{
    stm_setup();
    sfetch_desc_t sfetch_desc = {.logger.func = slog_func};
    sfetch_setup(&sfetch_desc);
    sg_desc appdesc =
        {
            .logger.func = slog_func,
            .environment = sglue_environment(),
        };
    sg_setup(&appdesc);
    simgui_desc_t imguidesc = {.logger = slog_func};
    simgui_setup(&imguidesc);
    sapp_lock_mouse(false);
    sapp_show_mouse(true);

    init_model_pipeline();
    // state.sprite_pipeline = init_sprite_pipeline();
    state.lines_renderer = InitLinesRenderer();
    InitializeCamera(state.cam);
    InitializeCamera(state.fakeCam, CameraType::ORBITING);
    state.fakeCam.speed = 0.01f;
}

bool MeshletBackfaceCull(const Camera& cam, const meshopt_Bounds& bounds)
{
    // dot(center - camera_position, cone_axis) >= cone_cutoff * length(center - camera_position) + radius
    hmm_vec3 meshletCenter = HMM_Vec3(bounds.center[0], bounds.center[1], bounds.center[2]);
    hmm_vec3 coneAxis = HMM_Vec3(bounds.cone_axis[0], bounds.cone_axis[1], bounds.cone_axis[2]);
    hmm_vec3 meshletToCam = HMM_SubtractVec3(meshletCenter, cam.pos);
    float dot = HMM_DotVec3(meshletToCam, coneAxis);
    return dot >= bounds.cone_cutoff * HMM_LengthVec3(meshletToCam) + bounds.radius;
}

u32 UpdateMeshletVisibilityBuffer(const MyMesh& mesh, const Camera& cam)
{
    u32 num_visible = 0;
    std::vector<meshlet_data_t> meshlet_data = {};
    for (int i = 0; i < mesh.meshlets.size(); i++)
    {
        const meshopt_Bounds& bounds = mesh.meshlet_bounds.at(i);
        bool should_frustum_cull = CamSphereShouldCull(cam, bounds.center, bounds.radius);
        bool should_backface_cull = MeshletBackfaceCull(cam, bounds);
        // bool should_occlusion_cull = OcclusionCull(cam, )
        bool is_meshlet_visible = !should_frustum_cull && !should_backface_cull;
        if (is_meshlet_visible) num_visible++;
        meshlet_data_t mdata = { .flags = is_meshlet_visible }; // only one flag rn
        meshlet_data.push_back(mdata);
    }
    sg_range visibility_buf_rng = {meshlet_data.data(), meshlet_data.size() * sizeof(meshlet_data_t)};
    sg_update_buffer(mesh.meshlet_data_buf, visibility_buf_rng);
    return num_visible;
}

void DrawGrid(hmm_vec3 gridCenter, hmm_vec3 color, int gridDimension, float gridSpacing)
{
    float extent = (gridDimension/2) * gridSpacing;
    for (int i = -gridDimension/2; i <= gridDimension/2; i++)
    {
        {
            float a[3] = {gridCenter.X + extent, 0, gridCenter.Z + (i * gridSpacing)};
            float b[3] = {gridCenter.X - extent, 0, gridCenter.Z + (i * gridSpacing)};
            PushLine(state.lines_renderer, a, b, color.Elements);
        }{
            float a[3] = {gridCenter.X + (i * gridSpacing), 0, gridCenter.Z + extent};
            float b[3] = {gridCenter.X + (i * gridSpacing), 0, gridCenter.Z - extent};
            PushLine(state.lines_renderer, a, b, color.Elements);
        }
    }
}

void draw()
{
    simgui_frame_desc_t imgui_frame_desc =
        {
            .width = sapp_width(),
            .height = sapp_height(),
            .delta_time = sapp_frame_duration(),
            .dpi_scale = sapp_dpi_scale()};
    simgui_new_frame(&imgui_frame_desc);
    sg_pass_action pass_action =
        {.colors[0] = {.load_action = SG_LOADACTION_CLEAR, .clear_value = {0.2f, 0.2f, 0.15f, 1.0f}}};
    sg_pass pass = {.action = pass_action, .swapchain = sglue_swapchain()};
    sg_begin_pass(&pass);
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Cam"))
        {
            if (ImGui::MenuItem("Reset to default position"))
            {
                state.cam.pos = {0, 0.5, 2.5};
            }
            ImGui::SliderFloat("Camera speed", &state.cam.speed, 0.0f, 20.0f);
            if (ImGui::Button("FakeCam right rotate"))
            {
                hmm_vec4 front = HMM_Vec4(state.fakeCam.front.X, state.fakeCam.front.Y, state.fakeCam.front.Z, 1.0f);
                front = HMM_MultiplyMat4ByVec4(HMM_Rotate(25.0, HMM_Vec3(0,1,0)), front);
                state.fakeCam.front = HMM_Vec3(front.X, front.Y, front.Z);
            }
            if (ImGui::Button("FakeCam left rotate"))
            {
                hmm_vec4 front = HMM_Vec4(state.fakeCam.front.X, state.fakeCam.front.Y, state.fakeCam.front.Z, 1.0f);
                front = HMM_MultiplyMat4ByVec4(HMM_Rotate(-25.0, HMM_Vec3(0,1,0)), front);
                state.fakeCam.front = HMM_Vec3(front.X, front.Y, front.Z);
            }
            ImGui::EndMenu();
        }
        bool meshvis = state.mesh.flags.visible;
        ImGui::Checkbox("Mesh visible", &meshvis);
        state.mesh.flags.visible = meshvis;
        ImGui::Checkbox("Fakecam attached", &state.isFakeCamAttached);
        ImGui::EndMainMenuBar();
    }

    // models
    hmm_mat4 proj = GetCameraProjectionMatrix(state.cam);
    hmm_mat4 view = GetCameraViewMatrix(state.cam);
    hmm_mat4 model = HMM_Mat4d(1);
    hmm_mat4 mvp = HMM_MultiplyMat4(HMM_MultiplyMat4(proj, view), model);
    struct vs_params
    {
        hmm_mat4 mvp;
        float time;
    };
    vs_params_t vs_params_instance = {.mvp = mvp, .time = (float)stm_sec(stm_now())};
    if (state.model_pipeline.pip.id && state.mesh.flags.visible)
    {
        u32 num_visible_meshlets = UpdateMeshletVisibilityBuffer(state.mesh, state.fakeCam);
        sg_apply_pipeline(state.model_pipeline.pip);
        sg_apply_bindings(&state.model_pipeline.bind);
        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, SG_RANGE_REF(vs_params_instance));
        sg_draw(0, state.mesh.indices.size(), 1);
    }
    // draw fake cam frustum
    if (!state.isFakeCamAttached)
    {
        DrawCamFrustum(state.fakeCam, state.lines_renderer, HMM_Vec3(0,1,1));
    }
    DrawGrid(HMM_Vec3(0,0,0), HMM_Vec3(1,1,1), 15, 0.5f);    
    FlushLinesRenderer(state.lines_renderer, mvp);

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
    if (state.isFakeCamAttached)
    {
        state.fakeCam = state.cam;
    }
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
    }
    break;
    case SAPP_KEYCODE_TAB:
    {
        sapp_show_mouse(!sapp_mouse_shown());
        sapp_lock_mouse(!sapp_mouse_locked());
    }
    break;
    default:
    {
    }
    break;
    }
}

void event(const sapp_event *event)
{
    if (simgui_handle_event(event))
    {
        return;
    }
    if (event->type == SAPP_EVENTTYPE_MOUSE_MOVE)
    {
       OnMouseEvent(event, state.cam);
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

sapp_desc sokol_main(int argc, char *argv[])
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