#pragma once

#include "external/HandmadeMath.h"

struct Camera
{
    hmm_vec3 pos;
    hmm_vec3 front;
    hmm_vec3 lookat_override;
    bool should_override_lookat;
    float orbit_speed;
    float yaw;
    float pitch;
};

enum FrustumPlanes
{
    LEFT, RIGHT, TOP, BOTTOM, NEAR, FAR, NUM_FRUSTUM_PLANES,
};

void InitializeCamera(Camera& cam);
void CameraTick(Camera& cam);
hmm_mat4 GetCameraViewMatrix(Camera& cam);
hmm_vec3 GetNormalizedLookDir(float yaw, float pitch);