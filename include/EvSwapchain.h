#pragma once

#include "EvDevice.h"

class EvSwapchain {
private:
    EvDevice& device;
    const int MAX_FRAMES_IN_FLIGHT = 3;
    uint currentFrame = 0;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    std::shared_ptr<EvSwapchain> oldSwapchain;

    void init();
    void createSwapchain();
    void createImageViews();
    void createSyncObjects();

    VkSurfaceFormatKHR chooseSwapSurfaceFormat() const;
    VkPresentModeKHR  chooseSwapPresentMode() const;
    VkExtent2D chooseSwapExtent() const;

public:
    VkSwapchainKHR vkSwapchain;
    std::vector<VkImage> vkImages;
    std::vector<VkImageView> vkImageViews;
    VkSurfaceFormatKHR surfaceFormat;
    VkExtent2D extent;

    EvSwapchain(EvDevice& device);
    EvSwapchain(EvDevice& device, std::shared_ptr<EvSwapchain> previous);
    ~EvSwapchain();

    VkResult acquireNextSwapchainImage(uint32_t* imageIndex);
    VkResult presentCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
};
