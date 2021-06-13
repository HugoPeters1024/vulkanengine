#pragma once

#include <vulkan/vulkan.h>
#include "EvDevice.h"
#include "ShaderTypes.h"
#include "Primitives.h"
#include "VulkanInitializers.hpp"
#include "EvCamera.h"

class EvGPass {
private:
    EvDevice& device;

    struct GBuffer : public EvFrameBuffer {
        std::vector<EvFrameBufferAttachment> positions;
        std::vector<EvFrameBufferAttachment> albedos;
        std::vector<EvFrameBufferAttachment> depths;

        virtual void destroy(EvDevice& device) override {
            for(auto& position : positions) position.destroy(device);
            for(auto& albedo : albedos) albedo.destroy(device);
            for(auto& depth : depths) depth.destroy(device);
            EvFrameBuffer::destroy(device);
        }
    } framebuffer;
    VkShaderModule vertShader;
    VkShaderModule fragShader;
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSetLayout descriptorSetLayout;

    void createFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages);
    void createDescriptorSetLayout();
    void createPipeline();

public:
    EvGPass(EvDevice& device, uint32_t width, uint32_t height, uint32_t nrImages);
    ~EvGPass();

    inline GBuffer& getFramebuffer() { return framebuffer; }
    inline VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
    inline VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }
    inline std::vector<VkImageView> getPosViews() const {
        std::vector<VkImageView> ret{};
        for(const auto& pos : framebuffer.positions) {
            ret.push_back(pos.view);
        }
        return ret;
    }
    inline std::vector<VkImageView> getAlbedoViews() const {
        std::vector<VkImageView> ret{};
        for(const auto& albedo : framebuffer.albedos) {
            ret.push_back(albedo.view);
        }
        return ret;
    }
    inline std::vector<VkImageView> getDepthViews() const {
        std::vector<VkImageView> ret{};
        for(const auto& depth : framebuffer.depths) {
            ret.push_back(depth.view);
        }
        return ret;
    }

    void recreateFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages);
    void startPass(VkCommandBuffer cmdBuffer, uint32_t imageIdx) const;
    void endPass(VkCommandBuffer cmdBuffer) const;
};
