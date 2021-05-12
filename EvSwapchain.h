#pragma once

#include "EvDevice.h"

class EvSwapchain {
private:
    EvDevice& device;
    const int MAX_FRAMES_IN_FLIGHT = 2;
    uint currentFrame = 0;
    bool resized = true;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;

    void createSwapchain();
    void createImageViews();
    void createDepthResources();
    void createRenderpass();
    void createFramebuffers();
    void createSyncObjects();

    VkSurfaceFormatKHR chooseSwapSurfaceFormat() const;
    VkPresentModeKHR  chooseSwapPresentMode() const;
    VkExtent2D chooseSwapExtent() const;
public:
    VkSwapchainKHR vkSwapchain;
    std::vector<VkImage> vkImages;
    VkImage vkDepthImage;
    VkDeviceMemory vkDepthImageMemory;
    std::vector<VkImageView> vkImageViews;
    VkImageView vkDepthImageView;
    std::vector<VkFramebuffer> vkFramebuffers;
    VkSurfaceFormatKHR surfaceFormat;
    VkExtent2D extent;
    VkRenderPass vkRenderPass;

    EvSwapchain(EvDevice& device);
    ~EvSwapchain();

    void presentCommandBuffer(const std::vector<VkCommandBuffer>& commandBuffers);
};
