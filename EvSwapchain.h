#pragma once

#include "EvDevice.h"

class EvSwapchain {
private:
    const EvDevice& device;

    void createSwapchain();
    void createRenderpass();

    VkSurfaceFormatKHR chooseSwapSurfaceFormat() const;
    VkPresentModeKHR  chooseSwapPresentMode() const;
    VkExtent2D chooseSwapExtent() const;
public:
    VkSwapchainKHR vkSwapchain;
    std::vector<VkImage> vkImages;
    VkSurfaceFormatKHR surfaceFormat;
    VkExtent2D extent;
    VkRenderPass vkRenderPass;

    EvSwapchain(const EvDevice& device);
    ~EvSwapchain();
};
