#pragma once

#include <flecs.h>

namespace ashenvale::editor::inspector
{
inline bool g_isOpen = true;
inline flecs::entity g_selectedEntity = flecs::entity::null();
} // namespace ashenvale::editor::inspector

namespace ashenvale::editor::inspector
{
void initialize();
void render();
}