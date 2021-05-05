#pragma once

#include "EvDevice.h"

class EvSwapchain {
private:
    const EvDevice& device;

    void createSwapchain();

    VkSurfaceFormatKHR chooseSwapSurfaceFormat() const;
    VkPresentModeKHR  chooseSwapPresentMode() const;
    VkExtent2D chooseSwapExtent() const;
public:
    VkSwapchainKHR vkSwapchain;
    std::vector<VkImage> vkImages;
    VkSurfaceFormatKHR surfaceFormat;
    VkExtent2D extent;

    EvSwapchain(const EvDevice& device);
    ~EvSwapchain();
};
