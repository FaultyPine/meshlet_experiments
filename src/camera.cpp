#include "camera.h"

#include "input.h"
#include "external/sokol/sokol_app.h"
#include "maths.h"
#include "lines_renderer.h"

#define HANDMADE_MATH_IMPLEMENTATION
#include "external/HandmadeMath.h"


void InitializeCamera(Camera& cam, CameraType type)
{
    cam.type = type;
    cam.pos = {0, 0.5, 2.5};
    cam.front = HMM_NormalizeVec3(HMM_SubtractVec3(cam.pos, HMM_Vec3(0,0,0)));
    if (type == ORBITING)
    {
        cam.orbit_speed = 0.2f;
        cam.should_override_lookat = true;
        cam.lookat_override = HMM_Vec3(0,0,0);
    }

    cam.FOV = 60.0f;
    cam.nearClip = 0.01f;
    cam.farClip = 100.0f;
}

void CameraTick(Camera& cam)
{
    constexpr float speed = 0.1f;
    hmm_vec3& cam_pos = cam.pos;
    if (cam.type == ORBITING)
    {
        hmm_vec4 rotated = HMM_MultiplyMat4ByVec4(HMM_Rotate(cam.orbit_speed, HMM_Vec3(0,1,0)), HMM_Vec4(cam_pos.X, cam_pos.Y, cam_pos.Z, 1.0));
        cam_pos = HMM_Vec3(rotated.X, rotated.Y, rotated.Z);
        cam.front = HMM_NormalizeVec3(HMM_SubtractVec3(cam.lookat_override, cam.pos));
    }
    else if (cam.type == FREECAM)
    {
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
    }
}

hmm_mat4 GetCameraViewMatrix(const Camera& cam)
{
    hmm_vec3 center = HMM_AddVec3(cam.pos, cam.front);
    hmm_mat4 view = HMM_LookAt(cam.pos, center, HMM_Vec3(0.0f, 1.0f, 0.0f));
    return view;
}

hmm_mat4 GetCameraProjectionMatrix(const Camera& cam)
{
    return HMM_Perspective(cam.FOV, sapp_widthf() / sapp_heightf(), cam.nearClip, cam.farClip);
}

hmm_vec3 GetNormalizedLookDir(float yaw, float pitch) 
{
    hmm_vec3 direction = HMM_Vec3(0,0,0);
    direction.X = cos(HMM_ToRadians(yaw)) * cos(HMM_ToRadians(pitch));
    direction.Y = sin(HMM_ToRadians(pitch));
    direction.Z = sin(HMM_ToRadians(yaw)) * cos(HMM_ToRadians(pitch));
    return HMM_NormalizeVec3(direction);
}

void DrawCamFrustum(const Camera& cam, LinesRenderer& renderer, hmm_vec3 color)
{
    hmm_vec3 camFrustumVerts[8];
    hmm_mat4 view = GetCameraViewMatrix(cam);
    hmm_mat4 proj = GetCameraProjectionMatrix(cam);
    hmm_mat4 projview = HMM_MultiplyMat4(proj, view);
    hmm_mat4 invProjView = {};
    bool result = InvertMatrix((float*)&projview.Elements[0], (float*)&invProjView.Elements[0]);
    SOKOL_ASSERT(result);
    static hmm_vec3 _cameraFrustumCornerVertices[8] =
    {
        // near
        {-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1},  {-1,  1, -1},
        // far
        {-1, -1, 1},	{ 1, -1, 1},	{ 1,  1, 1},  {-1,  1, 1}
    };
    static u32 frustumIndices[] = {0,1 , 1,2, 2,3, 3,0, 4,5, 5,6, 6,7, 7,4, 0,4, 1,5, 2,6, 3,7};
    for (u32 i = 0; i < 8; i++)
    {
        hmm_vec3 defaultVert = _cameraFrustumCornerVertices[i];
        hmm_vec4 vert = HMM_MultiplyMat4ByVec4(invProjView, HMM_Vec3To4(defaultVert));
        camFrustumVerts[i] = HMM_DivideVec3f(HMM_Vec4To3(vert), vert.W);
    }
    for (u32 i = 0; i < ARRAY_SIZE(frustumIndices); i+=2)
    {
        PushLine(renderer, camFrustumVerts[frustumIndices[i]].Elements, camFrustumVerts[frustumIndices[i+1]].Elements, color.Elements);
    }
}

void extract_planes_from_projviewmat(
        const float mat[4][4],
        float left[4], float right[4],
        float bottom[4], float top[4],
        float near[4], float far[4])
{
    for (int i = 4; i--; ) { left[i]   = mat[i][3] + mat[i][0]; }
    for (int i = 4; i--; ) { right[i]  = mat[i][3] - mat[i][0]; }
    for (int i = 4; i--; ) { bottom[i] = mat[i][3] + mat[i][1]; }
    for (int i = 4; i--; ) { top[i]    = mat[i][3] - mat[i][1]; }
    for (int i = 4; i--; ) { near[i]   = mat[i][3] + mat[i][2]; }
    for (int i = 4; i--; ) { far[i]    = mat[i][3] - mat[i][2]; }
}

void GetFrustumPlanes(hmm_mat4 proj, hmm_mat4 view, hmm_vec4 frustumPlanesOut[6])
{
    hmm_mat4 projview = HMM_MultiplyMat4(proj, view);
    extract_planes_from_projviewmat(projview.Elements, 
        frustumPlanesOut[LEFT].Elements, frustumPlanesOut[RIGHT].Elements, frustumPlanesOut[BOTTOM].Elements, 
        frustumPlanesOut[TOP].Elements, frustumPlanesOut[NEAR].Elements, frustumPlanesOut[FAR].Elements);
}

// frustumPlanes contains plane coefficients (a,b,c,d) where: ax + by + cz = d
bool FrustumSphereShouldCull(hmm_vec4 frustumPlanes[6], const float sphereCenter[3], float sphereRadius)
{
    for(int i = 0; i < 6; i++)
    {
        float dist = HMM_DotVec4(HMM_Vec4(sphereCenter[0], sphereCenter[1], sphereCenter[2], 1.0), frustumPlanes[i]) + sphereRadius;
        if(dist < 0) return true; // sphere culled
    }
    return false;
}

bool CamSphereShouldCull(const Camera& cam, const float sphereCenter[3], float sphereRadius)
{
    hmm_vec4 frustumPlanes[6];
    GetFrustumPlanes(GetCameraProjectionMatrix(cam), GetCameraViewMatrix(cam), frustumPlanes);
    return FrustumSphereShouldCull(frustumPlanes, sphereCenter, sphereRadius);
}