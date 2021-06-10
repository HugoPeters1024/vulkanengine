#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include "EvDevice.h"
#include "EvSwapchain.h"

struct UIInfo {
    float fps = 0;
    float gravity;
};

class EvOverlay {
private:
    EvDevice& device;
    VkDescriptorPool imguiPool;

    void createDescriptorPool();
    void initImGui(VkRenderPass renderPass, uint32_t nrImages);

public:
    UIInfo uiInfo;
    EvOverlay(EvDevice &device, VkRenderPass renderPass, uint32_t nrImages);
    ~EvOverlay();

    void NewFrame();
    void Draw(VkCommandBuffer commandBuffer);
};