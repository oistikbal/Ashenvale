#pragma once

#include "window/input.h"
#include <DirectXMath.h>

namespace ash
{
struct camera
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 rotation;
    DirectX::XMFLOAT4X4 mat_proj;
    DirectX::XMFLOAT4X4 mat_view;
};
inline camera g_camera;
} // namespace ash

namespace ash
{
void cam_update_view_mat(camera &cam);
void cam_update_proj_mat(camera &cam, float fov_y, float screen_aspect, float screen_near, float screen_far);
DirectX::XMMATRIX cam_get_view_proj_mat(const camera &cam);
void cam_handle_input(camera &cam, float delta_time, const win_input::input_state &input_state);
} // namespace ash
