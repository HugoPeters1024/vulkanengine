#include "RenderPasses/ForwardPass.h"

ForwardPass::ForwardPass(EvDevice &device, uint32_t width, uint32_t height, uint32_t nrImages,
                         const std::vector<EvFrameBufferAttachment> &depthAttachments) : device(device) {
    createFramebuffer(width, height, nrImages, depthAttachments);

    createTexturesDescriptorSetLayout();
    createLightBufferDescriptorSetLayout();
    createPipelineLayout();
    createPipeline();
    createLightBuffers(nrImages);
    allocateLightDescriptorSets(nrImages);
    createLightDescriptorSets(nrImages);
}

ForwardPass::~ForwardPass() {
    framebuffer.destroy(device);
    vkDestroyShaderModule(device.vkDevice, vertShader, nullptr);
    vkDestroyShaderModule(device.vkDevice, fragShader, nullptr);
    vkDestroyDescriptorSetLayout(device.vkDevice, texturesDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(device.vkDevice, lightBufferDescriptorSetLayout, nullptr);
    vkDestroyPipeline(device.vkDevice, pipeline, nullptr);
    vkDestroyPipelineLayout(device.vkDevice, pipelineLayout, nullptr);
    for(int i=0; i<lightBuffers.size(); i++) {
        vmaUnmapMemory(device.vmaAllocator, lightBufferMemories[i]);
        vmaDestroyBuffer(device.vmaAllocator, lightBuffers[i], lightBufferMemories[i]);
    }
}

void ForwardPass::createFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages,
                                    const std::vector<EvFrameBufferAttachment>& depthAttachments) {
    assert(nrImages > 0);
    assert(depthAttachments.size() == nrImages);

    framebuffer.width = width;
    framebuffer.height = height;

    framebuffer.colors.resize(nrImages);

    auto format = VK_FORMAT_R16G16B16A16_SFLOAT;
    for(int i=0; i<nrImages; i++) {
        device.createAttachment(format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, width, height, &framebuffer.colors[i]);
    }

    auto colorDescription = vks::initializers::attachmentDescription(format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    auto colorReference = vks::initializers::attachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    auto depthDescription = vks::initializers::attachmentDescription(depthAttachments[0].format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    depthDescription.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    depthDescription.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    auto depthReference = vks::initializers::attachmentReference(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    VkSubpassDescription subpassInfo {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorReference,
        .pDepthStencilAttachment = &depthReference,
    };

    std::array<VkAttachmentDescription,2> attachmentDescriptions {colorDescription, depthDescription};
    VkRenderPassCreateInfo renderPassInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size()),
        .pAttachments = attachmentDescriptions.data(),
        .subpassCount = 1,
        .pSubpasses = &subpassInfo,
        .dependencyCount = 0,
    };

    vkCheck(vkCreateRenderPass(device.vkDevice, &renderPassInfo, nullptr, &framebuffer.vkRenderPass));

    framebuffer.vkFrameBuffers.resize(nrImages);
    for(int i=0; i<nrImages; i++) {
        std::array<VkImageView, 2> attachments { framebuffer.colors[i].view, depthAttachments[i].view };

        VkFramebufferCreateInfo framebufferInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = framebuffer.vkRenderPass,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .width = width,
            .height = height,
            .layers = 1,
        };

        vkCheck(vkCreateFramebuffer(device.vkDevice, &framebufferInfo, nullptr, &framebuffer.vkFrameBuffers[i]));
    }
}

void ForwardPass::createTexturesDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding albedoBinding {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
    };

    VkDescriptorSetLayoutBinding normalBinding {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
    };

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {albedoBinding, normalBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data(),
    };

    vkCheck(vkCreateDescriptorSetLayout(device.vkDevice, &layoutInfo, nullptr, &texturesDescriptorSetLayout));
}

void ForwardPass::createLightBufferDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboBinding {
        .binding = 2,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 1,
            .pBindings = &uboBinding,
    };

    vkCheck(vkCreateDescriptorSetLayout(device.vkDevice, &layoutInfo, nullptr, &lightBufferDescriptorSetLayout));
}

void ForwardPass::createPipelineLayout() {
    VkPushConstantRange pushConstantRange {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(PushConstant),
    };

    std::array<VkDescriptorSetLayout,2> descriptorSetLayouts {texturesDescriptorSetLayout, lightBufferDescriptorSetLayout};
    VkPipelineLayoutCreateInfo pipelineLayoutInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = static_cast<uint32>(descriptorSetLayouts.size()),
            .pSetLayouts = descriptorSetLayouts.data(),
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushConstantRange,
    };

    vkCheck(vkCreatePipelineLayout(device.vkDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout));
}

