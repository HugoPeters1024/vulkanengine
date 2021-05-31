#include "EvRastPipeline.h"

#include <cassert>

#include "utils.hpp"

EvRastPipeline::EvRastPipeline(const EvDevice &device, const EvRastPipelineInfo &info)
                               : device(device) {
    info.assertComplete();
    createGraphicsPipeline(info);
}

EvRastPipeline::~EvRastPipeline() {
    printf("Destroying the pipeline\n");
    vkDestroyPipeline(device.vkDevice, vkPipeline, nullptr);
}


void EvRastPipeline::createGraphicsPipeline(const EvRastPipelineInfo& info) {
    assert(info.renderPass != VK_NULL_HANDLE);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = info.vertShaderModule,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = info.fragShaderModule,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo shaderStages[2] = { vertShaderStageInfo, fragShaderStageInfo };

    auto bindingDescriptions = Vertex::getBindingDescriptions();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size()),
        .pVertexBindingDescriptions = bindingDescriptions.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data(),
    };


    VkPipelineColorBlendStateCreateInfo blendStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &info.blendAttachmentState,
    };

    VkGraphicsPipelineCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &info.inputAssemblyStateCreateInfo,
        .pViewportState = &info.viewportInfo,
        .pRasterizationState = &info.rasterizationStateCreateInfo,
        .pMultisampleState = &info.multisampleStateCreateInfo,
        .pDepthStencilState = &info.depthStencilStateCreateInfo,
        .pColorBlendState = &blendStateCreateInfo,
        .pDynamicState = &info.dynamicStateCreateInfo,
        .layout = info.pipelineLayout,
        .renderPass = info.renderPass,
        .subpass = info.subpass,
    };

    vkCheck(vkCreateGraphicsPipelines(device.vkDevice, VK_NULL_HANDLE, 1, &createInfo, nullptr, &vkPipeline));
}


void EvRastPipeline::bind(VkCommandBuffer commandBuffer) const {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline);
}

void EvRastPipeline::defaultRastPipelineInfo(VkShaderModule vertShaderModule, VkShaderModule fragShaderModule, VkSampleCountFlagBits msaaSamples,
                                             EvRastPipelineInfo *info)
{
    VkPipelineViewportStateCreateInfo viewportInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = nullptr,
            .scissorCount = 1,
            .pScissors = nullptr,
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
    };

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = msaaSamples,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 1.0f,
            .pSampleMask = nullptr,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE,
    };

    VkPipelineColorBlendAttachmentState blendAttachmentState {
            .blendEnable = VK_FALSE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };


    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE,
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f,
    };

    info->dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    info->dynamicStateCreateInfo = VkPipelineDynamicStateCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .flags = 0,
            .dynamicStateCount = static_cast<uint32_t >(info->dynamicStateEnables.size()),
            .pDynamicStates = info->dynamicStateEnables.data(),
    };

    info->vertShaderModule = vertShaderModule;
    info->fragShaderModule = fragShaderModule;
    info->viewportInfo = viewportInfo;
    info->inputAssemblyStateCreateInfo = inputAssemblyStateCreateInfo;
    info->rasterizationStateCreateInfo = rasterizationStateCreateInfo;
    info->multisampleStateCreateInfo = multisampleStateCreateInfo;
    info->blendAttachmentState = blendAttachmentState;
    info->depthStencilStateCreateInfo = depthStencilStateCreateInfo;
}


