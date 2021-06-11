#include "RenderSystem.h"

RenderSystem::RenderSystem(EvDevice &device) : device(device) {
    createSwapchain();

    allocateCommandBuffers();

    uint32_t nrImages = swapchain->vkImages.size();
    gPass = std::make_unique<EvGPass>(device, swapchain->extent.width, swapchain->extent.height, nrImages);
    composePass = std::make_unique<EvComposePass>(device, swapchain->extent.width, swapchain->extent.height, nrImages,
                                                  gPass->getPosViews(),
                                                  gPass->getAlbedoViews());
    postPass = std::make_unique<EvPostPass>(device, swapchain->extent.width, swapchain->extent.height, nrImages,
                                            swapchain->surfaceFormat.format, swapchain->vkImageViews,
                                            composePass->getComposedViews());
    overlay = std::make_unique<EvOverlay>(device, postPass->getRenderPass(), nrImages);

    whiteTexture = createTextureFromIntColor(0xffffff);
    blueTexture = createTextureFromIntColor((0x0000ff));
    defaultTextureSet = createTextureSet(whiteTexture, blueTexture);
}

RenderSystem::~RenderSystem() {
}


void RenderSystem::createSwapchain() {
    if (swapchain == nullptr) {
        swapchain = std::make_unique<EvSwapchain>(device);
    } else {
        swapchain = std::make_unique<EvSwapchain>(device, std::move(swapchain));
    }
}

void RenderSystem::recordGBufferCommands(VkCommandBuffer commandBuffer, uint32_t imageIdx, const EvCamera &camera) {
    gPass->startPass(commandBuffer, imageIdx);

    PushConstant push{
        .camera = camera.getVPMatrix(device.window.getAspectRatio()),
    };

    for (const auto& entity : m_entities) {
        auto& modelComp = m_coordinator->GetComponent<ModelComponent>(entity);
        assert(modelComp.mesh);
        auto scaleMatrix = glm::scale(glm::mat4(1.0f), modelComp.scale);
        push.mvp = modelComp.transform * scaleMatrix;
        push.texScale = glm::vec4(modelComp.textureScale,0,0);
        vkCmdPushConstants(
                commandBuffer,
                gPass->getPipelineLayout(),
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(push),
                &push);
        auto textureSet = modelComp.textureSet ? modelComp.textureSet : defaultTextureSet;
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gPass->getPipelineLayout(), 0, 1, &textureSet->descriptorSets[imageIdx], 0, nullptr);
        modelComp.mesh->bind(commandBuffer);
        modelComp.mesh->draw(commandBuffer);
    }

    gPass->endPass(commandBuffer);
}

void RenderSystem::allocateCommandBuffers() {
    commandBuffers.resize(swapchain->vkImages.size());

    VkCommandBufferAllocateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = device.vkCommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<uint32_t>(commandBuffers.size()),
    };

    vkCheck(vkAllocateCommandBuffers(device.vkDevice, &createInfo, commandBuffers.data()));
}

void RenderSystem::recordCommandBuffer(uint32_t imageIndex, const EvCamera &camera) {
    const VkCommandBuffer& commandBuffer = commandBuffers[imageIndex];
    vkCheck(vkResetCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));

    VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkCheck(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    recordGBufferCommands(commandBuffer, imageIndex, camera);
    composePass->render(commandBuffer, imageIndex, camera);
    postPass->beginPass(commandBuffer, imageIndex);
    overlay->Draw(commandBuffer);
    postPass->endPass(commandBuffer);


    vkCheck(vkEndCommandBuffer(commandBuffer));
}

void RenderSystem::recreateSwapchain() {
    vkCheck(vkDeviceWaitIdle(device.vkDevice));
    device.window.waitForEvent();
    createSwapchain();
    uint32_t nrImages = swapchain->vkImages.size();
    gPass->recreateFramebuffer(swapchain->extent.width, swapchain->extent.height, nrImages);
    composePass->recreateFramebuffer(swapchain->extent.width, swapchain->extent.height, nrImages, gPass->getPosViews(), gPass->getAlbedoViews());
    postPass->recreateFramebuffer(swapchain->extent.width, swapchain->extent.height, nrImages, composePass->getComposedViews(), swapchain->surfaceFormat.format, swapchain->vkImageViews);
}

void RenderSystem::Render(const EvCamera &camera) {
    overlay->NewFrame();
    uint32_t imageIndex;
    VkResult acquireImageResult = swapchain->acquireNextSwapchainImage(&imageIndex);

    if (acquireImageResult == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    } else if (acquireImageResult != VK_SUCCESS && acquireImageResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    swapchain->waitForImageReady(imageIndex);


    recordCommandBuffer(imageIndex, camera);

    VkResult presentResult = swapchain->presentCommandBuffer(commandBuffers[imageIndex], imageIndex);

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || device.window.wasResized) {
        device.window.wasResized = false;
        recreateSwapchain();
        return;
    } else if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image");
    }
}

Signature RenderSystem::GetSignature() const {
    Signature signature{};
    signature.set(m_coordinator->GetComponentType<ModelComponent>());
    return signature;
}

EvMesh * RenderSystem::loadMesh(const std::string &filename, std::string *diffuseTextureFile, std::string *normalTextureFile) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    BoundingBox bb{};
    EvMesh::loadMesh(filename, &vertices, &indices, &bb, diffuseTextureFile, normalTextureFile);

    createdMeshes.push_back(std::make_unique<EvMesh>(device, vertices, indices, bb));
    return createdMeshes.back().get();
}

TextureSet *RenderSystem::createTextureSet(EvTexture *diffuseTexture, EvTexture *normalTexture) {
    assert(diffuseTexture);
    createdTextureSets.push_back(std::make_unique<TextureSet>());
    auto tset = createdTextureSets.back().get();
    if (!normalTexture) normalTexture = blueTexture;
    tset->descriptorSets.resize(swapchain->vkImages.size());

    std::vector<VkDescriptorSetLayout> layouts(tset->descriptorSets.size(), gPass->getDescriptorSetLayout());
    VkDescriptorSetAllocateInfo allocInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = device.vkDescriptorPool,
            .descriptorSetCount = static_cast<uint32_t>(tset->descriptorSets.size()),
            .pSetLayouts = layouts.data(),
    };

    vkCheck(vkAllocateDescriptorSets(device.vkDevice, &allocInfo, tset->descriptorSets.data()));

    auto diffuseDescriptor = diffuseTexture->getDescriptorInfo();
    auto normalDescriptor = normalTexture->getDescriptorInfo();

    for(int i=0; i<swapchain->vkImages.size(); i++) {
        std::array<VkWriteDescriptorSet,1> descriptorWrites {
                VkWriteDescriptorSet {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = tset->descriptorSets[i],
                        .dstBinding = 0,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .pImageInfo = &diffuseDescriptor,
                },
        };

        vkUpdateDescriptorSets(
                device.vkDevice,
                static_cast<uint32_t>(descriptorWrites.size()),
                descriptorWrites.data(),
                0, nullptr);
    }
    return tset;
}

EvTexture* RenderSystem::createTextureFromIntColor(uint32_t color) {
    createdTextures.push_back(EvTexture::fromIntColor(device, color));
    return createdTextures.back().get();
}

EvTexture *RenderSystem::createTextureFromFile(const std::string &filename) {
    printf("Loading texture %s\n", filename.c_str());
    createdTextures.push_back(EvTexture::fromFile(device, filename));
    return createdTextures.back().get();
}



