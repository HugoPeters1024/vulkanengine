#include "EvDevice.h"

#include <stdexcept>
#include <cassert>
#include <utility>
#include "UtilsPhysicalDevice.h"

EvDevice::EvDevice(EvDeviceInfo info, EvWindow &window) : window(window), info(std::move(info)) {
    finalizeInfo();
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createCommandPool();
}

EvDevice::~EvDevice() {
    printf("Destroying commandPool\n");
    vkDestroyCommandPool(vkDevice, vkCommandPool, nullptr);
    printf("Destroying logical device\n");
    vkDestroyDevice(vkDevice, nullptr);
    printf("Destroying surface\n");
    vkDestroySurfaceKHR(vkInstance, vkSurface, nullptr);
    printf("Destroying instance\n");
    vkDestroyInstance(vkInstance, nullptr);
}

void EvDevice::finalizeInfo() {
    info.validationLayers.insert("VK_LAYER_KHRONOS_validation");
    info.deviceExtensions.insert(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
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

void EvDevice::createCommandPool() {
    VkCommandPoolCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamilyIndices.graphics.value(),
    };

    vkCheck(vkCreateCommandPool(vkDevice, &createInfo, nullptr, &vkCommandPool));
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
                              VkMemoryPropertyFlags properties, VkImage* image, VkDeviceMemory* memory) {

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
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    vkCheck(vkCreateImage(vkDevice, &imageInfo, nullptr, image));

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(vkDevice, *image, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties),
    };

    vkCheck(vkAllocateMemory(vkDevice, &allocInfo, nullptr, memory));
    vkCheck(vkBindImageMemory(vkDevice, *image, *memory, 0));
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

uint32_t EvDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
    for(uint32_t i=0; i<vkPhysicalDeviceMemoryProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i))  && (vkPhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Could not find suitable memory type");
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


void EvDevice::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *bufferMemory) {
    VkBufferCreateInfo bufferInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    vkCheck(vkCreateBuffer(vkDevice, &bufferInfo, nullptr, buffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vkDevice, *buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties),
    };

    vkCheck(vkAllocateMemory(vkDevice, &allocInfo, nullptr, bufferMemory));
    vkCheck(vkBindBufferMemory(vkDevice, *buffer, *bufferMemory, 0));
}


