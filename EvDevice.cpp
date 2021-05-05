#include "EvDevice.h"

#include <stdexcept>
#include <cassert>
#include "UtilsPhysicalDevice.h"

EvDevice::EvDevice(EvDeviceInfo info, EvWindow &window) : window(window), info(info) {
    finalizeInfo();
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
}

EvDevice::~EvDevice() {
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
        .pApplicationName = window.name,
        .applicationVersion = VK_MAKE_VERSION(1,0,0),
        .pEngineName = "EngineName",
        .engineVersion = VK_MAKE_VERSION(1,0,0),
        .apiVersion = VK_API_VERSION_1_2,
    };

    VkInstanceCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
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
    swapchainSupportDetails = querySwapchainSupport(vkPhysicalDevice, vkSurface);
    vkGetPhysicalDeviceProperties(vkPhysicalDevice, &vkPhysicalDeviceProperties);
    vkGetPhysicalDeviceFeatures(vkPhysicalDevice, &vkPhysicalDeviceFeatures);

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


