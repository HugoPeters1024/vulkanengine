#include "EvDevice.h"

#include <stdexcept>
#include <cassert>
#include <utility>
#include "UtilsPhysicalDevice.hpp"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.hpp>

EvDevice::EvDevice(EvDeviceInfo info, EvWindow &window) : window(window), info(std::move(info)) {
    finalizeInfo();
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createAllocator();
    createCommandPool();
    createDescriptorPool();
}

EvDevice::~EvDevice() {
    printf("Destroying descriptor pool\n");
    vkDestroyDescriptorPool(vkDevice, vkDescriptorPool, nullptr);
    printf("Destroying commandPool\n");
    vkDestroyCommandPool(vkDevice, vkCommandPool, nullptr);
    printf("Destroying the VMA allocator\n");
    vmaDestroyAllocator(vmaAllocator);
    printf("Destroying logical device\n");
    vkDestroyDevice(vkDevice, nullptr);
    printf("Destroying surface\n");
    vkDestroySurfaceKHR(vkInstance, vkSurface, nullptr);
    printf("Destroying instance\n");
    vkDestroyInstance(vkInstance, nullptr);
}

void EvDevice::finalizeInfo() {
#ifndef NDEBUG
    info.validationLayers.insert("VK_LAYER_KHRONOS_validation");
#else
    printf("Release build, validation layers disabled.\n");
#endif
    info.deviceExtensions.insert(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    // dedicated allocations
    info.deviceExtensions.insert("VK_KHR_get_memory_requirements2");
    info.deviceExtensions.insert("VK_KHR_dedicated_allocation");

    // Flip the y axis to match OpenGL
    info.deviceExtensions.insert("VK_KHR_maintenance1");

    // Multiplication blending.
    //info.deviceExtensions.insert("VK_EXT_blend_operation_advanced");

    window.collectInstanceExtensions(info.instanceExtensions);
}

void EvDevice::createInstance() {
    std::vector<const char*> validationLayers(info.validationLayers.begin(), info.validationLayers.end());
    std::vector<const char*> instanceExtensions(info.instanceExtensions.begin(), info.instanceExtensions.end());

    if (!checkValidationLayersSupported(validationLayers)) {
        throw std::runtime_error("Not all requested validation layers supported.");
    }

    VkApplicationInfo appInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = window.name.c_str(),
        .applicationVersion = VK_MAKE_VERSION(1,0,0),
        .pEngineName = "Engine Name",
        .engineVersion = VK_MAKE_VERSION(1,0,0),
        .apiVersion = VK_API_VERSION_1_2,
    };

    VkInstanceCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .flags = 0,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
        .ppEnabledExtensionNames = instanceExtensions.data(),
    };

    vkCheck(vkCreateInstance(&createInfo, nullptr, &vkInstance));
}

void EvDevice::createSurface() {
    window.createWindowSurface(vkInstance, &vkSurface);
}

void EvDevice::pickPhysicalDevice() {
    uint deviceCount = 0;
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("No devices with vulkan support found");
    }

    VkPhysicalDevice devices[deviceCount];
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices);

    vkPhysicalDevice = devices[0];
    queueFamilyIndices = findQueueFamilies(vkPhysicalDevice, vkSurface);
    vkGetPhysicalDeviceProperties(vkPhysicalDevice, &vkPhysicalDeviceProperties);
    vkGetPhysicalDeviceFeatures(vkPhysicalDevice, &vkPhysicalDeviceFeatures);
    vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &vkPhysicalDeviceMemoryProperties);

    printf("Selected GPU: %s\n", vkPhysicalDeviceProperties.deviceName);

    if (!isDeviceSuitable(vkPhysicalDevice)) {
        throw std::runtime_error("Selected device does not support requirements");
    }

    VkSampleCountFlags sampleFlags = vkPhysicalDeviceProperties.limits.framebufferColorSampleCounts
                                   & vkPhysicalDeviceProperties.limits.framebufferDepthSampleCounts;

    for (VkSampleCountFlagBits sampleCountBit = VK_SAMPLE_COUNT_1_BIT
        ; sampleCountBit <= VK_SAMPLE_COUNT_64_BIT
        ; sampleCountBit = static_cast<VkSampleCountFlagBits>( sampleCountBit << 1))
    {
        if (sampleFlags & sampleCountBit) { msaaSamples = sampleCountBit; }
    }
}

