#include "EvSwapchain.h"
#include "UtilsMemory.h"
#include <array>
#include <cassert>

EvSwapchain::EvSwapchain(EvDevice &device) : device(device) {
    createSwapchain();
    createImageViews();
    createDepthResources();
    createRenderpass();
    createFramebuffers();
    createSyncObjects();
}

EvSwapchain::~EvSwapchain() {
    printf("Destroying fences\n");

    for(const auto& fence : inFlightFences)
        vkDestroyFence(device.vkDevice, fence, nullptr);

    printf("Destroying semaphores\n");

    for(const auto& sem : imageAvailableSemaphores)
        vkDestroySemaphore(device.vkDevice, sem, nullptr);

    for(const auto& sem : renderFinishedSemaphores)
        vkDestroySemaphore(device.vkDevice, sem, nullptr);


    printf("Destroying frame buffers\n");
    for(const auto& framebuffer : vkFramebuffers){
        vkDestroyFramebuffer(device.vkDevice, framebuffer, nullptr);
    }

    printf("Destroying swapchain's renderpass\n");
    vkDestroyRenderPass(device.vkDevice, vkRenderPass, nullptr);
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

void EvSwapchain::createImageViews() {
    vkImageViews.resize((vkImages.size()));
    for(uint i=0; i<vkImageViews.size(); i++) {
        vkImageViews[i] = device.createImageView(vkImages[i], surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void EvSwapchain::createDepthResources() {
    VkFormat depthFormat = findDepthFormat(device.vkPhysicalDevice);
    vkDepthImage = device.createImage(extent.width, extent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkDepthImageView = device.createImageView(vkDepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void EvSwapchain::createFramebuffers() {
    vkFramebuffers.resize(vkImageViews.size());

    for(uint i=0; i < vkFramebuffers.size(); i++) {
        std::array<VkImageView, 2> attachments {
            vkImageViews[i],
            vkDepthImageView,
        };

        VkFramebufferCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = vkRenderPass,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .width = extent.width,
            .height = extent.height,
            .layers = 1,
        };

        vkCheck(vkCreateFramebuffer(device.vkDevice, &createInfo, nullptr, &vkFramebuffers[i]));
    }
}

void EvSwapchain::createRenderpass() {
    VkAttachmentDescription colorAttachment {
        .format = surfaceFormat.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference colorAttachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentDescription depthAttachment {
        .format = findDepthFormat(device.vkPhysicalDevice),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depthAttachmentRef {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

   VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef
    };

    VkSubpassDependency dependency {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT + VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    };

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 1,
            .pDependencies = &dependency,
    };

    vkCheck(vkCreateRenderPass(device.vkDevice, &createInfo, nullptr, &vkRenderPass));
}

void EvSwapchain::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);

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

void EvSwapchain::presentCommandBuffer(const std::vector<VkCommandBuffer> &commandBuffers) {
    assert(commandBuffers.size() == vkImages.size());
    vkWaitForFences(device.vkDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint imageIndex;
    VkResult result = vkAcquireNextImageKHR(device.vkDevice, vkSwapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        throw std::runtime_error("Window was resized!");
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device.vkDevice, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }

    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    std::array<VkSemaphore,1> waitSemaphores = { imageAvailableSemaphores[currentFrame] };
    std::array<VkPipelineStageFlags,1> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    std::array<VkCommandBuffer,1> pCommandBuffers = {commandBuffers[imageIndex]};
    std::array<VkSemaphore,1> signalSemaphores = { renderFinishedSemaphores[currentFrame]};

    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
        .pWaitSemaphores = waitSemaphores.data(),
        .pWaitDstStageMask = waitStages.data(),
        .commandBufferCount = static_cast<uint32_t>(pCommandBuffers.size()),
        .pCommandBuffers = pCommandBuffers.data(),
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

    result = vkQueuePresentKHR(device.presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || resized) {
        resized = false;
        // recreate swapchain
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image");
    }

    currentFrame = (currentFrame + 1) & MAX_FRAMES_IN_FLIGHT;
}






