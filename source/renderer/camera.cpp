#include "renderer/camera.h"

using namespace DirectX;

void ashenvale::renderer::camera::initialize()
{
    g_position = {0.0f, 0.0f, -1.0f};
    g_rotation = {0.0f, 0.0f, 0.0f};

    XMVECTOR eye = XMLoadFloat3(&g_position);
    XMVECTOR forward = XMVectorSet(0, 0, 1, 0);
    XMVECTOR up = XMVectorSet(0, 1, 0, 0);

    g_viewMatrix = XMMatrixLookToLH(eye, forward, up);
}

void ashenvale::renderer::camera::update(float fovY, float aspectRatio, float nearZ, float farZ)
{
    XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(g_rotation.x, g_rotation.y, g_rotation.z);

    XMVECTOR forward = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), rotationMatrix);

    XMVECTOR eye = XMLoadFloat3(&g_position);
    XMVECTOR up = XMVectorSet(0, 1, 0, 0);

    g_viewMatrix = XMMatrixLookToLH(eye, forward, up);
    g_projectionMatrix = XMMatrixPerspectiveFovLH(fovY, aspectRatio, nearZ, farZ);
    g_viewProjectionMatrix = XMMatrixMultiply(g_viewMatrix, g_projectionMatrix);
}