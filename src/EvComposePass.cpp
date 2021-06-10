#include "EvComposePass.h"

EvComposePass::EvComposePass(EvDevice &device, uint32_t width, uint32_t height, uint32_t nrImages,
                             const std::vector<VkImageView> &normalViews,
                             const std::vector<VkImageView> &posViews,
                             const std::vector<VkImageView> &albedoViews)
                             : device(device) {
    createBuffer(width, height, nrImages);
    createDescriptorSetLayout();
    allocateDescriptorSets(nrImages);
    createDescriptorSets(nrImages, normalViews, posViews, albedoViews);
    createPipeline();
}

EvComposePass::~EvComposePass() {
    vkDestroySampler(device.vkDevice, normalSampler, nullptr);
    vkDestroySampler(device.vkDevice, posSampler, nullptr);
    vkDestroySampler(device.vkDevice, albedoSampler, nullptr);
    vkDestroyShaderModule(device.vkDevice, vertShader, nullptr);
    vkDestroyShaderModule(device.vkDevice, fragShader, nullptr);
    framebuffer.destroy(device);
    vkDestroyDescriptorSetLayout(device.vkDevice, descriptorSetLayout, nullptr);
    vkDestroyPipeline(device.vkDevice, pipeline, nullptr);
    vkDestroyPipelineLayout(device.vkDevice, pipelineLayout, nullptr);
}

void EvComposePass::createBuffer(uint32_t width, uint32_t height, uint32_t nrImages) {
    assert(nrImages > 0);
    framebuffer.width = width;
    framebuffer.height = height;

    framebuffer.composeds.resize(nrImages);

    for(int i=0; i<nrImages; i++) {
        device.createAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, framebuffer.width, framebuffer.height, &framebuffer.composeds[i]);
    };

    VkAttachmentDescription composedDescription {
        .format = framebuffer.composeds[0].format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkAttachmentReference composedReference {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    std::array<VkAttachmentDescription, 1> attachmentDescriptions {composedDescription };
    std::array<VkAttachmentReference, 1> colorReferences { composedReference };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = static_cast<uint32_t>(colorReferences.size()),
        .pColorAttachments = colorReferences.data(),
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

    VkRenderPassCreateInfo renderPassInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size()),
        .pAttachments = attachmentDescriptions.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = static_cast<uint32_t>(dependencies.size()),
        .pDependencies = dependencies.data(),
    };

    vkCheck(vkCreateRenderPass(device.vkDevice, &renderPassInfo, nullptr, &framebuffer.vkRenderPass));

    framebuffer.vkFrameBuffers.resize(nrImages);

    for(int i=0; i<nrImages; i++) {
        std::array<VkImageView, 1> attachments{
                framebuffer.composeds[i].view,
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

void EvComposePass::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding normalBinding {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
    };

    VkDescriptorSetLayoutBinding posBinding {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
    };

    VkDescriptorSetLayoutBinding albedoBinding {
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
    };

    std::array<VkDescriptorSetLayoutBinding, 3> bindings = {normalBinding, posBinding, albedoBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()
    };

    vkCheck(vkCreateDescriptorSetLayout(device.vkDevice, &layoutInfo, nullptr, &descriptorSetLayout));
}

void EvComposePass::allocateDescriptorSets(uint32_t nrImages) {
    descriptorSets.resize(nrImages);
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts(nrImages, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = device.vkDescriptorPool,
            .descriptorSetCount = nrImages,
            .pSetLayouts = descriptorSetLayouts.data(),
    };

    vkCheck(vkAllocateDescriptorSets(device.vkDevice, &allocInfo, descriptorSets.data()));

    VkSamplerCreateInfo samplerInfo = vks::initializers::samplerCreateInfo(device.vkPhysicalDeviceProperties.limits.maxSamplerAnisotropy);
    vkCheck(vkCreateSampler(device.vkDevice, &samplerInfo, nullptr, &normalSampler));
    vkCheck(vkCreateSampler(device.vkDevice, &samplerInfo, nullptr, &posSampler));
    vkCheck(vkCreateSampler(device.vkDevice, &samplerInfo, nullptr, &albedoSampler));
}

void EvComposePass::createDescriptorSets(
        uint32_t nrImages,
        const std::vector<VkImageView>& normalViews,
        const std::vector<VkImageView>& posViews,
        const std::vector<VkImageView>& albedoViews) {

    assert(normalViews.size() == nrImages);
    assert(posViews.size() == nrImages);
    assert(albedoViews.size() == nrImages);

    for(int i=0; i<nrImages; i++) {
        VkDescriptorImageInfo normalDescriptorInfo{
                .sampler = normalSampler,
                .imageView = normalViews[i],
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VkDescriptorImageInfo posDescriptorInfo{
                .sampler = posSampler,
                .imageView = posViews[i],
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VkDescriptorImageInfo albedoDescriptorInfo{
                .sampler = albedoSampler,
                .imageView = albedoViews[i],
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        std::array<VkDescriptorImageInfo, 3> descriptors{
                normalDescriptorInfo,
                posDescriptorInfo,
                albedoDescriptorInfo,
        };

        VkWriteDescriptorSet writeDescriptors{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = static_cast<uint32_t>(descriptors.size()),
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = descriptors.data(),
        };

        vkUpdateDescriptorSets(device.vkDevice, 1, &writeDescriptors, 0, nullptr);
    }
}

void EvComposePass::createPipeline() {
    VkPushConstantRange pushConstant {
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(glm::vec3),
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstant,
    };

    vkCheck(vkCreatePipelineLayout(device.vkDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout));

    vertShader = device.createShaderModule("assets/shaders_bin/screenfill.vert.spv");
    fragShader = device.createShaderModule("assets/shaders_bin/compose.frag.spv");

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

void EvComposePass::recreateFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages,
                                        const std::vector<VkImageView> &normalViews,
                                        const std::vector<VkImageView> &posViews,
                                        const std::vector<VkImageView> &albedoViews) {
    framebuffer.destroy(device);
    createBuffer(width, height, nrImages);
    createDescriptorSets(nrImages, normalViews, posViews, albedoViews);
}



void EvComposePass::render(VkCommandBuffer commandBuffer, uint32_t imageIdx, const EvCamera& camera) const {
    assert(imageIdx >= 0 && imageIdx < framebuffer.vkFrameBuffers.size());
    std::array<VkClearValue, 1> clearValues {
        VkClearValue { .color = {0.0f, 0.0f, 0.0f, 0.0f}, },
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
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::vec3), &camera.position);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[imageIdx], 0, nullptr);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
}

