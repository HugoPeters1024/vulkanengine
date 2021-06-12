#include "EvTexture.h"

EvTexture::EvTexture(EvDevice &device, unsigned char *pixels, int width, int height, VkFormat format)
    : device(device) {
    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
    createImage(pixels, width, height);
    createSampler();
}

std::unique_ptr<EvTexture> EvTexture::fromFile(EvDevice &device, const std::string &filename, VkFormat format) {
    unsigned char* pixels;
    int width, height;
    loadFile(filename, &pixels, &width, &height);
    auto ret = std::make_unique<EvTexture>(device, pixels, width, height, format);
    stbi_image_free(pixels);
    return std::move(ret);
}

std::unique_ptr<EvTexture> EvTexture::fromIntColor(EvDevice &device, uint32_t color) {
    union {
        uint32_t dcolor;
        uchar data[4];
    } packed{};
    packed.dcolor = color;
    int width = 1;
    int height = 1;
    return std::make_unique<EvTexture>(device, packed.data, width, height, VK_FORMAT_R8G8B8A8_UNORM);
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

    auto format = VK_FORMAT_R8G8B8A8_UNORM;
    auto imageInfo = vks::initializers::imageCreateInfo(width, height, format,
                                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    imageInfo.mipLevels = mipLevels;
    device.createDeviceImage(imageInfo, &vkImage, &vkImageMemory);

    device.transitionImageLayout(vkImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
    device.copyBufferToImage(vkImage, stagingBuffer, width, height);
    // also transitions the layout to shader read optimal.
    generateMipmaps(width, height, format);

    vmaDestroyBuffer(device.vmaAllocator, stagingBuffer, stagingBufferMemory);

    auto viewInfo = vks::initializers::imageViewCreateInfo(vkImage, format, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCheck(vkCreateImageView(device.vkDevice, &viewInfo, nullptr, &vkImageView));
}

void EvTexture::generateMipmaps(uint32_t width, uint32_t height, VkFormat format) {
    // Check if format support linear blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(device.vkPhysicalDevice, format, &formatProperties);
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("system does not support linear blitting for this format, implement an alternative");
    }
    auto cmdBuffer = device.beginSingleTimeCommands();

    VkImageMemoryBarrier barrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = vkImage,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };

    auto mipWidth = static_cast<int32_t>(width);
    auto mipHeight = static_cast<int32_t>(height);

    for(uint32_t i=1; i<mipLevels; i++) {
        // First ensure that we are in transfer src optimal
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

        // copy the mip from use to the next level
        VkImageBlit blit {
            .srcSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i - 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .srcOffsets { { 0, 0, 0 }, { mipWidth, mipHeight, 1 } },
            .dstSubresource {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .dstOffsets = { {0, 0, 0}, { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2: 1, 1} },
        };

        vkCmdBlitImage(cmdBuffer, vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        // ensure we are ready for shader reading.
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    // Finally, ensure that the final mip level is also ready for reading.
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);

    device.endSingleTimeCommands(cmdBuffer);
}

void EvTexture::createSampler() {
    VkSamplerCreateInfo samplerInfo {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = device.vkPhysicalDeviceProperties.limits.maxSamplerAnisotropy,
        .minLod = 5.0f,
        .maxLod = static_cast<float>(mipLevels),
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

