#pragma once

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include "EvDevice.h"
#include "EvSwapchain.h"

class EvOverlay {
private:
    EvDevice& device;
    VkDescriptorPool imguiPool;

    void createDescriptorPool();
    void initImGui(const EvSwapchain &swapchain);

public:
    EvOverlay(EvDevice &device, const EvSwapchain &swapchain);
    ~EvOverlay();

    void NewFrame();
    void Draw(VkCommandBuffer commandBuffer);
};