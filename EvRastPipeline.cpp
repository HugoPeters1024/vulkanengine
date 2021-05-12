#include "EvRastPipeline.h"

#include <cassert>

#include "utils.h"

EvRastPipeline::EvRastPipeline(const EvDevice &device, const EvRastPipelineInfo &info)
                               : device(device) {
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

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr,
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
        .layout = info.layout,
        .renderPass = info.renderPass,
        .subpass = info.subpass,
    };

    vkCheck(vkCreateGraphicsPipelines(device.vkDevice, VK_NULL_HANDLE, 1, &createInfo, nullptr, &vkPipeline));
}

void EvRastPipeline::bind(VkCommandBuffer commandBuffer) const {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline);
}

