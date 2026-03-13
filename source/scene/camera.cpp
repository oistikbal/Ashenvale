#include "camera.h"
#include "window/input.h"

using namespace DirectX;

void ash::cam_update_view_mat(camera &cam)
{
    XMMATRIX rotation_matrix = XMMatrixRotationRollPitchYaw(cam.rotation.x, cam.rotation.y, cam.rotation.z);

    XMVECTOR forward = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), rotation_matrix);

    XMVECTOR eye = XMLoadFloat3(&cam.position);
    XMVECTOR up = XMVectorSet(0, 1, 0, 0);

    XMStoreFloat4x4(&cam.mat_view, XMMatrixLookToLH(eye, forward, up));
}

void ash::cam_update_proj_mat(camera &cam, float fov_y, float screen_aspect, float screen_near, float screen_far)
{
    XMStoreFloat4x4(&cam.mat_proj, XMMatrixPerspectiveFovLH(fov_y, screen_aspect, screen_near, screen_far));
}

DirectX::XMMATRIX ash::cam_get_view_proj_mat(const camera &cam)
{
    return (XMLoadFloat4x4(&cam.mat_view) * XMLoadFloat4x4(&cam.mat_proj));
}

void ash::cam_handle_input(camera &cam, float delta_time, const win_input::input_state &input_state)
{
    const float move_speed = 2.0f * delta_time;
    const float mouse_sensitivity = 1.0f * delta_time;

    if (input_state.mouse_button[1])
    {
        cam.rotation.y += input_state.mouse_delta_pos[0] * mouse_sensitivity;
        cam.rotation.x += input_state.mouse_delta_pos[1] * mouse_sensitivity;
    }

    if (cam.rotation.x < -XM_PIDIV2 + 0.01f)
        cam.rotation.x = -XM_PIDIV2 + 0.01f;
    if (cam.rotation.x > XM_PIDIV2 - 0.01f)
        cam.rotation.x = XM_PIDIV2 - 0.01f;

    XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(cam.rotation.x, cam.rotation.y, cam.rotation.z);
    XMVECTOR forward = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), rotationMatrix);
    XMVECTOR right = XMVector3TransformNormal(XMVectorSet(1, 0, 0, 0), rotationMatrix);

    XMVECTOR move = XMVectorZero();
    if (input_state.keyboard['W'])
        move = XMVectorAdd(move, forward);
    if (input_state.keyboard['S'])
        move = XMVectorSubtract(move, forward);
    if (input_state.keyboard['A'])
        move = XMVectorSubtract(move, right);
    if (input_state.keyboard['D'])
        move = XMVectorAdd(move, right);

    XMVECTOR position = XMLoadFloat3(&cam.position);
    const float move_len_sq = XMVectorGetX(XMVector3LengthSq(move));
    if (move_len_sq > 0.0f)
    {
        const XMVECTOR move_dir = XMVector3Normalize(move);
        position = XMVectorAdd(position, XMVectorScale(move_dir, move_speed));
    }

    XMStoreFloat3(&cam.position, position);
}
