#pragma once

#include "EvDevice.h"
#include "EvModel.h"
#include "EvTexture.h"
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
    VkPipelineLayout pipelineLayout = nullptr;
    VkDescriptorSetLayout descriptorSetLayout = nullptr;
    VkRenderPass renderPass = nullptr;
    uint subpass = 0;
    uint swapchainImages = 0;

    void assertComplete() const {
        assert(pipelineLayout);
        assert(descriptorSetLayout);
        assert(renderPass);
        assert(swapchainImages > 0);
    }
};

class EvRastPipeline : NoCopy {
private:
    const EvDevice& device;
    VkPipeline vkPipeline;

    void createGraphicsPipeline(const EvRastPipelineInfo& info);

public:
    EvRastPipeline(const EvDevice& device, const EvRastPipelineInfo &info);
    ~EvRastPipeline();

    static void defaultRastPipelineInfo(VkShaderModule vertShaderModule, VkShaderModule fragShaderModule, VkSampleCountFlagBits msaaSamples, EvRastPipelineInfo* info);
    void bind(VkCommandBuffer commandBuffer) const;
};