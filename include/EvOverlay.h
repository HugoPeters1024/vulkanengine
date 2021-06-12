#pragma once

#include "core.h"
#include "EvDevice.h"
#include "EvSwapchain.h"

struct UIInfo {
    float fps = 0;
    float gravity;
    float floorScale = 1.0f;
    float forceField = 0.0f;
};

class EvOverlay {
private:
    EvDevice& device;
    VkDescriptorPool imguiPool;
    UIInfo uiInfo;

    void createDescriptorPool();
    void initImGui(VkRenderPass renderPass, uint32_t nrImages);

public:
    EvOverlay(EvDevice &device, VkRenderPass renderPass, uint32_t nrImages);
    ~EvOverlay();

    void NewFrame();
    void Draw(VkCommandBuffer commandBuffer);
    inline UIInfo& getUIInfo() { return uiInfo; }
};