void EvDevice::createLogicalDevice() {
    assert(queueFamilyIndices.isComplete());

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint> uniqueQueueFamilies = {
            queueFamilyIndices.compute.value(),
            queueFamilyIndices.graphics.value(),
            queueFamilyIndices.present.value(),
    };

    queueCreateInfos.reserve(uniqueQueueFamilies.size());
    float queuePriority = 1.0f;
    for(uint queueFamily : uniqueQueueFamilies)
    {
        queueCreateInfos.push_back(VkDeviceQueueCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queueFamily,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        });
    }

    VkPhysicalDeviceFeatures deviceFeatures{
        .samplerAnisotropy = VK_TRUE,
    };

    std::vector<const char*> devicesExtensions(info.deviceExtensions.begin(), info.deviceExtensions.end());

    VkDeviceCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(devicesExtensions.size()),
        .ppEnabledExtensionNames = devicesExtensions.data(),
        .pEnabledFeatures = &deviceFeatures,
    };

    vkCheck(vkCreateDevice(vkPhysicalDevice, &createInfo, nullptr, &vkDevice));

    vkGetDeviceQueue(vkDevice, queueFamilyIndices.compute.value(), 0, &computeQueue);
    vkGetDeviceQueue(vkDevice, queueFamilyIndices.graphics.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(vkDevice, queueFamilyIndices.present.value(), 0, &presentQueue);

}

void EvDevice::createAllocator() {
    VmaAllocatorCreateInfo allocatorInfo {
        .flags = VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT,
        .physicalDevice = vkPhysicalDevice,
        .device = vkDevice,
        .instance = vkInstance,
        .vulkanApiVersion = VK_API_VERSION_1_2,
    };

    vmaCreateAllocator(&allocatorInfo, &vmaAllocator);
}

void EvDevice::createCommandPool() {
    VkCommandPoolCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamilyIndices.graphics.value(),
    };

    vkCheck(vkCreateCommandPool(vkDevice, &createInfo, nullptr, &vkCommandPool));
}

void EvDevice::createDescriptorPool() {
    VkDescriptorPoolSize pool_sizes[] =
            {
                    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
            };

    VkDescriptorPoolCreateInfo poolInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = 100,
            .poolSizeCount = std::size(pool_sizes),
            .pPoolSizes = pool_sizes,
    };

    vkCheck(vkCreateDescriptorPool(vkDevice, &poolInfo, nullptr, &vkDescriptorPool));
}

