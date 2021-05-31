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
    VkDescriptorPoolSize poolSize {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 20,
    };

    VkDescriptorPoolCreateInfo poolInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = 20,
            .poolSizeCount = 1,
            .pPoolSizes = &poolSize,
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

void EvDevice::createImage(uint width, uint height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                           VkSampleCountFlagBits numSamples, VkImage *image, VmaAllocation *memory) {

    VkImageCreateInfo imageInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {
            .width = width,
            .height = height,
            .depth = 1,
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = numSamples,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo allocInfo {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
    };

    vkCheck(vmaCreateImage(vmaAllocator, &imageInfo, &allocInfo, image, memory, nullptr));
}

void EvDevice::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView* imageView) {

    VkImageViewCreateInfo viewInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange = {
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    vkCheck(vkCreateImageView(vkDevice, &viewInfo, nullptr, imageView));
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
        .usage = VMA_MEMORY_USAGE_CPU_ONLY,
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

void EvDevice::copyBufferToImage(VkImage dst, VkBuffer src, uint32_t width, uint32_t height) {
    auto cmdBuffer = beginSingleTimeCommands();
    VkBufferImageCopy region {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageOffset = {0,0,0},
        .imageExtent = {width, height, 1},
    };

    vkCmdCopyBufferToImage(cmdBuffer, src, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    endSingleTimeCommands(cmdBuffer);
}

void EvDevice::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
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
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
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

