#include "App.h"
#include "utils.h"

App::App() {
    createSwapchain();
    createPipelineLayout();
    createPipeline();
    allocateCommandBuffers();
    recordCommandBuffers();
}

App::~App() {
    printf("Destroying pipeline layout\n");
    vkDestroyPipelineLayout(device.vkDevice, vkPipelineLayout, nullptr);
}

void App::Run() {

    while (!window.shouldClose()) {
        window.processEvents();
        drawFrame();
    }

    printf("Flushing GPU before shutdown...\n");
    vkDeviceWaitIdle(device.vkDevice);
    printf("Goodbye!\n");
}

void App::createSwapchain() {
    swapchain.reset();
    swapchain = std::make_unique<EvSwapchain>(device);
}

void App::createPipelineLayout() {
    VkPipelineLayoutCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 0,
            .pSetLayouts = nullptr,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
    };

    vkCheck(vkCreatePipelineLayout(device.vkDevice, &createInfo, nullptr, &vkPipelineLayout));
}

void App::createPipeline() {
    pipelineInfo = defaultRastPipelineInfo(swapchain->extent.width, swapchain->extent.height);
    pipelineInfo.renderPass = swapchain->vkRenderPass;
    pipelineInfo.layout = vkPipelineLayout;

    rastPipeline = std::make_unique<EvRastPipeline>(
            device, pipelineInfo, "shaders_bin/shader.vert.spv", "shaders_bin/shader.frag.spv");
}

void App::allocateCommandBuffers() {
    commandBuffers.resize(swapchain->vkImages.size());

    VkCommandBufferAllocateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = device.vkCommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<uint32_t>(commandBuffers.size()),
    };

    vkCheck(vkAllocateCommandBuffers(device.vkDevice, &createInfo, commandBuffers.data()));
}

void App::recordCommandBuffers() {
    for(uint i=0; i<commandBuffers.size(); i++) {
        vkCheck(vkResetCommandBuffer(commandBuffers[i], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));

        VkCommandBufferBeginInfo beginInfo{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        };

        vkCheck(vkBeginCommandBuffer(commandBuffers[i], &beginInfo));

        std::array<VkClearValue, 2> clearValues {};
        clearValues[0] = {
                .color = {0.0f, 0.1f, 0.1f, 1.0f},
        };
        clearValues[1] = {
                .depthStencil = {1.0f, 0},
        };

        VkRenderPassBeginInfo renderPassInfo {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = swapchain->vkRenderPass,
                .framebuffer = swapchain->vkFramebuffers[i],
                .renderArea = {
                        .offset = {0,0},
                        .extent = swapchain->extent,
                },
                .clearValueCount = static_cast<uint32_t>(clearValues.size()),
                .pClearValues = clearValues.data(),
        };

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        rastPipeline->bind(commandBuffers[i]);
        vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
        vkCmdEndRenderPass(commandBuffers[i]);
        vkCheck(vkEndCommandBuffer(commandBuffers[i]));
    }
}

void App::drawFrame() {
    uint32_t imageIndex;
    VkResult acquireImageResult = swapchain->acquireNextSwapchainImage(&imageIndex);

    if (acquireImageResult == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    } else if (acquireImageResult != VK_SUCCESS && acquireImageResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    VkResult presentResult = swapchain->presentCommandBuffer(commandBuffers[imageIndex], imageIndex);

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || window.wasResized) {
        window.wasResized = false;
        recreateSwapchain();
        return;
    } else if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image");
    }
}

void App::recreateSwapchain() {
    window.waitForEvent();
    vkCheck(vkDeviceWaitIdle(device.vkDevice));
    createSwapchain();
    createPipeline();
    recordCommandBuffers();
}




