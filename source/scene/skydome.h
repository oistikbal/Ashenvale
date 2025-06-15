#pragma once

#include <DirectXMath.h>
#include <d3d11_4.h>
#include <winrt/base.h>


namespace ashenvale::scene::skydome
{
void initialize();
void load_hdri_skydome(const char *path);
void render();
void close();
}