#pragma once

#include "EvDevice.h"
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

struct EvRastPipelineInfo {
    VkViewport viewPort;
    VkRect2D scissor;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo;
    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo;
    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo;
    VkPipelineColorBlendAttachmentState blendAttachmentState;
    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;
    VkRenderPass renderPass = nullptr;
    VkPipelineLayout layout = nullptr;
    uint subpass = 0;
};

class EvRastPipeline : NoCopy {
private:
    const EvDevice& device;
    VkPipeline vkPipeline;
    VkShaderModule vkVertShaderModule;
    VkShaderModule vkFragShaderModule;

    void createGraphicsPipeline(EvRastPipelineInfo info, const char *vertFile, const char *fragFile);
    VkShaderModule createShaderModule(const std::vector<char>& code) const;

public:
    EvRastPipeline(const EvDevice& device, const EvRastPipelineInfo &info, const char *vertFile, const char *fragFile);
    ~EvRastPipeline();

    void bind(VkCommandBuffer commandBuffer) const;
};

inline EvRastPipelineInfo defaultRastPipelineInfo(uint width, uint height) {
    VkViewport viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(width),
        .height = static_cast<float>(height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor {
        .offset = {0,0},
        .extent = {width, height},
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

    return EvRastPipelineInfo {
        .viewPort = viewport,
        .scissor = scissor,
        .inputAssemblyStateCreateInfo = inputAssemblyStateCreateInfo,
        .rasterizationStateCreateInfo = rasterizationStateCreateInfo,
        .multisampleStateCreateInfo = multisampleStateCreateInfo,
        .blendAttachmentState = blendAttachmentState,
        .depthStencilStateCreateInfo = depthStencilStateCreateInfo,
    };
}