#pragma once

#include "EvDevice.h"
#include "EvMesh.h"
#include "EvCamera.h"
#include "EvSwapchain.h"
#include "ShaderTypes.h"
#include "EvTexture.h"
#include "EvOverlay.h"
#include "EvGPass.h"
#include "EvComposePass.h"
#include "EvForwardPass.h"
#include "EvPostPass.h"

struct TextureSet : NoCopy {
    std::vector<VkDescriptorSet> descriptorSets;
};

struct ModelComponent {
    EvMesh* mesh = nullptr;
    TextureSet* textureSet = nullptr;
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
    glm::vec2 textureScale{1.0f, 1.0f};
    glm::mat4 transform{1.0f};
};

struct LightComponent {
    glm::vec3 position;
    glm::vec3 color;
};


class RenderSystem : public System
{
    class LightSystem : public System
    {
    public:
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

    std::unique_ptr<EvGPass> gPass;
    std::unique_ptr<EvComposePass> composePass;
    std::unique_ptr<EvForwardPass> forwardPass;
    std::unique_ptr<EvPostPass> postPass;
    std::unique_ptr<EvOverlay> overlay;

    std::vector<std::unique_ptr<EvMesh>> createdMeshes;
    std::vector<std::unique_ptr<EvTexture>> createdTextures;
    std::vector<std::unique_ptr<TextureSet>> createdTextureSets;

    EvMesh* m_cubeMesh;
    EvMesh* m_sphereMesh;


    void createSwapchain();

    void recordGBufferCommands(VkCommandBuffer commandBuffer, uint32_t imageIdx, const EvCamera &camera);

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
