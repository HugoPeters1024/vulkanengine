#include "RenderPasses/DepthPass.h"

DepthPass::DepthPass(EvDevice &device, uint32_t width, uint32_t height, uint32_t nrImages) : device(device) {
    createFramebuffer(width, height, nrImages);
    createPipelineLayout();
    createPipeline();
}

DepthPass::~DepthPass() {
    framebuffer.destroy(device);
    vkDestroyPipeline(device.vkDevice, pipeline, nullptr);
    vkDestroyPipelineLayout(device.vkDevice, pipelineLayout, nullptr);
    vkDestroyShaderModule(device.vkDevice, vertShader, nullptr);
}

void DepthPass::createFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages) {
    assert(nrImages > 0);
    framebuffer.width = width;
    framebuffer.height = height;

    framebuffer.depths.resize(nrImages);

    auto depthFormat = device.findDepthFormat();
    for(int i=0; i<nrImages; i++) {
        device.createAttachment(
                depthFormat,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_SAMPLE_COUNT_1_BIT,
                width, height,
                &framebuffer.depths[i]);
    }


    auto depthDescription = vks::initializers::attachmentDescription(depthFormat, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    depthDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    auto depthRef = vks::initializers::attachmentReference(0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 0,
        .pDepthStencilAttachment = &depthRef,
    };

    VkRenderPassCreateInfo renderPassInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &depthDescription,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 0,
    };

    vkCheck(vkCreateRenderPass(device.vkDevice, &renderPassInfo, nullptr, &framebuffer.vkRenderPass));

    framebuffer.vkFrameBuffers.resize(nrImages);
    for(int i=0; i<nrImages; i++) {
        VkFramebufferCreateInfo framebufferInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = framebuffer.vkRenderPass,
            .attachmentCount = 1,
            .pAttachments = &framebuffer.depths[i].view,
            .width = width,
            .height = height,
            .layers = 1,
        };

        vkCheck(vkCreateFramebuffer(device.vkDevice, &framebufferInfo, nullptr, &framebuffer.vkFrameBuffers[i]));
    }
}

void DepthPass::createPipelineLayout() {
    VkPushConstantRange pushConstant {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(PushConstant),
    };

    VkPipelineLayoutCreateInfo layoutInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstant,
    };

    vkCheck(vkCreatePipelineLayout(device.vkDevice, &layoutInfo, nullptr, &pipelineLayout));
}

void DepthPass::createPipeline() {
    vertShader = device.createShaderModule("assets/shaders_bin/depth.vert.spv");

    auto inputAssembly = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    auto rasterization = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    auto colorBlend = vks::initializers::pipelineColorBlendStateCreateInfo(0, nullptr);
    auto depthStencil = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    auto viewport = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
    auto multisample = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
    std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    auto dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
    std::array<VkPipelineShaderStageCreateInfo, 1> shaderStages {
        vks::initializers::pipelineShaderStageCreateInfo(vertShader, VK_SHADER_STAGE_VERTEX_BIT),
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

void DepthPass::recreateFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages) {
    framebuffer.destroy(device);
    createFramebuffer(width, height, nrImages);
}

void DepthPass::startPass(VkCommandBuffer cmdBuffer, uint32_t imageIdx) const {
    std::array<VkClearValue, 1> clearValues {
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

    vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void DepthPass::endPass(VkCommandBuffer cmdBuffer) const {
    vkCmdEndRenderPass(cmdBuffer);
}
