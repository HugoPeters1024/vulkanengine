#include "EvTexture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

EvTexture::EvTexture(EvDevice &device, unsigned char *pixels, int width, int height)
    : device(device) {
    createImage(pixels, width, height);
    createSampler();
}

std::unique_ptr<EvTexture> EvTexture::fromFile(EvDevice &device, const std::string &filename) {
    unsigned char* pixels;
    int width, height;
    loadFile(filename, &pixels, &width, &height);
    auto ret = std::make_unique<EvTexture>(device, pixels, width, height);
    stbi_image_free(pixels);
    return std::move(ret);
}

std::unique_ptr<EvTexture> EvTexture::fromIntColor(EvDevice &device, uint32_t color) {
    int width = 1;
    int height = 1;
    return std::make_unique<EvTexture>(device, reinterpret_cast<unsigned char*>(&color), width, height);
}

std::unique_ptr<EvTexture> EvTexture::fromVec4Color(EvDevice &device, glm::vec4 color) {
    int width = 1;
    int height = 1;
    unsigned char pixels[4];
    pixels[0] = static_cast<char>(std::clamp(color.r * 256.0f, 0.0f, 255.0f));
    pixels[1] = static_cast<char>(std::clamp(color.g * 256.0f, 0.0f, 255.0f));
    pixels[2] = static_cast<char>(std::clamp(color.b * 256.0f, 0.0f, 255.0f));
    pixels[3] = static_cast<char>(std::clamp(color.a * 256.0f, 0.0f, 255.0f));
    return std::make_unique<EvTexture>(device, pixels, width, height);
}

EvTexture::~EvTexture() {
    printf("Destroying texture\n");
    vmaDestroyImage(device.vmaAllocator, vkImage, vkImageMemory);
    vkDestroyImageView(device.vkDevice, vkImageView, nullptr);
    vkDestroySampler(device.vkDevice, vkSampler, nullptr);
}

void EvTexture::loadFile(const std::string &filename, unsigned char **pixels, int *width, int *height) {
    int texChannels;
    *pixels = static_cast<unsigned char*>(stbi_load(filename.c_str(), width, height, &texChannels, STBI_rgb_alpha));

    if (!*pixels) {
        throw std::runtime_error("failed to load texture " + filename);
    }
}

void EvTexture::createImage(unsigned char* pixels, int width, int height) {
    VkDeviceSize imageSize = width * height * 4 * sizeof(stbi_uc);
    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferMemory;
    device.createHostBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &stagingBuffer, &stagingBufferMemory);

    void* data;
    vmaMapMemory(device.vmaAllocator, stagingBufferMemory, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vmaUnmapMemory(device.vmaAllocator, stagingBufferMemory);

    auto format = VK_FORMAT_R8G8B8A8_SRGB;

    device.createImage(
            width, height,
            format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_SAMPLE_COUNT_1_BIT,
            &vkImage, &vkImageMemory);

    device.transitionImageLayout(vkImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    device.copyBufferToImage(vkImage, stagingBuffer, width, height);
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

VkDescriptorImageInfo EvTexture::getDescriptorInfo() const {
    return VkDescriptorImageInfo {
        .sampler = vkSampler,
        .imageView = vkImageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
}

