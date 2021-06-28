#include "RenderSystem.h"

RenderSystem::RenderSystem(EvDevice &device) : device(device) {

    createSwapchain();

    allocateCommandBuffers();

    uint32_t width = swapchain->extent.width;
    uint32_t height = swapchain->extent.height;
    uint32_t nrImages = swapchain->vkImages.size();
    depthPass = std::make_unique<DepthPass>(device, width, height, nrImages);
    forwardPass = std::make_unique<ForwardPass>(device, width, height, nrImages, depthPass->getFramebuffer().depths);
    bloomPass = std::make_unique<BloomPass>(device, width, height, nrImages, forwardPass->getFramebuffer().blooms);
    postPass = std::make_unique<PostPass>(device, width, height, nrImages,
                                            swapchain->surfaceFormat.format, swapchain->vkImageViews,
                                            forwardPass->getFramebuffer().colors, forwardPass->getFramebuffer().blooms);
    overlay = std::make_unique<EvOverlay>(device, postPass->getRenderPass(), nrImages);

    m_whiteTexture = createTextureFromIntColor(0xffffff);
    m_normalTexture = createTextureFromIntColor((makeRGBA(128, 128, 255, 0)));
    defaultTextureSet = createTextureSet(m_whiteTexture, m_normalTexture);
    m_cubeMesh = loadMesh("./assets/models/cube.obj");
    m_sphereMesh = loadMesh("./assets/models/sphere_low.obj");
    loadSkybox();
}

RenderSystem::~RenderSystem() {
    vmaDestroyImage(device.vmaAllocator, m_skybox.image, m_skybox.imageMemory);
    vkDestroyImageView(device.vkDevice, m_skybox.imageView, nullptr);
    vkDestroySampler(device.vkDevice, m_skybox.sampler, nullptr);
}


void RenderSystem::createSwapchain() {
    if (swapchain == nullptr) {
        swapchain = std::make_unique<EvSwapchain>(device);
    } else {
        swapchain = std::make_unique<EvSwapchain>(device, std::move(swapchain));
    }
}

void RenderSystem::loadSkybox() {
    uchar* data[6];
    int32_t width, height;
    EvTexture::loadFile("./assets/textures/skybox/right.jpg", &data[0], &width, &height);
    EvTexture::loadFile("./assets/textures/skybox/left.jpg", &data[1], &width, &height);
    EvTexture::loadFile("./assets/textures/skybox/top.jpg", &data[2], &width, &height);
    EvTexture::loadFile("./assets/textures/skybox/bottom.jpg", &data[3], &width, &height);
    EvTexture::loadFile("./assets/textures/skybox/front.jpg", &data[4], &width, &height);
    EvTexture::loadFile("./assets/textures/skybox/back.jpg", &data[5], &width, &height);

    device.createDeviceCubemap(width, height, data, &m_skybox.image, &m_skybox.imageMemory, &m_skybox.imageView);

    // Create sampler
    auto samplerInfo = vks::initializers::samplerCreateInfo(device.vkPhysicalDeviceProperties.limits.maxSamplerAnisotropy);
    vkCheck(vkCreateSampler(device.vkDevice, &samplerInfo, nullptr, &m_skybox.sampler));

    // Allocate the descriptor sets
    //uint32_t nrImages = swapchain->vkImages.size();
    //m_skybox.descriptorSets.resize(nrImages);
    //std::vector<VkDescriptorSetLayout> descriptorSetLayouts(nrImages, composePass->skyPipeline.descriptorSetLayout);
    //VkDescriptorSetAllocateInfo allocInfo {
    //        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    //        .descriptorPool = device.vkDescriptorPool,
    //        .descriptorSetCount = nrImages,
    //        .pSetLayouts = descriptorSetLayouts.data(),
    //};

    //vkCheck(vkAllocateDescriptorSets(device.vkDevice, &allocInfo, m_skybox.descriptorSets.data()));

    //// Write the descriptor sets
    //for(int i=0; i<nrImages; i++) {
    //    VkDescriptorImageInfo skyboxDescriptorInfo {
    //            .sampler = m_skybox.sampler,
    //            .imageView = m_skybox.imageView,
    //            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    //    };

    //    VkWriteDescriptorSet writeDescriptorImage{
    //            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    //            .dstSet = m_skybox.descriptorSets[i],
    //            .dstBinding = 0,
    //            .dstArrayElement = 0,
    //            .descriptorCount = 1,
    //            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    //            .pImageInfo = &skyboxDescriptorInfo
    //    };

    //    vkUpdateDescriptorSets(device.vkDevice, 1, &writeDescriptorImage, 0, nullptr);
    //}
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
    const VkCommandBuffer &commandBuffer = commandBuffers[imageIndex];
    vkCheck(vkResetCommandBuffer(commandBuffer, 0));

    VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkCheck(vkBeginCommandBuffer(commandBuffer, &beginInfo));


    {
        depthPass->startPass(commandBuffer, imageIndex);
        PushConstant push{
                .camera = camera.getVPMatrix(device.window.getAspectRatio()),
        };

        for (const auto &entity : m_entities) {
            auto &modelComp = m_coordinator->GetComponent<ModelComponent>(entity);
            assert(modelComp.mesh);
            auto scaleMatrix = glm::scale(glm::mat4(1.0f), modelComp.scale);
            push.mvp = modelComp.transform * scaleMatrix;
            vkCmdPushConstants(
                    commandBuffer,
                    depthPass->getPipelineLayout(),
                    VK_SHADER_STAGE_VERTEX_BIT,
                    0,
                    sizeof(push),
                    &push);
            auto textureSet = modelComp.textureSet ? modelComp.textureSet : defaultTextureSet;
            modelComp.mesh->bind(commandBuffer);
            modelComp.mesh->draw(commandBuffer);
        }
        depthPass->endPass(commandBuffer);
    }
    {
        forwardPass->startPass(commandBuffer, imageIndex);
        PushConstant push{
                .camera = camera.getVPMatrix(device.window.getAspectRatio()),
                .camPos = camera.position,
        };

        for (const auto &entity : m_entities) {
            auto &modelComp = m_coordinator->GetComponent<ModelComponent>(entity);
            assert(modelComp.mesh);
            auto scaleMatrix = glm::scale(glm::mat4(1.0f), modelComp.scale);
            push.mvp = modelComp.transform * scaleMatrix;
            vkCmdPushConstants(
                    commandBuffer,
                    forwardPass->getPipelineLayout(),
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof(push),
                    &push);
            auto textureSet = modelComp.textureSet ? modelComp.textureSet : defaultTextureSet;
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, forwardPass->getPipelineLayout(), 0, 1,
                                    &textureSet->descriptorSets[imageIndex], 0, nullptr);
            modelComp.mesh->bind(commandBuffer);
            modelComp.mesh->draw(commandBuffer);
        }
        forwardPass->endPass(commandBuffer);
    }
    {
        bloomPass->run(commandBuffer, imageIndex);
    }
    {
        postPass->beginPass(commandBuffer, imageIndex);
        overlay->Draw(commandBuffer);
        postPass->endPass(commandBuffer);
    }
    vkCheck(vkEndCommandBuffer(commandBuffer));
}

