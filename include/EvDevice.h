#pragma once

#include "core.h"
#include "EvWindow.h"

class EvDevice;

struct EvDeviceInfo {
    std::set<const char*> validationLayers;
    std::set<const char*> instanceExtensions;
    std::set<const char*> deviceExtensions;
    uint32_t textureDescriptorSetCount = 0;
};

struct EvFrameBufferAttachment {
    VkImage image;
    VmaAllocation imageMemory;
    VkImageView view;
    VkFormat format;

    void destroy(EvDevice& device);
};

struct EvFrameBuffer : NoCopy {
    uint32_t width, height;
    std::vector<VkFramebuffer> vkFrameBuffers;
    VkRenderPass vkRenderPass;

    virtual void destroy(EvDevice& device);
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
    void createDescriptorPool();

public:
    EvDeviceInfo info;
    EvWindow& window;
    VkInstance vkInstance;
    VkSurfaceKHR vkSurface;
    VkPhysicalDevice vkPhysicalDevice;
    VkPhysicalDeviceProperties vkPhysicalDeviceProperties;
    VkPhysicalDeviceFeatures vkPhysicalDeviceFeatures;
    VkPhysicalDeviceMemoryProperties vkPhysicalDeviceMemoryProperties;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    QueueFamilyIndices queueFamilyIndices;
    VkDevice vkDevice;
    VmaAllocator vmaAllocator;
    VkCommandPool vkCommandPool;
    VkDescriptorPool vkDescriptorPool;

    VkQueue computeQueue;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    EvDevice(EvDeviceInfo info, EvWindow &window);
    ~EvDevice();

    VkFormat findSupportedFormat ( const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
    VkFormat findDepthFormat() const;

    void createDeviceImage(VkImageCreateInfo imageInfo, VkImage *image, VmaAllocation *memory);
    void createDeviceCubemap(uint32_t width, uint32_t height, uchar **dataLayers, VkImage* image, VmaAllocation* imageMemory, VkImageView* imageView);
    void createAttachment(VkFormat format, VkImageUsageFlagBits usage, VkSampleCountFlagBits samples, uint32_t width, uint32_t height, EvFrameBufferAttachment* attachment);
    SwapchainSupportDetails getSwapchainSupportDetails() const;
    VkShaderModule createShaderModule(const char* filepath) const;
    void createHostBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer* buffer, VmaAllocation* bufferMemory);
    void createDeviceBuffer(VkDeviceSize size, void* data, VkBufferUsageFlags usage, VkBuffer* buffer, VmaAllocation* bufferMemory);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void copyBuffer(VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size);
    void copyBufferToImage(VkImage dst, VkBuffer src, VkBufferImageCopy copyInfo);
    void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels,
                               uint32_t arrayLayers);
};