bool EvDevice::isDeviceSuitable(VkPhysicalDevice physicalDevice) const {
    assert(vkSurface && "Surface should be initialized");
    auto indices = findQueueFamilies(physicalDevice, vkSurface);
    if (!indices.isComplete()) return false;

    if (!deviceExtensionsSupported(physicalDevice, info.deviceExtensions)) {
        return false;
    }

    auto swapchainDetails = querySwapchainSupport(physicalDevice, vkSurface);
    bool swapchainSuitable = !swapchainDetails.formats.empty() && !swapchainDetails.presentModes.empty();
    if (!swapchainSuitable) {
        return false;
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    return deviceFeatures.samplerAnisotropy;
}

void EvDevice::createDeviceImage(VkImageCreateInfo imageInfo, VkImage *image, VmaAllocation *memory) {
    VmaAllocationCreateInfo allocInfo {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
    };

    vkCheck(vmaCreateImage(vmaAllocator, &imageInfo, &allocInfo, image, memory, nullptr));
}

void EvDevice::createDeviceCubemap(uint32_t width, uint32_t height, float **dataLayers, VkImage* image, VmaAllocation* imageMemory, VkImageView* imageView) {
    size_t layerValues = width * height * 4;
    VkDeviceSize layerSize = layerValues * sizeof(float);
    VkDeviceSize imageSize = layerSize * 6;

    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferMemory;
    createHostBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &stagingBuffer, &stagingBufferMemory);

    void* data;
    vkCheck(vmaMapMemory(vmaAllocator, stagingBufferMemory, &data));
    for(int i=0; i<6; i++) {
        memcpy(static_cast<float*>(data) + (layerValues * i), dataLayers[i], layerSize);
    }
    vmaUnmapMemory(vmaAllocator, stagingBufferMemory);

    auto format = VK_FORMAT_R32G32B32A32_SFLOAT;
    auto imageInfo = vks::initializers::imageCreateInfo(width, height, format, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    imageInfo.arrayLayers = 6;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    createDeviceImage(imageInfo, image, imageMemory);

    transitionImageLayout(*image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, 6);

    auto copyInfo = vks::initializers::imageCopy(width, height);
    copyInfo.imageSubresource.layerCount = 6;
    copyBufferToImage(*image, stagingBuffer, copyInfo);
    transitionImageLayout(*image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 6);

    vmaDestroyBuffer(vmaAllocator, stagingBuffer, stagingBufferMemory);

    auto viewInfo = vks::initializers::imageViewCreateInfo(*image, format, VK_IMAGE_ASPECT_COLOR_BIT);
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.subresourceRange.layerCount = 6;

    vkCreateImageView(vkDevice, &viewInfo, nullptr, imageView);
}

void EvDevice::createAttachment(VkFormat format, VkImageUsageFlagBits usage, VkSampleCountFlagBits samples, uint32_t width, uint32_t height, EvFrameBufferAttachment *attachment) {
    VkImageAspectFlags aspectMask;

    attachment->format = format;

    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    else if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else {
        throw std::range_error("Given usage not implemented");
    }

    auto imageInfo = vks::initializers::imageCreateInfo(width, height, format, usage | VK_IMAGE_USAGE_SAMPLED_BIT);
    imageInfo.samples = samples;
    createDeviceImage(imageInfo, &attachment->image, &attachment->imageMemory);
    auto viewInfo = vks::initializers::imageViewCreateInfo(attachment->image, format, aspectMask);
    vkCheck(vkCreateImageView(vkDevice, &viewInfo, nullptr, &attachment->view));
}

SwapchainSupportDetails EvDevice::getSwapchainSupportDetails() const {
    return querySwapchainSupport(vkPhysicalDevice, vkSurface);
}

VkShaderModule EvDevice::createShaderModule(const char* filepath) const {
    auto code = readFile(filepath);
    printf("shader module %s constains %lu bytes\n", filepath, code.size());
    VkShaderModuleCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = code.size(),
            .pCode = reinterpret_cast<const uint32_t*>(code.data()),
    };

    VkShaderModule shaderModule;
    vkCheck(vkCreateShaderModule(vkDevice, &createInfo, nullptr, &shaderModule));
    return shaderModule;
}


void EvDevice::createHostBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer *buffer, VmaAllocation *bufferMemory) {
    VkBufferCreateInfo bufferInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationCreateInfo allocInfo {
        .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    };

    vkCheck(vmaCreateBuffer(vmaAllocator, &bufferInfo, &allocInfo, buffer, bufferMemory, nullptr));
}

