#pragma once

#include "EvDevice.h"
#include "EvMesh.h"
#include "EvCamera.h"
#include "EvSwapchain.h"
#include "ShaderTypes.h"
#include "EvTexture.h"
#include "EvOverlay.h"
#include "Components.h"
#include "RenderPasses/DepthPass.h"
#include "RenderPasses/ForwardPass.h"
#include "RenderPasses/PostPass.h"

class RenderSystem : public System
{
    struct LightSystem : public System
    {
        inline Signature GetSignature() const override {
            Signature signature;
            signature.set(m_coordinator->GetComponentType<LightComponent>());
            return signature;
        }
    };
    std::shared_ptr<LightSystem> lightSubSystem;

    EvDevice& device;
    std::unique_ptr<EvSwapchain> swapchain;
    std::vector<VkCommandBuffer> commandBuffers;

    std::unique_ptr<EvOverlay> overlay;
    std::unique_ptr<DepthPass> depthPass;
    std::unique_ptr<ForwardPass> forwardPass;
    std::unique_ptr<PostPass> postPass;

    std::vector<std::unique_ptr<EvMesh>> createdMeshes;
    std::vector<std::unique_ptr<EvTexture>> createdTextures;
    std::vector<std::unique_ptr<TextureSet>> createdTextureSets;

    EvMesh* m_cubeMesh;
    EvMesh* m_sphereMesh;

    struct {
        VkImage image;
        VmaAllocation imageMemory;
        VkImageView imageView;
        VkSampler sampler;
        std::vector<VkDescriptorSet> descriptorSets;
    } m_skybox;


    void createSwapchain();
    void loadSkybox();

    void allocateCommandBuffers();
    void recordCommandBuffer(uint32_t imageIndex, const EvCamera &camera);
    void recreateSwapchain();

public:
    EvTexture* m_whiteTexture;
    EvTexture* m_normalTexture;
    TextureSet* defaultTextureSet;

    RenderSystem(EvDevice& device);
    ~RenderSystem();

    inline UIInfo& getUIInfo() { assert(overlay); return overlay->getUIInfo(); }

    void Render(const EvCamera &camera);

    void RegisterStage() override;

    Signature GetSignature() const override;
    inline EvSwapchain* getSwapchain() const { assert(swapchain); return swapchain.get(); }

    EvMesh* loadMesh(const std::string& filename, std::string* diffuseTextureFile = nullptr, std::string* normalTextureFile = nullptr);
    TextureSet* createTextureSet(EvTexture* diffuseTexture, EvTexture* normalTexture = nullptr);
    EvTexture* createTextureFromIntColor(uint32_t color);
    EvTexture* createTextureFromFile(const std::string& filename, VkFormat format);
};
