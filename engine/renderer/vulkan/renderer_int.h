#ifndef RENDERER_INT_H
#define RENDERER_INT_H
#include <stdbool.h>
#include "interface.h"

bool init_vulkan(Interface *func);
bool init_surface(Interface *func);
bool init_device(Interface *func);
bool init_swapchain(Interface *func);
bool init_render(Interface *func);
bool init_render_pass(Interface *func);
bool init_framebuffers(Interface *func);
bool init_shaders(Interface *func);
bool init_pipeline(Interface *func);

#endif
