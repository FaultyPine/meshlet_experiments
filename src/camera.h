#pragma once

#include "external/HandmadeMath.h"

enum CameraType
{
    NONE,
    FREECAM,
    ORBITING,
};

struct Camera
{
    hmm_vec3 pos;
    hmm_vec3 front;
    hmm_vec3 lookat_override;
    bool should_override_lookat;
    float orbit_speed;
    float yaw;
    float pitch;
    float FOV;
    float nearClip;
    float farClip;
    CameraType type;
};

enum FrustumPlanes
{
    LEFT, RIGHT, TOP, BOTTOM, NEAR, FAR, NUM_FRUSTUM_PLANES,
};

void InitializeCamera(Camera& cam, CameraType type = CameraType::FREECAM);
void CameraTick(Camera& cam);
hmm_mat4 GetCameraViewMatrix(const Camera& cam);
hmm_mat4 GetCameraProjectionMatrix(const Camera& cam);
hmm_vec3 GetNormalizedLookDir(float yaw, float pitch);

struct LinesRenderer;
void DrawCamFrustum(const Camera& cam, LinesRenderer& renderer, hmm_vec3 color);
// outputs frustum planes in form plane coefficients (a,b,c,d) where: ax + by + cz = d
void GetFrustumPlanes(hmm_mat4 proj, hmm_mat4 view, hmm_vec4 frustumPlanesOut[6]);

// TODO: process a batch of spheres
// returns true if sphere should be culled - i.e. is outside the camera's frustum
bool CamSphereShouldCull(const Camera& cam, const float sphereCenter[3], float sphereRadius);