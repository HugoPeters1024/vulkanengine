#include "EvSwapchain.h"
#include <array>
#include <utility>

EvSwapchain::EvSwapchain(EvDevice &device) : device(device) {
    init();
}

EvSwapchain::EvSwapchain(EvDevice &device, std::shared_ptr<EvSwapchain> previous)
    : device(device), oldSwapchain(std::move(previous)) {
    init();
    oldSwapchain.reset();
}

void EvSwapchain::init() {
    createSwapchain();
    createImageViews();
    createSyncObjects();
}

EvSwapchain::~EvSwapchain() {
    printf("Destroying image views\n");
    for(const auto& view : vkImageViews)
        vkDestroyImageView(device.vkDevice, view, nullptr);
    printf("Destroying semaphores\n");
    for(const auto& sem : imageAvailableSemaphores)
        vkDestroySemaphore(device.vkDevice, sem, nullptr);
    for(const auto& sem : renderFinishedSemaphores)
        vkDestroySemaphore(device.vkDevice, sem, nullptr);

    printf("Destroying fences\n");
    for(const auto& fence : inFlightFences)
        vkDestroyFence(device.vkDevice, fence, nullptr);

    printf("Destroying swapchain\n");
    vkDestroySwapchainKHR(device.vkDevice, vkSwapchain, nullptr);
}


void EvSwapchain::createSwapchain() {
    const auto& capabilities = device.getSwapchainSupportDetails().capabilities;
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
        .oldSwapchain = oldSwapchain == nullptr ? VK_NULL_HANDLE : oldSwapchain->vkSwapchain,
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

void EvSwapchain::createImageViews() {
    vkImageViews.resize((vkImages.size()));
    for(uint i=0; i<vkImageViews.size(); i++) {
        auto viewInfo = vks::initializers::imageViewCreateInfo(vkImages[i], surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
        vkCheck(vkCreateImageView(device.vkDevice, &viewInfo, nullptr, &vkImageViews[i]));
    }
}

void EvSwapchain::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(vkImages.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semInfo {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkFenceCreateInfo fenceInfo {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++) {
        vkCheck(vkCreateSemaphore(device.vkDevice, &semInfo, nullptr, &imageAvailableSemaphores[i]));
        vkCheck(vkCreateSemaphore(device.vkDevice, &semInfo, nullptr, &renderFinishedSemaphores[i]));
        vkCheck(vkCreateFence(device.vkDevice, &fenceInfo, nullptr, &inFlightFences[i]));
    }
}

VkSurfaceFormatKHR EvSwapchain::chooseSwapSurfaceFormat() const {
    // Prefer SRGB
    const auto& availableFormats = device.getSwapchainSupportDetails().formats;
    for(const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR EvSwapchain::chooseSwapPresentMode() const {
    const auto& availablePresentModes = device.getSwapchainSupportDetails().presentModes;
    for(const auto& presentMode : availablePresentModes) {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return presentMode;
        }
    }

    // Always available
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D EvSwapchain::chooseSwapExtent() const {
    const auto& capabilities = device.getSwapchainSupportDetails().capabilities;

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

VkResult EvSwapchain::acquireNextSwapchainImage(uint32_t *imageIndex) {
    vkWaitForFences(device.vkDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    return vkAcquireNextImageKHR(device.vkDevice, vkSwapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, imageIndex);
}

VkResult EvSwapchain::presentCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device.vkDevice, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    std::array<VkSemaphore,1> waitSemaphores = { imageAvailableSemaphores[currentFrame] };
    std::array<VkPipelineStageFlags,1> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    std::array<VkSemaphore,1> signalSemaphores = { renderFinishedSemaphores[currentFrame]};

    VkSubmitInfo submitInfo {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
            .pWaitSemaphores = waitSemaphores.data(),
            .pWaitDstStageMask = waitStages.data(),
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer,
            .signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size()),
            .pSignalSemaphores = signalSemaphores.data(),
    };

    vkResetFences(device.vkDevice, 1, &inFlightFences[currentFrame]);
    vkCheck(vkQueueSubmit(device.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]));

    VkPresentInfoKHR presentInfo {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size()),
            .pWaitSemaphores = signalSemaphores.data(),
            .swapchainCount = 1,
            .pSwapchains = &vkSwapchain,
            .pImageIndices = &imageIndex,
            .pResults = nullptr,
    };

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    return vkQueuePresentKHR(device.presentQueue, &presentInfo);
}










