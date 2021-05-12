#pragma once

#include "EvDevice.h"
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

struct EvRastPipelineInfo : NoCopy {
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
    VkPipelineViewportStateCreateInfo viewportInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo;
    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo;
    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo;
    VkPipelineColorBlendAttachmentState blendAttachmentState;
    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;
    std::vector<VkDynamicState> dynamicStateEnables;
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo;
    VkRenderPass renderPass = nullptr;
    VkPipelineLayout layout = nullptr;
    uint subpass = 0;
};

class EvRastPipeline : NoCopy {
private:
    const EvDevice& device;
    VkPipeline vkPipeline;

    void createGraphicsPipeline(const EvRastPipelineInfo& info);

public:
    EvRastPipeline(const EvDevice& device, const EvRastPipelineInfo &info);
    ~EvRastPipeline();

    void bind(VkCommandBuffer commandBuffer) const;
};

inline void defaultRastPipelineInfo(VkShaderModule vertShaderModule, VkShaderModule fragShaderModule, EvRastPipelineInfo* info) {
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
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
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