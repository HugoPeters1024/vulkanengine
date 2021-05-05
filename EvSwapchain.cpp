#include "EvSwapchain.h"

EvSwapchain::EvSwapchain(const EvDevice &device) : device(device) {
    createSwapchain();
}

EvSwapchain::~EvSwapchain() {
    printf("Destroying swapchain\n");
    vkDestroySwapchainKHR(device.vkDevice, vkSwapchain, nullptr);
}

void EvSwapchain::createSwapchain() {
    const auto& capabilities = device.swapchainSupportDetails.capabilities;
    surfaceFormat = chooseSwapSurfaceFormat();
    VkPresentModeKHR presentMode = chooseSwapPresentMode();
    extent = chooseSwapExtent();

    uint imageCount = capabilities.minImageCount + 1;

    // Check that the specs allow the extra wiggle room
    if (capabilities.maxImageCount > 0) {
        imageCount = std::min(imageCount, capabilities.maxImageCount);
    }

    VkSwapchainCreateInfoKHR createInfo {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = device.vkSurface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };

    const auto& indices = device.queueFamilyIndices;
    uint queueFamilyIndices[2] = { indices.graphics.value(), indices.present.value() };

    if (indices.graphics != indices.present) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    vkCheck(vkCreateSwapchainKHR(device.vkDevice, &createInfo, nullptr, &vkSwapchain));
    vkGetSwapchainImagesKHR(device.vkDevice, vkSwapchain, &imageCount, nullptr);
    vkImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device.vkDevice, vkSwapchain, &imageCount, vkImages.data());
}

VkSurfaceFormatKHR EvSwapchain::chooseSwapSurfaceFormat() const {
    // Prefer SRGB
    const auto& availableFormats = device.swapchainSupportDetails.formats;
    for(const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR EvSwapchain::chooseSwapPresentMode() const {
    const auto& availablePresentModes = device.swapchainSupportDetails.presentModes;
    for(const auto& presentMode : availablePresentModes) {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return presentMode;
        }
    }

    // Always available
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D EvSwapchain::chooseSwapExtent() const {
    const auto& capabilities = device.swapchainSupportDetails.capabilities;

    if (capabilities.currentExtent.width != 0xffffffff) {
        return capabilities.currentExtent;
    }

    int width, height;
    device.window.getFramebufferSize(&width, &height);

    return VkExtent2D {
        .width = std::clamp(static_cast<uint>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        .height = std::clamp(static_cast<uint>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
    };
}

