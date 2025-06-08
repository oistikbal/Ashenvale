#pragma once

namespace ashenvale::editor::inspector
{
inline bool g_isOpen = true;
inline void* g_selectedItem = nullptr;
} // namespace ashenvale::editor::inspector

namespace ashenvale::editor::inspector
{
void render();
}