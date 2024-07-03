#include "camera.h"

#include "input.h"
#include "external/sokol/sokol_app.h"

#define HANDMADE_MATH_IMPLEMENTATION
#include "external/HandmadeMath.h"


void InitializeCamera(Camera& cam)
{
    cam.pos = {0, 0.5, 2.5};
    cam.orbit_speed = 0.2f;
    
    cam.should_override_lookat = true;
    cam.lookat_override = HMM_Vec3(0,0,0);
}

void CameraTick(Camera& cam)
{
    constexpr float speed = 0.1f;
    hmm_vec3& cam_pos = cam.pos;
    hmm_vec4 rotated = HMM_MultiplyMat4ByVec4(HMM_Rotate(cam.orbit_speed, HMM_Vec3(0,1,0)), HMM_Vec4(cam_pos.X, cam_pos.Y, cam_pos.Z, 1.0));
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
}

hmm_mat4 GetCameraViewMatrix(Camera& cam)
{
    hmm_vec3 center = HMM_AddVec3(cam.pos, cam.front);
    if (cam.should_override_lookat)
    {
        center = cam.lookat_override;
        cam.front = HMM_SubtractVec3(center, cam.pos);
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



template <typename fptype>
bool InvertMatrix(const fptype* m, fptype* invOut)
{
    double inv[16], det;
    int i;

    inv[0] = m[5]  * m[10] * m[15] - 
             m[5]  * m[11] * m[14] - 
             m[9]  * m[6]  * m[15] + 
             m[9]  * m[7]  * m[14] +
             m[13] * m[6]  * m[11] - 
             m[13] * m[7]  * m[10];

    inv[4] = -m[4]  * m[10] * m[15] + 
              m[4]  * m[11] * m[14] + 
              m[8]  * m[6]  * m[15] - 
              m[8]  * m[7]  * m[14] - 
              m[12] * m[6]  * m[11] + 
              m[12] * m[7]  * m[10];

    inv[8] = m[4]  * m[9] * m[15] - 
             m[4]  * m[11] * m[13] - 
             m[8]  * m[5] * m[15] + 
             m[8]  * m[7] * m[13] + 
             m[12] * m[5] * m[11] - 
             m[12] * m[7] * m[9];

    inv[12] = -m[4]  * m[9] * m[14] + 
               m[4]  * m[10] * m[13] +
               m[8]  * m[5] * m[14] - 
               m[8]  * m[6] * m[13] - 
               m[12] * m[5] * m[10] + 
               m[12] * m[6] * m[9];

    inv[1] = -m[1]  * m[10] * m[15] + 
              m[1]  * m[11] * m[14] + 
              m[9]  * m[2] * m[15] - 
              m[9]  * m[3] * m[14] - 
              m[13] * m[2] * m[11] + 
              m[13] * m[3] * m[10];

    inv[5] = m[0]  * m[10] * m[15] - 
             m[0]  * m[11] * m[14] - 
             m[8]  * m[2] * m[15] + 
             m[8]  * m[3] * m[14] + 
             m[12] * m[2] * m[11] - 
             m[12] * m[3] * m[10];

    inv[9] = -m[0]  * m[9] * m[15] + 
              m[0]  * m[11] * m[13] + 
              m[8]  * m[1] * m[15] - 
              m[8]  * m[3] * m[13] - 
              m[12] * m[1] * m[11] + 
              m[12] * m[3] * m[9];

    inv[13] = m[0]  * m[9] * m[14] - 
              m[0]  * m[10] * m[13] - 
              m[8]  * m[1] * m[14] + 
              m[8]  * m[2] * m[13] + 
              m[12] * m[1] * m[10] - 
              m[12] * m[2] * m[9];

    inv[2] = m[1]  * m[6] * m[15] - 
             m[1]  * m[7] * m[14] - 
             m[5]  * m[2] * m[15] + 
             m[5]  * m[3] * m[14] + 
             m[13] * m[2] * m[7] - 
             m[13] * m[3] * m[6];

    inv[6] = -m[0]  * m[6] * m[15] + 
              m[0]  * m[7] * m[14] + 
              m[4]  * m[2] * m[15] - 
              m[4]  * m[3] * m[14] - 
              m[12] * m[2] * m[7] + 
              m[12] * m[3] * m[6];

    inv[10] = m[0]  * m[5] * m[15] - 
              m[0]  * m[7] * m[13] - 
              m[4]  * m[1] * m[15] + 
              m[4]  * m[3] * m[13] + 
              m[12] * m[1] * m[7] - 
              m[12] * m[3] * m[5];

    inv[14] = -m[0]  * m[5] * m[14] + 
               m[0]  * m[6] * m[13] + 
               m[4]  * m[1] * m[14] - 
               m[4]  * m[2] * m[13] - 
               m[12] * m[1] * m[6] + 
               m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] + 
              m[1] * m[7] * m[10] + 
              m[5] * m[2] * m[11] - 
              m[5] * m[3] * m[10] - 
              m[9] * m[2] * m[7] + 
              m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] - 
             m[0] * m[7] * m[10] - 
             m[4] * m[2] * m[11] + 
             m[4] * m[3] * m[10] + 
             m[8] * m[2] * m[7] - 
             m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] + 
               m[0] * m[7] * m[9] + 
               m[4] * m[1] * m[11] - 
               m[4] * m[3] * m[9] - 
               m[8] * m[1] * m[7] + 
               m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] - 
              m[0] * m[6] * m[9] - 
              m[4] * m[1] * m[10] + 
              m[4] * m[2] * m[9] + 
              m[8] * m[1] * m[6] - 
              m[8] * m[2] * m[5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if (det == 0)
        return false;

    det = 1.0 / det;

    for (i = 0; i < 16; i++)
        invOut[i] = inv[i] * det;

    return true;
}

void GetFrustumVertices(hmm_mat4 projection, hmm_mat4 view, hmm_vec3 frustumVertsOut[8])
{
    hmm_mat4 clip = HMM_MultiplyMat4(projection, view);
    hmm_mat4 invClip;
    bool result = InvertMatrix<float>((float*)&clip.Elements[0], (float*)&invClip.Elements[0]);
    SOKOL_ASSERT(result);
    static hmm_vec3 _cameraFrustumCornerVertices[8] =
    {
        // near
        {-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1},  {-1,  1, -1},
        // far
        {-1, -1, 1},	{ 1, -1, 1},	{ 1,  1, 1},  {-1,  1, 1}
    };
    static unsigned int frustumIndices[] = {0,1 , 1,2, 2,3, 3,0, 4,5, 5,6, 6,7, 7,4, 0,4, 1,5, 2,6, 3,7};
    hmm_vec3 camFrustumVerts[8];
    for (unsigned int i = 0; i < 8; i++)
    {
        hmm_vec3 defaultVert = _cameraFrustumCornerVertices[i];
        hmm_vec4 vert = HMM_MultiplyMat4ByVec4(invClip, HMM_Vec4(defaultVert.X, defaultVert.Y, defaultVert.Z, 1.0));
        camFrustumVerts[i] = HMM_DivideVec3f(HMM_Vec3(vert.X, vert.Y, vert.Z), vert.W);
    }
    frustumVertsOut = camFrustumVerts;
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
bool FrustumSphereShouldCull(hmm_vec4 frustumPlanes[6], hmm_vec3 point, float sphereRadius)
{
    for(int i = 0; i < 6; i++)
    {
        float dist = HMM_DotVec4(HMM_Vec4(point.X, point.Y, point.Z, 1.0), frustumPlanes[i]) + sphereRadius;
        if(dist < 0) return true; // sphere culled
    }
    return false;
}