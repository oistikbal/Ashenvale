#pragma once
#include <renderer/render_pass.h>

namespace ashenvale::renderer::render_pass::normal
{
void initialize();
void execute(const render_pass_context &context);
} // namespace ashenvale::renderer::render_pass::normal