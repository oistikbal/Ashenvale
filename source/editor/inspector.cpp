#include "inspector.h"
#include "renderer/device.h"
#include "scene/scene.h"
#include <imgui.h>

using namespace winrt;

namespace
{
com_ptr<ID3D11Buffer> g_stagingConstantBuffer;
}

void ashenvale::editor::inspector::initialize()
{
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_STAGING;
    desc.ByteWidth = sizeof(ashenvale::scene::material_constants);
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = ashenvale::renderer::device::g_device->CreateBuffer(&desc, nullptr, g_stagingConstantBuffer.put());
}

void ashenvale::editor::inspector::render()
{
    if (!g_isOpen)
        return;

    bool visible = ImGui::Begin("Inspector##", &g_isOpen);
    if (visible)
    {
        if (g_selectedEntity && g_selectedEntity.is_alive())
        {
            auto e = g_selectedEntity;
            auto name = e.get<scene::name>();
            ImGui::Text("Entity ID: %llu", static_cast<flecs::entity_t>(e.id()));
            ImGui::Text("%s", name.name.c_str());

            if (auto *transform = e.try_get<ashenvale::scene::transform>())
            {
                ashenvale::scene::transform nt = *transform;

                if (ImGui::CollapsingHeader("Transform"))
                {
                    ImGui::DragFloat3("Position", &nt.position.x, 0.1f);
                    ImGui::DragFloat3("Rotation", &nt.rotation.x, 1.0f, -180.0f, 180.0f);
                    ImGui::DragFloat3("Scale", &nt.scale.x, 0.1f);
                }

                e.set<ashenvale::scene::transform>(nt);
            }

            if (auto *meshRenderer = e.try_get<ashenvale::scene::mesh_renderer>())
            {
                ashenvale::scene::mesh_renderer nmc = *meshRenderer;

                if (ImGui::CollapsingHeader("Mesh Renderer"))
                {
                    ImGui::Text("Meshes: %zu", nmc.meshes.size());
                    ImGui::Text("Materials: %zu", nmc.materials.size());

                    if (ImGui::TreeNode("Materials"))
                    {
                        int matIndex = 0;
                        for (const auto &mat : nmc.materials)
                        {
                            std::string matLabel = "Material " + std::to_string(matIndex++) + ": " + mat.name;
                            if (ImGui::TreeNode(matLabel.c_str()))
                            {
                                int resIndex = 0;
                                for (const auto &res : mat.resources)
                                {
                                    std::string resLabel = "Resource " + std::to_string(resIndex++) + ": " + res.name;
                                    if (ImGui::TreeNode(resLabel.c_str()))
                                    {
                                        if (res.srv)
                                        {
                                            ImGui::Text("Shader Resource View:");
                                            ImGui::Image((ImTextureID)(intptr_t)res.srv.get(), ImVec2(128, 128));
                                        }

                                        if (res.constantBuffer)
                                        {
                                            if (ImGui::TreeNode("Constant Buffer"))
                                            {

                                                renderer::device::g_context->CopyResource(g_stagingConstantBuffer.get(),
                                                                                          res.constantBuffer.get());

                                                D3D11_MAPPED_SUBRESOURCE mapped;
                                                HRESULT hr = renderer::device::g_context->Map(
                                                    g_stagingConstantBuffer.get(), 0, D3D11_MAP_READ_WRITE, 0, &mapped);

                                                if (SUCCEEDED(hr))
                                                {
                                                    auto *constants =
                                                        reinterpret_cast<ashenvale::scene::material_constants *>(
                                                            mapped.pData);

                                                    ImGui::ColorEdit4("Base Color", reinterpret_cast<float *>(
                                                                                        &constants->baseColorFactor));
                                                    ImGui::SliderFloat("Metallic", &constants->metallicFactor, 0.0f,
                                                                       1.0f);
                                                    ImGui::SliderFloat("Roughness", &constants->roughnessFactor, 0.0f,
                                                                       1.0f);
                                                    ImGui::SliderFloat("Normal Scale", &constants->normalScale, 0.0f,
                                                                       2.0f);

                                                    renderer::device::g_context->Unmap(g_stagingConstantBuffer.get(),
                                                                                       0);

                                                    renderer::device::g_context->UpdateSubresource(
                                                        res.constantBuffer.get(), 0, nullptr, constants, 0, 0);
                                                }
                                                else
                                                {
                                                    ImGui::TextColored(ImVec4(1, 0, 0, 1),
                                                                       "Failed to map constant buffer!");
                                                }

                                                ImGui::TreePop();
                                            }
                                        }

                                        if (res.sampler)
                                            ImGui::Text("Has Sampler");

                                        ImGui::TreePop();
                                    }
                                }
                                ImGui::TreePop();
                            }
                        }
                        ImGui::TreePop();
                    }
                }
            }

            if (auto *light = e.try_get<ashenvale::scene::light>())
            {
                ashenvale::scene::light lt = *light;

                if (ImGui::CollapsingHeader("Light"))
                {
                    switch (light->type)
                    {
                    case scene::light::light_type::directional:
                        ImGui::Text("Type: directional");
                        break;
                    case scene::light::light_type::point:
                        ImGui::Text("Type: point");
                        ImGui::SliderFloat("Range", &lt.range, 0.0f, 100.0f, "%.2f");

                        break;
                    case scene::light::light_type::spot:
                        ImGui::Text("Type: spot");
                        ImGui::SliderFloat("Range", &lt.range, 0.0f, 100.0f, "%.2f");
                        ImGui::SliderFloat("Inner Angle", &lt.spot_inner_cone_angle, 0.0f, 180.0f);
                        ImGui::SliderFloat("Outer Angle", &lt.spot_outer_cone_angle, 0.0f, 180.0f);
                        break;
                    default:
                        break;
                    }
                    ImGui::SliderFloat("Intensity", &lt.intensity, 0.0f, 100.0f, "%.2f");
                    ImGui::ColorEdit3("Color", reinterpret_cast<float *>(&lt.color));

                    e.set<ashenvale::scene::light>(lt);
                }

            }
        }
    }
    ImGui::End();
}