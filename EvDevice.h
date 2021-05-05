#pragma once

#include <vulkan/vulkan.h>
#include <set>

#include "utils.h"
#include "UtilsPhysicalDevice.h"
#include "EvWindow.h"

struct EvDeviceInfo {
    std::set<const char*> validationLayers;
    std::set<const char*> instanceExtensions;
    std::set<const char*> deviceExtensions;
};

class EvDevice : NoCopy {
private:
    bool isDeviceSuitable(VkPhysicalDevice physicalDevice) const;

public:
    EvDeviceInfo info;
    EvWindow& window;
    VkInstance vkInstance;
    VkPhysicalDevice vkPhysicalDevice;
    VkPhysicalDeviceProperties vkPhysicalDeviceProperties;
    VkPhysicalDeviceFeatures vkPhysicalDeviceFeatures;
    QueueFamilyIndices queueFamilyIndices;
    SwapchainSupportDetails swapchainSupportDetails;
    VkDevice vkDevice;
    VkSurfaceKHR vkSurface;

    VkQueue computeQueue;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    EvDevice(EvDeviceInfo info, EvWindow &window);
    ~EvDevice();

    void finalizeInfo();
    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
};
