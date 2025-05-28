#pragma once

#include <DirectXMath.h>

namespace ashenvale::renderer::camera
{
    /// View matrix in column-major order
    inline DirectX::XMMATRIX g_viewMatrix;
    /// Projection matrix in column-major order
    inline DirectX::XMMATRIX g_projectionMatrix;
    /// View Projection matrix in column-major order
    inline DirectX::XMMATRIX g_viewProjectionMatrix;

    inline DirectX::XMFLOAT3 g_position;
    inline DirectX::XMFLOAT3 g_rotation;
}

namespace ashenvale::renderer::camera
{
    void initialize();
    void update(float fovY, float aspectRatio, float nearZ, float farZ);
}