void EvDevice::createDeviceBuffer(VkDeviceSize size, void* src, VkBufferUsageFlags usage, VkBuffer *buffer, VmaAllocation *bufferMemory) {
    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferMemory;
    createHostBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | usage, &stagingBuffer, &stagingBufferMemory);
    void* data;
    vkCheck(vmaMapMemory(vmaAllocator, stagingBufferMemory, &data));
    memcpy(data, src, static_cast<size_t>(size));
    vmaUnmapMemory(vmaAllocator, stagingBufferMemory);

    VkBufferCreateInfo bufferInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationCreateInfo allocInfo {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
    };

    vkCheck(vmaCreateBuffer(vmaAllocator, &bufferInfo, &allocInfo, buffer, bufferMemory, nullptr));
    copyBuffer(*buffer, stagingBuffer, size);
    vmaDestroyBuffer(vmaAllocator, stagingBuffer, stagingBufferMemory);
}

VkCommandBuffer EvDevice::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vkCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer commandBuffer;
    vkCheck(vkAllocateCommandBuffers(vkDevice, &allocInfo, &commandBuffer));

    VkCommandBufferBeginInfo beginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkCheck(vkBeginCommandBuffer(commandBuffer, &beginInfo));
    return commandBuffer;
}

void EvDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkCheck(vkEndCommandBuffer(commandBuffer));
    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };

    vkCheck(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(vkDevice, vkCommandPool, 1, &commandBuffer);
}

void EvDevice::copyBuffer(VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    VkBufferCopy copyRegion {
        .size = size,
    };
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    endSingleTimeCommands(commandBuffer);
}

VkFormat EvDevice::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
{
    for (const auto& format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(vkPhysicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            return format;

        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            return format;
    }

    throw std::runtime_error("Failed to find supported format");
}

VkFormat EvDevice::findDepthFormat() const {
    return findSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

void EvDevice::copyBufferToImage(VkImage dst, VkBuffer src, VkBufferImageCopy copyInfo) {
    auto cmdBuffer = beginSingleTimeCommands();
    vkCmdCopyBufferToImage(cmdBuffer, src, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyInfo);
    endSingleTimeCommands(cmdBuffer);
}

void EvDevice::transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels,
                                uint32_t arrayLayers) {
    struct TransitionInfo_T {
        VkAccessFlags srcAccessMask;
        VkAccessFlags dstAccessMask;
        VkPipelineStageFlags srcStage;
        VkPipelineStageFlags dstStage;
    };

    std::unordered_map<std::pair<VkImageLayout, VkImageLayout>, TransitionInfo_T, pair_hash> lookup {
        {
            {VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL},
            TransitionInfo_T {
                .srcAccessMask = 0,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                .dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
            }
        },
        {
            {VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL},
            TransitionInfo_T {
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                    .srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    .dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            }
        },
        {
            {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
            TransitionInfo_T {
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                .srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
                .dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            }
        }
    };

    if (lookup.find({oldLayout, newLayout}) == lookup.end()) {
        throw std::runtime_error("Transition not implemented");
    }

    auto transitionInfo = lookup[{oldLayout, newLayout}];

    VkImageMemoryBarrier barrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = transitionInfo.srcAccessMask,
        .dstAccessMask = transitionInfo.dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = mipLevels,
            .baseArrayLayer = 0,
            .layerCount = arrayLayers,
        },
    };

    auto cmdBuffer = beginSingleTimeCommands();
    vkCmdPipelineBarrier(
            cmdBuffer,
            transitionInfo.srcStage, transitionInfo.dstStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

    endSingleTimeCommands(cmdBuffer);
}

void EvFrameBufferAttachment::destroy(EvDevice &device) {
    vmaDestroyImage(device.vmaAllocator, image, imageMemory);
    vkDestroyImageView(device.vkDevice, view, nullptr);
}

void EvFrameBuffer::destroy(EvDevice &device) {
    for(const auto& frameBuffer : vkFrameBuffers)
        vkDestroyFramebuffer(device.vkDevice, frameBuffer, nullptr);
    vkDestroyRenderPass(device.vkDevice, vkRenderPass, nullptr);
}
