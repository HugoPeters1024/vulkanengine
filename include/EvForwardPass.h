#pragma once

#include "core.h"
#include "EvDevice.h"
#include "Primitives.h"

struct ForwardPush {
    glm::mat4 camera;
    glm::mat4 mvp;
};

class EvForwardPass {
    EvDevice& device;

    struct ForwardBuffer : public EvFrameBuffer {

    } framebuffer;

    VkShaderModule vertShader;
    VkShaderModule fragShader;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    VkSampler composedSampler;

    void createBuffer(uint32_t width, uint32_t height, uint32_t nrImages,
                      const std::vector<EvFrameBufferAttachment>& depthAttachments,
                      const std::vector<EvFrameBufferAttachment>& composedAttachments);
    void createDescriptorSetLayout();
    void createPipeline();

public:
    EvForwardPass(EvDevice& device, uint32_t width, uint32_t height, uint32_t nrImages,
                  const std::vector<EvFrameBufferAttachment> &depthAttachments,
                  const std::vector<EvFrameBufferAttachment> &composedImageAttachments);
    ~EvForwardPass();

    void recreateFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages,
                             const std::vector<EvFrameBufferAttachment> &depthAttachments,
                             const std::vector<EvFrameBufferAttachment> &composedImageAttachments);
    void beginPass(VkCommandBuffer commandBuffer, uint32_t imageIdx) const;
    void endPass(VkCommandBuffer commandBuffer) const;
};
