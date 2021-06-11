#pragma once

// STL
#include <set>
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <cstdlib>
#include <string>
#include <vector>
#include <fstream>
#include <limits>
#include <optional>

// ECS subproject
#include <ecs/ecs.h>

// GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// VULKAN & FRIENDS
#include <vulkan/vulkan.h>
#include <VulkanInitializers.hpp>

// IMGUI
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

// STB IMAGE (implementation macro in main)
#include <stb_image.h>

// TINY OBJ LOADER
#include <tiny_obj_loader.h>

// REACTPHYSICS3D
#include <reactphysics3d/reactphysics3d.h>

// GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_SSE2
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/hash.hpp>

// OTHER UTILS
#include "utils.hpp"
#include "UtilsPhysicalDevice.hpp"
