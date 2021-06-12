#pragma once
#include "core.h"
#include "EvDevice.h"

class EvTexture : NoCopy {
    EvDevice& device;
    VkImage vkImage;
    VmaAllocation vkImageMemory;
    VkImageView vkImageView;
    VkSampler vkSampler;
    uint32_t mipLevels;

    static void loadFile(const std::string &filename, unsigned char** pixels, int* width, int* height);
    void createImage(unsigned char* pixels, int width, int height);
    void generateMipmaps(uint32_t width, uint32_t height, VkFormat format);
    void createSampler();

public:
    EvTexture(EvDevice& device, unsigned char* pixels, int width, int height, VkFormat format);
    static std::unique_ptr<EvTexture> fromFile(EvDevice& device, const std::string& filename, VkFormat format);
    static std::unique_ptr<EvTexture> fromIntColor(EvDevice& device, uint32_t color);
    ~EvTexture();

    VkDescriptorImageInfo getDescriptorInfo() const;
};
