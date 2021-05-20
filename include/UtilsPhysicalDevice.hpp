#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

struct QueueFamilyIndices {
    std::optional<uint32_t> compute;
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;

    bool isComplete() const {
        return compute.has_value() && graphics.has_value() && present.has_value();
    }
};

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

namespace {
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
        QueueFamilyIndices indices{};

        uint queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        VkQueueFamilyProperties properties[queueFamilyCount];
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, properties);

        for(uint i=0; i<queueFamilyCount; i++) {
            const auto& queueFamily = properties[i];
            if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                indices.compute = i;
            }
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphics = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
            if (presentSupport) {
                indices.present = i;
            }
        }

        return indices;
    }

    SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
        SwapchainSupportDetails details{};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

        uint formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

        if (formatCount > 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
        }

        uint presentCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentCount, nullptr);

        if (presentCount > 0) {
            details.presentModes.resize(presentCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentCount, details.presentModes.data());
        }

        return details;
    }

    bool deviceExtensionsSupported(VkPhysicalDevice physicalDevice, const std::set<const char*>& extensions) {
        uint extensionCount;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
        VkExtensionProperties availableExtensions[extensionCount];
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions);

        std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());

        for(const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }
}
