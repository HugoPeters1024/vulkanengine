#include "RenderSystem.h"
#include "ShaderTypes.h"
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>

RenderSystem::RenderSystem(EvDevice &device) : device(device) {
    createShaderModules();
    createSwapchain();
    createPipelineLayout();
    createPipeline();
    allocateCommandBuffers();
}

RenderSystem::~RenderSystem() {
    printf("Destroying the shader modules\n");
    vkDestroyShaderModule(device.vkDevice, vertShaderModule, nullptr);
    vkDestroyShaderModule(device.vkDevice, fragShaderModule, nullptr);

    printf("Destroying pipeline layout\n");
    vkDestroyPipelineLayout(device.vkDevice, vkPipelineLayout, nullptr);
}


void RenderSystem::createShaderModules() {
    vertShaderModule = device.createShaderModule("glsl_shaders_bin/shader.vert.spv");
    fragShaderModule = device.createShaderModule("glsl_shaders_bin/shader.frag.spv");
}

void RenderSystem::createSwapchain() {
    if (swapchain == nullptr) {
        swapchain = std::make_unique<EvSwapchain>(device);
    } else {
        swapchain = std::make_unique<EvSwapchain>(device, std::move(swapchain));
    }
}

void RenderSystem::createPipelineLayout() {
    VkPushConstantRange pushConstantRange{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(PushConstant),
    };

    VkPipelineLayoutCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 0,
            .pSetLayouts = nullptr,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushConstantRange,
    };

    vkCheck(vkCreatePipelineLayout(device.vkDevice, &createInfo, nullptr, &vkPipelineLayout));
}

void RenderSystem::createPipeline() {
    EvRastPipeline::defaultRastPipelineInfo(vertShaderModule, fragShaderModule, &pipelineInfo);
    pipelineInfo.renderPass = swapchain->vkRenderPass;
    pipelineInfo.layout = vkPipelineLayout;

    rastPipeline = std::make_unique<EvRastPipeline>(device, pipelineInfo);
}

void RenderSystem::allocateCommandBuffers() {
    commandBuffers.resize(swapchain->vkImages.size());

    VkCommandBufferAllocateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = device.vkCommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<uint32_t>(commandBuffers.size()),
    };

    vkCheck(vkAllocateCommandBuffers(device.vkDevice, &createInfo, commandBuffers.data()));
}

void RenderSystem::recordCommandBuffer(uint32_t imageIndex) const {
    const VkCommandBuffer& commandBuffer = commandBuffers[imageIndex];
    vkCheck(vkResetCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));

    VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkCheck(vkBeginCommandBuffer(commandBuffer, &beginInfo));

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
            .framebuffer = swapchain->vkFramebuffers[imageIndex],
            .renderArea = {
                    .offset = {0,0},
                    .extent = swapchain->extent,
            },
            .clearValueCount = static_cast<uint32_t>(clearValues.size()),
            .pClearValues = clearValues.data(),
    };
    VkViewport viewport {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(swapchain->extent.width),
            .height = static_cast<float>(swapchain->extent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };
    VkRect2D scissor{{0,0}, swapchain->extent};

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    rastPipeline->bind(commandBuffer);

    auto viewMatrix = glm::lookAt(glm::vec3(0,0,0), glm::vec3(0,0,1), glm::vec3(0,1,0));
    auto projectionMatrix = glm::perspective(glm::radians(90.0f), 640.0f / 480.0f, 0.1f, 100.0f);

    PushConstant push{
        .camera = projectionMatrix * viewMatrix,
    };

    for (const auto& entity : m_entities) {
        auto& modelComp = m_coordinator->GetComponent<ModelComponent>(entity);
        auto& transformComp = m_coordinator->GetComponent<TransformComponent>(entity);

        auto translation = glm::translate(glm::mat4(1.0f), transformComp.position);
        auto rotation = glm::rotate(glm::mat4(1.0f), transformComp.yrot, glm::vec3(0,1,0));

        push.mvp = translation * rotation;
        vkCmdPushConstants(
                commandBuffer,
                vkPipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(push),
                &push);
        modelComp.model->bind(commandBuffer);
        modelComp.model->draw(commandBuffer);
    }

    vkCmdEndRenderPass(commandBuffers[imageIndex]);
    vkCheck(vkEndCommandBuffer(commandBuffers[imageIndex]));
}

void RenderSystem::recreateSwapchain() {
    vkCheck(vkDeviceWaitIdle(device.vkDevice));
    device.window.waitForEvent();
    createSwapchain();
}

void RenderSystem::Render() {
    uint32_t imageIndex;
    VkResult acquireImageResult = swapchain->acquireNextSwapchainImage(&imageIndex);

    if (acquireImageResult == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    } else if (acquireImageResult != VK_SUCCESS && acquireImageResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    swapchain->waitForImageReady(imageIndex);
    recordCommandBuffer(imageIndex);
    VkResult presentResult = swapchain->presentCommandBuffer(commandBuffers[imageIndex], imageIndex);

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || device.window.wasResized) {
        device.window.wasResized = false;
        recreateSwapchain();
        return;
    } else if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image");
    }
}

Signature RenderSystem::GetSignature() const {
    Signature signature{};
    signature.set(m_coordinator->GetComponentType<ModelComponent>());
    signature.set(m_coordinator->GetComponentType<TransformComponent>());
    return signature;
}


