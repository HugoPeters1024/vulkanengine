#include "RenderSystem.h"

RenderSystem::RenderSystem(EvDevice &device) : device(device) {
    createShaderModules();
    createSwapchain();
    createDescriptorSetLayout();
    createPipelineLayout();
    createPipeline();
    createGBuffer();
    allocateCommandBuffers();
}

RenderSystem::~RenderSystem() {
    printf("Destroying the gbuffer framebuffer");
    gbuffer.albedo.destroy(device);
    gbuffer.destroy(device);

    printf("Destroying descriptor set layout\n");
    vkDestroyDescriptorSetLayout(device.vkDevice, vkDescriptorSetLayout, nullptr);

    printf("Destroying the shader modules\n");
    vkDestroyShaderModule(device.vkDevice, vertShaderModule, nullptr);
    vkDestroyShaderModule(device.vkDevice, fragShaderModule, nullptr);

    printf("Destroying pipeline pipelineLayout\n");
    vkDestroyPipelineLayout(device.vkDevice, vkPipelineLayout, nullptr);
}


void RenderSystem::createShaderModules() {
    vertShaderModule = device.createShaderModule("assets/shaders_bin/shader.vert.spv");
    fragShaderModule = device.createShaderModule("assets/shaders_bin/shader.frag.spv");
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
            .setLayoutCount = 1,
            .pSetLayouts = &vkDescriptorSetLayout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushConstantRange,
    };

    vkCheck(vkCreatePipelineLayout(device.vkDevice, &createInfo, nullptr, &vkPipelineLayout));
}

void RenderSystem::createPipeline() {
    EvRastPipeline::defaultRastPipelineInfo(vertShaderModule, fragShaderModule, device.msaaSamples, &pipelineInfo);
    pipelineInfo.renderPass = swapchain->vkRenderPass;
    pipelineInfo.pipelineLayout = vkPipelineLayout;
    pipelineInfo.descriptorSetLayout = vkDescriptorSetLayout;
    pipelineInfo.swapchainImages = static_cast<uint32>(swapchain->vkImages.size());

    rastPipeline = std::make_unique<EvRastPipeline>(device, pipelineInfo);
}

void RenderSystem::createGBuffer() {
    gbuffer.width = swapchain->extent.width;
    gbuffer.height = swapchain->extent.height;
    device.createAttachment(VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, swapchain->extent.width, swapchain->extent.height, &gbuffer.albedo);

    std::array<VkAttachmentDescription, 1> attachmentDescriptions{};

    for (size_t i = 0; i < attachmentDescriptions.size(); i++) {
        attachmentDescriptions[i].samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentDescriptions[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDescriptions[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescriptions[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescriptions[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        // TODO: not correct for the depth buffer
        attachmentDescriptions[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    attachmentDescriptions[0].format = gbuffer.albedo.format;

    std::vector<VkAttachmentReference> colorReferences {
        VkAttachmentReference {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        }
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = static_cast<uint32_t>(colorReferences.size()),
        .pColorAttachments = colorReferences.data(),
        // TODO
        .pDepthStencilAttachment = nullptr,
    };

    std::array<VkSubpassDependency, 2> dependencies {
        VkSubpassDependency {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        },
        VkSubpassDependency {
            .srcSubpass = 0,
            .dstSubpass = VK_SUBPASS_EXTERNAL,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        }
    };

    VkRenderPassCreateInfo renderpassInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size()),
        .pAttachments = attachmentDescriptions.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = static_cast<uint32_t>(dependencies.size()),
        .pDependencies = dependencies.data(),
    };

    vkCheck(vkCreateRenderPass(device.vkDevice, &renderpassInfo, nullptr, &gbuffer.renderPass));

    std::array<VkImageView, 1> attachments{};
    attachments[0] = gbuffer.albedo.view;

    VkFramebufferCreateInfo framebufferInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = gbuffer.renderPass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = gbuffer.width,
        .height= gbuffer.height,
        .layers = 1,
    };

    vkCheck(vkCreateFramebuffer(device.vkDevice, &framebufferInfo, nullptr, &gbuffer.frameBuffer));
}

void RenderSystem::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding textureBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };

    std::array<VkDescriptorSetLayoutBinding, 1> bindings = {textureBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    vkCheck(vkCreateDescriptorSetLayout(device.vkDevice, &layoutInfo, nullptr, &vkDescriptorSetLayout));
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

void RenderSystem::recordCommandBuffer(uint32_t imageIndex, const EvCamera &camera, EvOverlay *overlay) const {
    const VkCommandBuffer& commandBuffer = commandBuffers[imageIndex];
    vkCheck(vkResetCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));

    VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkCheck(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    std::array<VkClearValue, 3> clearValues {};
    clearValues[0] = {
            .color = {0.0f, 0.1f, 0.1f, 1.0f},
    };
    clearValues[1] = {
            .color = {0.0f, 0.1f, 0.1f, 1.0f},
    };
    clearValues[2] = {
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

    PushConstant push{
        .camera = camera.getVPMatrix(device.window.getAspectRatio()),
    };

    for (const auto& entity : m_entities) {
        auto& modelComp = m_coordinator->GetComponent<ModelComponent>(entity);
        auto scaleMatrix = glm::scale(glm::mat4(1.0f), modelComp.scale);
        push.mvp = modelComp.transform * scaleMatrix;
        vkCmdPushConstants(
                commandBuffer,
                vkPipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(push),
                &push);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipelineLayout, 0, 1, &modelComp.model->vkDescriptorSets[imageIndex], 0, nullptr);
        modelComp.model->bind(commandBuffer);
        modelComp.model->draw(commandBuffer);
    }

    if (overlay)
        overlay->Draw(commandBuffer);
    vkCmdEndRenderPass(commandBuffer);
    vkCheck(vkEndCommandBuffer(commandBuffer));
}

void RenderSystem::recreateSwapchain() {
    vkCheck(vkDeviceWaitIdle(device.vkDevice));
    device.window.waitForEvent();
    createSwapchain();
}

void RenderSystem::Render(const EvCamera &camera, EvOverlay* overlay) {
    overlay->NewFrame();
    uint32_t imageIndex;
    VkResult acquireImageResult = swapchain->acquireNextSwapchainImage(&imageIndex);

    if (acquireImageResult == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    } else if (acquireImageResult != VK_SUCCESS && acquireImageResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    swapchain->waitForImageReady(imageIndex);


    recordCommandBuffer(imageIndex, camera, overlay);

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
    return signature;
}

std::unique_ptr<EvModel> RenderSystem::createModel(const std::string &filename, const EvTexture *texture) {
    auto ret = std::make_unique<EvModel>(device, filename, texture);
    ret->vkDescriptorSets.resize(swapchain->vkImages.size());

    std::vector<VkDescriptorSetLayout> layouts(ret->vkDescriptorSets.size(), vkDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = device.vkDescriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(ret->vkDescriptorSets.size()),
        .pSetLayouts = layouts.data(),
    };

    vkCheck(vkAllocateDescriptorSets(device.vkDevice, &allocInfo, ret->vkDescriptorSets.data()));

    auto imageDescriptor = texture->getDescriptorInfo();

    for(int i=0; i<swapchain->vkImages.size(); i++) {
        std::array<VkWriteDescriptorSet,1> descriptorWrites {
            VkWriteDescriptorSet {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = ret->vkDescriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &imageDescriptor,
            },
        };

        vkUpdateDescriptorSets(
            device.vkDevice,
            static_cast<uint32_t>(descriptorWrites.size()),
            descriptorWrites.data(),
            0, nullptr);
    }

    return ret;
}



