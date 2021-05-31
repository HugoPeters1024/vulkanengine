#pragma once
#include <vulkan/vulkan.h>
#include "utils.hpp"
#include "EvDevice.h"

class EvTexture : NoCopy {
    EvDevice& device;
    VkImage vkImage;
    VmaAllocation vkImageMemory;
    VkImageView vkImageView;
    VkSampler vkSampler;

    void createImage(const std::string &filename);
    void createSampler();

    static VkDescriptorSetLayoutBinding defaultBinding(uint32_t binding);
public:
    EvTexture(EvDevice& device, const std::string& filename);
    ~EvTexture();

    VkDescriptorImageInfo getDescriptorInfo() const;
};
