#include "EvPostPass.h"

EvPostPass::EvPostPass(EvDevice &device, uint32_t width, uint32_t height, uint32_t nrImages,
                       VkFormat swapchainFormat,
                       const std::vector<VkImageView> &swapchainImageViews,
                       const std::vector<VkImageView> &composedImageViews)
                       : device(device) {
    createBuffer(width, height, nrImages, swapchainFormat, swapchainImageViews);
    createDescriptorSetLayout();
    allocateDescriptorSets(nrImages);
    createDescriptorSets(nrImages, composedImageViews);
    createPipeline();
}

EvPostPass::~EvPostPass() {
    vkDestroySampler(device.vkDevice, composedSampler, nullptr);
    vkDestroyShaderModule(device.vkDevice, vertShader, nullptr);
    vkDestroyShaderModule(device.vkDevice, fragShader, nullptr);
    framebuffer.destroy(device);
    vkDestroyDescriptorSetLayout(device.vkDevice, descriptorSetLayout, nullptr);
    vkDestroyPipeline(device.vkDevice, pipeline, nullptr);
    vkDestroyPipelineLayout(device.vkDevice, pipelineLayout, nullptr);
}

void EvPostPass::createBuffer(uint32_t width, uint32_t height, uint32_t nrImages, VkFormat swapchainFormat,
                              const std::vector<VkImageView> &swapchainImageViews) {
    assert(nrImages > 0);
    framebuffer.width = width;
    framebuffer.height = height;

    VkAttachmentDescription colorAttachment {
            .format = swapchainFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
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

    VkSubpassDescription subpass {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentRef,
            .pDepthStencilAttachment = nullptr,
    };

    VkSubpassDependency dependency {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT + VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
    };

    VkRenderPassCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &colorAttachment,
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 1,
            .pDependencies = &dependency,
    };

    vkCheck(vkCreateRenderPass(device.vkDevice, &createInfo, nullptr, &framebuffer.vkRenderPass));

    framebuffer.vkFrameBuffers.resize(nrImages);
    for(int i=0; i<nrImages; i++) {
        std::array<VkImageView, 1> attachments{
                swapchainImageViews[i],
        };

        VkFramebufferCreateInfo framebufferInfo{
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = framebuffer.vkRenderPass,
                .attachmentCount = static_cast<uint32_t>(attachments.size()),
                .pAttachments = attachments.data(),
                .width = framebuffer.width,
                .height = framebuffer.height,
                .layers = 1,
        };

        vkCheck(vkCreateFramebuffer(device.vkDevice, &framebufferInfo, nullptr, &framebuffer.vkFrameBuffers[i]));
    }
}

void EvPostPass::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding composedBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 1,
            .pBindings = &composedBinding,
    };

    vkCheck(vkCreateDescriptorSetLayout(device.vkDevice, &layoutInfo, nullptr, &descriptorSetLayout));
}

void EvPostPass::allocateDescriptorSets(uint32_t nrImages) {
    descriptorSets.resize(nrImages);
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts(2, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = device.vkDescriptorPool,
            .descriptorSetCount = nrImages,
            .pSetLayouts = descriptorSetLayouts.data(),
    };

    vkCheck(vkAllocateDescriptorSets(device.vkDevice, &allocInfo, descriptorSets.data()));

    VkSamplerCreateInfo samplerInfo = vks::initializers::samplerCreateInfo(device.vkPhysicalDeviceProperties.limits.maxSamplerAnisotropy);
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.anisotropyEnable = false;
    vkCheck(vkCreateSampler(device.vkDevice, &samplerInfo, nullptr, &composedSampler));
}

void EvPostPass::createDescriptorSets(uint32_t nrImages, const std::vector<VkImageView> &composedViews) {
    assert(composedViews.size() == nrImages);

    for(int i=0; i<nrImages; i++) {
        VkDescriptorImageInfo composedDescriptorInfo{
                .sampler = composedSampler,
                .imageView = composedViews[i],
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VkWriteDescriptorSet writeDescriptors{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &composedDescriptorInfo,
        };

        vkUpdateDescriptorSets(device.vkDevice, 1, &writeDescriptors, 0, nullptr);
    }
}

void EvPostPass::createPipeline() {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorSetLayout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };

    vkCheck(vkCreatePipelineLayout(device.vkDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout));

    vertShader = device.createShaderModule("assets/shaders_bin/screenfill.vert.spv");
    fragShader = device.createShaderModule("assets/shaders_bin/post.frag.spv");

    auto inputAssembly = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    auto rasterization = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
    auto blendAttachment = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
    auto colorBlend = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachment);
    auto depthStencil = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);
    auto viewport = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
    auto multisample = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
    std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    auto dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages {
        vks::initializers::pipelineShaderStageCreateInfo(vertShader, VK_SHADER_STAGE_VERTEX_BIT),
        vks::initializers::pipelineShaderStageCreateInfo(fragShader, VK_SHADER_STAGE_FRAGMENT_BIT),
    };
    auto vertexInput = vks::initializers::pipelineVertexInputStateCreateInfo();

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
        .subpass = 0
    };

    vkCheck(vkCreateGraphicsPipelines(device.vkDevice, nullptr, 1, &pipelineInfo, nullptr, &pipeline));
}

void EvPostPass::recreateFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages,
                                     const std::vector<VkImageView> &composedViews, VkFormat swapchainFormat,
                                     const std::vector<VkImageView> &swapchainImageViews) {
    framebuffer.destroy(device);
    createBuffer(width, height, nrImages, swapchainFormat, swapchainImageViews);
    createDescriptorSets(nrImages, composedViews);
}

void EvPostPass::beginPass(VkCommandBuffer commandBuffer, uint32_t imageIdx) const {
    assert(imageIdx >= 0 && imageIdx < framebuffer.vkFrameBuffers.size());
    std::array<VkClearValue, 1> clearValues {
            VkClearValue { .color = {0.0f, 0.1f, 0.1f, 1.0f}, },
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
            .y = 0.0f,
            .width = static_cast<float>(framebuffer.width),
            .height = static_cast<float>(framebuffer.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };
    VkRect2D scissor{{0,0}, {framebuffer.width, framebuffer.height}};

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[imageIdx], 0, nullptr);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void EvPostPass::endPass(VkCommandBuffer commandBuffer) const {
    vkCmdEndRenderPass(commandBuffer);
}
