#include "EvForwardPass.h"

EvForwardPass::EvForwardPass(EvDevice &device, uint32_t width, uint32_t height, uint32_t nrImages,
                             const std::vector<EvFrameBufferAttachment> &depthAttachments,
                             const std::vector<EvFrameBufferAttachment> &composedImageAttachments)
                             : device(device) {
    createBuffer(width, height, nrImages, depthAttachments, composedImageAttachments);
    createDescriptorSetLayout();
    createPipeline();
}

EvForwardPass::~EvForwardPass() {
    vkDestroyShaderModule(device.vkDevice, vertShader, nullptr);
    vkDestroyShaderModule(device.vkDevice, fragShader, nullptr);
    vkDestroyDescriptorSetLayout(device.vkDevice, descriptorSetLayout, nullptr);
    framebuffer.destroy(device);
    vkDestroyPipeline(device.vkDevice, pipeline, nullptr);
    vkDestroyPipelineLayout(device.vkDevice, pipelineLayout, nullptr);
}

void EvForwardPass::createBuffer(uint32_t width, uint32_t height, uint32_t nrImages,
                                 const std::vector<EvFrameBufferAttachment> &depthAttachments,
                                 const std::vector<EvFrameBufferAttachment> &composedAttachments) {
    assert(nrImages > 0);
    assert(depthAttachments.size() == nrImages);
    assert(composedAttachments.size() == nrImages);

    framebuffer.width = width;
    framebuffer.height = height;

    VkAttachmentDescription defaultDescription {
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    };

    VkAttachmentDescription composedDescription = defaultDescription;
    composedDescription.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    composedDescription.format = composedAttachments[0].format;
    composedDescription.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    composedDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentDescription depthDescription = defaultDescription;
    // We actually want to get the depth values from the gpass
    depthDescription.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    depthDescription.format = depthAttachments[0].format;
    depthDescription.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    std::array<VkAttachmentDescription,2> attachmentDescriptions { composedDescription, depthDescription };

    VkAttachmentReference composedReference {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depthAttachmentRef {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &composedReference,
            .pDepthStencilAttachment = &depthAttachmentRef,
    };

    std::array<VkSubpassDependency, 2> dependencies {
            VkSubpassDependency {
                    .srcSubpass = VK_SUBPASS_EXTERNAL,
                    .dstSubpass = 0,
                    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                    .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
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

    vkCheck(vkCreateRenderPass(device.vkDevice, &renderpassInfo, nullptr, &framebuffer.vkRenderPass));

    framebuffer.vkFrameBuffers.resize(nrImages);

    for(int i=0; i<nrImages; i++) {
        std::array<VkImageView, 2> attachments{
                composedAttachments[i].view,
                depthAttachments[i].view,
        };

        VkFramebufferCreateInfo framebufferInfo{
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = framebuffer.vkRenderPass,
                .attachmentCount = static_cast<uint32_t>(attachments.size()),
                .pAttachments = attachments.data(),
                .width = framebuffer.width,
                .height= framebuffer.height,
                .layers = 1,
        };

        vkCheck(vkCreateFramebuffer(device.vkDevice, &framebufferInfo, nullptr, &framebuffer.vkFrameBuffers[i]));
    }
}

void EvForwardPass::createDescriptorSetLayout() {
    VkDescriptorSetLayoutCreateInfo layoutInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 0,
    };

    vkCheck(vkCreateDescriptorSetLayout(device.vkDevice, &layoutInfo, nullptr, &descriptorSetLayout));
}

void EvForwardPass::createPipeline() {

    VkPushConstantRange pushConstantRange{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(ForwardPush),
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &descriptorSetLayout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushConstantRange,
    };

    vkCheck(vkCreatePipelineLayout(device.vkDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout));

    vertShader = device.createShaderModule("assets/shaders_bin/forward.vert.spv");
    fragShader = device.createShaderModule("assets/shaders_bin/forward.frag.spv");

    auto inputAssembly = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    auto rasterization = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    auto blendAttachmentComposed = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
    std::array<VkPipelineColorBlendAttachmentState, 1> blendAttachments { blendAttachmentComposed };
    auto colorBlend = vks::initializers::pipelineColorBlendStateCreateInfo(static_cast<uint32_t>(blendAttachments.size()), blendAttachments.data());
    auto depthStencil = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
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

void EvForwardPass::recreateFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages,
                                        const std::vector<EvFrameBufferAttachment> &depthAttachments,
                                        const std::vector<EvFrameBufferAttachment> &composedImageAttachments) {
    framebuffer.destroy(device);
    createBuffer(width, height, nrImages, depthAttachments, composedImageAttachments);
}

void EvForwardPass::beginPass(VkCommandBuffer commandBuffer, uint32_t imageIdx) const {
    std::array<VkClearValue, 2> clearValues {
            VkClearValue { .color = {0.0f, 0.0f, 0.0f, 0.0f}, },
            // should be ignore since the load op is not CLEAR
            VkClearValue { .depthStencil = {1.0f, 0}, },
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

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void EvForwardPass::endPass(VkCommandBuffer commandBuffer) const {
    vkCmdEndRenderPass(commandBuffer);
}