void ForwardPass::createPipeline() {
    vertShader = device.createShaderModule("assets/shaders_bin/forward.vert.spv");
    fragShader = device.createShaderModule("assets/shaders_bin/forward.frag.spv");

    auto inputAssembly = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    auto rasterization = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    auto colorBlendInfo = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
    auto colorBlend = vks::initializers::pipelineColorBlendStateCreateInfo(1, &colorBlendInfo);
    auto depthStencil = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_FALSE, VK_COMPARE_OP_EQUAL);
    auto viewport = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
    auto multisample = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
    std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    auto dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages {
            vks::initializers::pipelineShaderStageCreateInfo(vertShader, VK_SHADER_STAGE_VERTEX_BIT),
            vks::initializers::pipelineShaderStageCreateInfo(fragShader, VK_SHADER_STAGE_FRAGMENT_BIT),
    };

    auto bindingDescriptions= Vertex::getBindingDescriptions();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    auto vertexInput = vks::initializers::pipelineVertexInputStateCreateInfo(bindingDescriptions, attributeDescriptions);

    VkGraphicsPipelineCreateInfo pipelineInfo {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = static_cast<uint32_t>(shaderStages.size()),
            .pStages = shaderStages.data(),
            .pVertexInputState = &vertexInput,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewport,
            .pRasterizationState = &rasterization,
            .pMultisampleState = &multisample,
            .pDepthStencilState = &depthStencil,
            .pColorBlendState = &colorBlend,
            .pDynamicState = &dynamicState,
            .layout = pipelineLayout,
            .renderPass = framebuffer.vkRenderPass,
            .subpass = 0,
    };

    vkCheck(vkCreateGraphicsPipelines(device.vkDevice, nullptr, 1, &pipelineInfo, nullptr, &pipeline));
}

void ForwardPass::createLightBuffers(uint32_t nrImages) {
    lightBuffers.resize(nrImages);
    lightBufferMemories.resize(nrImages);
    lightBuffersMapped.resize(nrImages);
    VkDeviceSize bufferSize = sizeof(LightBuffer);
    for(int i=0; i<nrImages; i++) {
        device.createHostBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &lightBuffers[i], &lightBufferMemories[i]);
        vkCheck(vmaMapMemory(device.vmaAllocator, lightBufferMemories[i], (void**)&lightBuffersMapped[i]));
        auto& buffer = *lightBuffersMapped[i];
        buffer.falloffConstant = 1.0f;
        buffer.falloffLinear = 1.0f;
        buffer.falloffQuadratic = 1.0f;
        buffer.lightCount = 0;
    }
}

void ForwardPass::allocateLightDescriptorSets(uint32_t nrImages) {
    lightBufferDescriptorSets.resize(nrImages);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts(nrImages, lightBufferDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = device.vkDescriptorPool,
        .descriptorSetCount = nrImages,
        .pSetLayouts = descriptorSetLayouts.data(),
    };

    vkCheck(vkAllocateDescriptorSets(device.vkDevice, &allocInfo, lightBufferDescriptorSets.data()));
}

void ForwardPass::createLightDescriptorSets(uint32_t nrImages) {
    for(int i=0; i<nrImages; i++) {
        VkDescriptorBufferInfo lightDataDescriptorInfo {
            .buffer = lightBuffers[i],
            .offset = 0,
            .range = VK_WHOLE_SIZE,
        };

        auto write = vks::initializers::writeDescriptorSet(lightBufferDescriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &lightDataDescriptorInfo, 1);
        vkUpdateDescriptorSets(device.vkDevice, 1, &write, 0, nullptr);
    }
}

void ForwardPass::updateLights(LightComponent *lightData, uint32_t nrLights, uint32_t imageIdx) {
    lightBuffersMapped[imageIdx]->lightCount = nrLights;
    memcpy(lightBuffersMapped[imageIdx]->lightData, lightData, nrLights * sizeof(LightComponent));
}

void ForwardPass::recreateFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages,
                                      const std::vector<EvFrameBufferAttachment> &depthAttachments) {
    framebuffer.destroy(device);
    createFramebuffer(width, height, nrImages, depthAttachments);
}

void ForwardPass::startPass(VkCommandBuffer cmdBuffer, uint32_t imageIdx) const {
    std::array<VkClearValue, 2> clearValues {
        VkClearValue {.color = {0.0f, 0.0f, 0.0f, 0.0f}},
        VkClearValue { .depthStencil = {0.0f, 1}},
    };

    VkRenderPassBeginInfo renderPassInfo {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = framebuffer.vkRenderPass,
            .framebuffer = framebuffer.vkFrameBuffers[imageIdx],
            .renderArea = {
                    .offset = {0,0},
                    .extent = {framebuffer.width, framebuffer.height},
            },
            .clearValueCount = static_cast<uint32_t>(clearValues.size()),
            .pClearValues = clearValues.data(),
    };
    VkViewport viewport {
            .x = 0.0f,
            .y = static_cast<float>(framebuffer.height),
            .width = static_cast<float>(framebuffer.width),
            .height = -static_cast<float>(framebuffer.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };
    VkRect2D scissor{{0,0}, {framebuffer.width, framebuffer.height}};

    vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &lightBufferDescriptorSets[imageIdx], 0, nullptr);
}

void ForwardPass::endPass(VkCommandBuffer cmdBuffer) const {
    vkCmdEndRenderPass(cmdBuffer);
}
