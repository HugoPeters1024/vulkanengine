#pragma once

#include <vulkan/vulkan.h>
#include <set>

#include "utils.hpp"
#include "UtilsPhysicalDevice.hpp"
#include "EvWindow.h"
#include "vk_mem_alloc.hpp"

struct EvDeviceInfo {
    std::set<const char*> validationLayers;
    std::set<const char*> instanceExtensions;
    std::set<const char*> deviceExtensions;
};

class EvDevice : NoCopy {
private:
    bool isDeviceSuitable(VkPhysicalDevice physicalDevice) const;
    void finalizeInfo();
    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createAllocator();
    void createCommandPool();

public:
    EvDeviceInfo info;
    EvWindow& window;
    VkInstance vkInstance;
    VkSurfaceKHR vkSurface;
    VkPhysicalDevice vkPhysicalDevice;
    VkPhysicalDeviceProperties vkPhysicalDeviceProperties;
    VkPhysicalDeviceFeatures vkPhysicalDeviceFeatures;
    VkPhysicalDeviceMemoryProperties vkPhysicalDeviceMemoryProperties;
    QueueFamilyIndices queueFamilyIndices;
    VkDevice vkDevice;
    VmaAllocator vmaAllocator;
    VkCommandPool vkCommandPool;

    VkQueue computeQueue;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    EvDevice(EvDeviceInfo info, EvWindow &window);
    ~EvDevice();

    VkFormat findSupportedFormat ( const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
    VkFormat findDepthFormat() const;

    void createImage(uint width, uint height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImage* image, VmaAllocation* memory);
    void createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView* imageView);
    SwapchainSupportDetails getSwapchainSupportDetails() const;
    VkShaderModule createShaderModule(const char* filepath) const;
    void createHostBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer* buffer, VmaAllocation* bufferMemory);
    void createDeviceBuffer(VkDeviceSize size, void* data, VkBufferUsageFlags usage, VkBuffer* buffer, VmaAllocation* bufferMemory);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void copyBuffer(VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size);
};
