#include "EvRastPipeline.h"

#include <cassert>

#include "utils.h"

EvRastPipeline::EvRastPipeline(const EvDevice &device,
                               const EvRastPipelineInfo &info,
                               const char *vertFile,
                               const char *fragFile)
                               : device(device) {
    createGraphicsPipeline(info, vertFile, fragFile);
}

EvRastPipeline::~EvRastPipeline() {
    printf("destroying shader modules\n");
    vkDestroyShaderModule(device.vkDevice, vkVertShaderModule, nullptr);
    vkDestroyShaderModule(device.vkDevice, vkFragShaderModule, nullptr);

    printf("Destroying the pipeline\n");
    vkDestroyPipeline(device.vkDevice, vkPipeline, nullptr);
}


void EvRastPipeline::createGraphicsPipeline(EvRastPipelineInfo info, const char *vertFile, const char *fragFile) {
    assert(info.renderPass != VK_NULL_HANDLE);

    auto vertCode = readFile(vertFile);
    auto fragCode = readFile(fragFile);

    printf("vertex shader:   %lu bytes\n", vertCode.size());
    printf("fragment shader: %lu bytes\n", vertCode.size());

    vkVertShaderModule = createShaderModule(vertCode);
    vkFragShaderModule = createShaderModule(fragCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vkVertShaderModule,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = vkFragShaderModule,
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

    VkPipelineViewportStateCreateInfo viewportInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &info.viewPort,
        .scissorCount = 1,
        .pScissors = &info.scissor,
    };

    VkPipelineColorBlendStateCreateInfo blendStateCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_NO_OP,
            .attachmentCount = 1,
            .pAttachments = &info.blendAttachmentState,
    };

    VkGraphicsPipelineCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &info.inputAssemblyStateCreateInfo,
        .pViewportState = &viewportInfo,
        .pRasterizationState = &info.rasterizationStateCreateInfo,
        .pMultisampleState = &info.multisampleStateCreateInfo,
        .pDepthStencilState = &info.depthStencilStateCreateInfo,
        .pColorBlendState = &blendStateCreateInfo,
        .pDynamicState = nullptr,
        .layout = info.layout,
        .renderPass = info.renderPass,
        .subpass = info.subpass,
    };

    vkCheck(vkCreateGraphicsPipelines(device.vkDevice, VK_NULL_HANDLE, 1, &createInfo, nullptr, &vkPipeline));
}


VkShaderModule EvRastPipeline::createShaderModule(const std::vector<char>& code) const {
    VkShaderModuleCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = code.size(),
            .pCode = reinterpret_cast<const uint32_t*>(code.data()),
    };

    VkShaderModule shaderModule;
    vkCheck(vkCreateShaderModule(device.vkDevice, &createInfo, nullptr, &shaderModule));
    return shaderModule;
}

