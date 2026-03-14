#pragma once
#include "camera.h"
#include <filesystem>
#include <flecs.h>
#include <scene/component.h>
#include <string>
#include <string_view>
#include <vector>

namespace ash
{
struct scene_object
{
    uint32_t mesh_handle;
    material mat;
    DirectX::XMFLOAT4X4 model;
};

struct scene_data
{
    DirectX::XMFLOAT4X4 view_proj;
    std::vector<scene_object> objects;
};

inline flecs::world scene_g_world;
inline flecs::entity scene_g_selected;
} // namespace ash

namespace ash
{
flecs::entity scene_create_empty(flecs::entity parent = {});
std::string scene_make_unique_name(std::string_view desired_name, flecs::entity parent = {}, flecs::entity ignore = {});
void scene_set_entity_name_safe(flecs::entity entity, std::string_view desired_name);
bool scene_load_gltf(const std::filesystem::path &path);
scene_data get_scene_data(const flecs::world &world, const ash::camera &cam);
void scene_update();
void scene_init();
void scene_shutdown();
} // namespace ash