void RenderSystem::recreateSwapchain() {
    vkCheck(vkDeviceWaitIdle(device.vkDevice));
    device.window.waitForEvent();
    createSwapchain();
    uint32_t width = swapchain->extent.width;
    uint32_t height = swapchain->extent.height;
    uint32_t nrImages = swapchain->vkImages.size();
    depthPass->recreateFramebuffer(width, height, nrImages);
    forwardPass->recreateFramebuffer(width, height, nrImages, depthPass->getFramebuffer().depths);
    bloomPass->recreateFramebuffer(width, height, nrImages, forwardPass->getFramebuffer().blooms);
    postPass->recreateFramebuffer(width, height, nrImages, forwardPass->getFramebuffer().colors, forwardPass->getFramebuffer().blooms,
                                  swapchain->surfaceFormat.format, swapchain->vkImageViews);
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

    const uint nrLights = lightSubSystem->m_entities.size();
    const UIInfo& uiInfo = getUIInfo();
    forwardPass->setLightProperties(imageIndex, 1.0f, uiInfo.linear, uiInfo.quadratic);
    forwardPass->updateLights(m_coordinator->GetComponentArrayData<LightComponent>(), nrLights, imageIndex);
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
    if (!normalTexture) normalTexture = m_normalTexture;
    tset->descriptorSets.resize(swapchain->vkImages.size());

    std::vector<VkDescriptorSetLayout> layouts(tset->descriptorSets.size(), forwardPass->getDescriptorSetLayout());
    VkDescriptorSetAllocateInfo allocInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = device.vkDescriptorPool,
            .descriptorSetCount = static_cast<uint32_t>(tset->descriptorSets.size()),
            .pSetLayouts = layouts.data(),
    };

    vkCheck(vkAllocateDescriptorSets(device.vkDevice, &allocInfo, tset->descriptorSets.data()));

    auto albedoDescriptor = diffuseTexture->getDescriptorInfo();
    auto normalDescriptor = normalTexture->getDescriptorInfo();

    for(int i=0; i<swapchain->vkImages.size(); i++) {
        std::array<VkWriteDescriptorSet,2> descriptorWrites {
                VkWriteDescriptorSet {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = tset->descriptorSets[i],
                        .dstBinding = 0,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .pImageInfo = &albedoDescriptor,
                },
                VkWriteDescriptorSet {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = tset->descriptorSets[i],
                        .dstBinding = 1,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .pImageInfo = &normalDescriptor,
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

EvTexture *RenderSystem::createTextureFromFile(const std::string &filename, VkFormat format) {
    printf("Loading texture %s\n", filename.c_str());
    createdTextures.push_back(EvTexture::fromFile(device, filename, format));
    return createdTextures.back().get();
}

void RenderSystem::RegisterStage() {
    m_coordinator->RegisterComponent<ModelComponent>();
    m_coordinator->RegisterComponent<LightComponent>();
    lightSubSystem = m_coordinator->RegisterSystem<LightSystem>();
}


