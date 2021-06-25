#pragma once

#include "../core.h"
#include "../EvDevice.h"
#include "../ShaderTypes.h"
#include "../Primitives.h"

class DepthPass : NoCopy
{
    EvDevice& device;

    struct Buffer : public EvFrameBuffer {
        std::vector<EvFrameBufferAttachment> depths;

        virtual void destroy(EvDevice& device) override {
            for(auto& depth : depths) depth.destroy(device);
            EvFrameBuffer::destroy(device);
        }
    } framebuffer;

    VkShaderModule vertShader;
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    void createFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages);
    void createPipelineLayout();
    void createPipeline();

public:
    DepthPass(EvDevice& device, uint32_t width, uint32_t height, uint32_t nrImages);
    ~DepthPass();

    inline Buffer& getFramebuffer() { return framebuffer; }
    inline VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }

    void recreateFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages);
    void startPass(VkCommandBuffer cmdBuffer, uint32_t imageIdx) const;
    void endPass(VkCommandBuffer cmdBuffer) const;
};
