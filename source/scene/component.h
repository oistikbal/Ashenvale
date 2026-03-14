#pragma once

#include <DirectXMath.h>
#include <flecs.h>
#include <vector>

namespace ash
{
struct game_object
{
};

struct transform
{
    DirectX::XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
    DirectX::XMFLOAT4 rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    DirectX::XMFLOAT3 scale = {1.0f, 1.0f, 1.0f};
};

struct mesh_component
{
    uint32_t handle = {};
};

struct material
{
    uint32_t albedo = {};
    uint32_t normal = {};
};

inline DirectX::XMMATRIX get_local_transform_matrix(const ash::transform &transform)
{
    const DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(transform.scale.x, transform.scale.y, transform.scale.z);
    const DirectX::XMVECTOR rotation = XMLoadFloat4(&transform.rotation);
    const DirectX::XMMATRIX rotation_matrix = DirectX::XMMatrixRotationQuaternion(rotation);
    const DirectX::XMMATRIX translation =
        DirectX::XMMatrixTranslation(transform.position.x, transform.position.y, transform.position.z);
    return scale * rotation_matrix * translation;
}

inline DirectX::XMMATRIX get_world_transform_matrix(flecs::entity entity)
{
    std::vector<flecs::entity> chain;
    for (flecs::entity cursor = entity; cursor.is_valid(); cursor = cursor.parent())
    {
        chain.push_back(cursor);
    }

    DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();
    for (auto it = chain.rbegin(); it != chain.rend(); ++it)
    {
        if (it->has<ash::transform>())
        {
            const ash::transform &node_transform = it->get<ash::transform>();
            world = get_local_transform_matrix(node_transform) * world;
        }
    }

    return world;
}
} // namespace ash
