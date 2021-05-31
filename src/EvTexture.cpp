#include "EvTexture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

EvTexture::EvTexture(EvDevice &device, const std::string &filename)
    : device(device) {
    createImage(filename);
    createSampler();
}

EvTexture::~EvTexture() {
    printf("Destroying texture\n");
    vmaDestroyImage(device.vmaAllocator, vkImage, vkImageMemory);
    vkDestroyImageView(device.vkDevice, vkImageView, nullptr);
    vkDestroySampler(device.vkDevice, vkSampler, nullptr);
}

void EvTexture::createImage(const std::string &filename) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4 * sizeof(stbi_uc);

    if (!pixels) {
        throw std::runtime_error("failed to load texture " + filename);
    }

    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferMemory;
    device.createHostBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &stagingBuffer, &stagingBufferMemory);

    void* data;
    vmaMapMemory(device.vmaAllocator, stagingBufferMemory, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vmaUnmapMemory(device.vmaAllocator, stagingBufferMemory);
    stbi_image_free(pixels);

    auto format = VK_FORMAT_R8G8B8A8_SRGB;

    device.createImage(
            texWidth, texHeight,
            format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_SAMPLE_COUNT_1_BIT,
            &vkImage, &vkImageMemory);

    device.transitionImageLayout(vkImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    device.copyBufferToImage(vkImage, stagingBuffer, texWidth, texHeight);
    device.transitionImageLayout(vkImage, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vmaDestroyBuffer(device.vmaAllocator, stagingBuffer, stagingBufferMemory);

    device.createImageView(vkImage, format, VK_IMAGE_ASPECT_COLOR_BIT, &vkImageView);
}

void EvTexture::createSampler() {
    VkSamplerCreateInfo samplerInfo {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = device.vkPhysicalDeviceProperties.limits.maxSamplerAnisotropy,
        .unnormalizedCoordinates = VK_FALSE,
    };

    vkCheck(vkCreateSampler(device.vkDevice, &samplerInfo, nullptr, &vkSampler));
}

VkDescriptorSetLayoutBinding EvTexture::defaultBinding(uint32_t binding) {
    return VkDescriptorSetLayoutBinding {
        .binding = binding,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };
}

VkDescriptorImageInfo EvTexture::getDescriptorInfo() const {
    return VkDescriptorImageInfo {
        .sampler = vkSampler,
        .imageView = vkImageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
}

