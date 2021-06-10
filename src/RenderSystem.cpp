#include "RenderSystem.h"

RenderSystem::RenderSystem(EvDevice &device) : device(device) {
    createSwapchain();

    allocateCommandBuffers();

    uint32_t nrImages = swapchain->vkImages.size();
    gPass = std::make_unique<EvGPass>(device, swapchain->extent.width, swapchain->extent.height, nrImages);
    composePass = std::make_unique<EvComposePass>(device, swapchain->extent.width, swapchain->extent.height, nrImages,
                                                  gPass->getNormalViews(),
                                                  gPass->getPosViews(),
                                                  gPass->getAlbedoViews());
    postPass = std::make_unique<EvPostPass>(device, swapchain->extent.width, swapchain->extent.height, nrImages,
                                            swapchain->surfaceFormat.format, swapchain->vkImageViews,
                                            composePass->getComposedViews());
    overlay = std::make_unique<EvOverlay>(device, postPass->getRenderPass(), nrImages);
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
        auto scaleMatrix = glm::scale(glm::mat4(1.0f), modelComp.scale);
        push.mvp = modelComp.transform * scaleMatrix;
        vkCmdPushConstants(
                commandBuffer,
                gPass->getPipelineLayout(),
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(push),
                &push);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gPass->getPipelineLayout(), 0, 1, &modelComp.model->vkDescriptorSets[imageIdx], 0, nullptr);
        modelComp.model->bind(commandBuffer);
        modelComp.model->draw(commandBuffer);
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
    composePass->recreateFramebuffer(swapchain->extent.width, swapchain->extent.height, nrImages, gPass->getNormalViews(), gPass->getPosViews(), gPass->getAlbedoViews());
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

std::unique_ptr<EvModel> RenderSystem::createModel(const std::string &filename, const EvTexture *texture) {
    auto ret = std::make_unique<EvModel>(device, filename, texture);
    ret->vkDescriptorSets.resize(swapchain->vkImages.size());

    std::vector<VkDescriptorSetLayout> layouts(ret->vkDescriptorSets.size(), gPass->getDescriptorSetLayout());
    VkDescriptorSetAllocateInfo allocInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = device.vkDescriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(ret->vkDescriptorSets.size()),
        .pSetLayouts = layouts.data(),
    };

    vkCheck(vkAllocateDescriptorSets(device.vkDevice, &allocInfo, ret->vkDescriptorSets.data()));

    auto imageDescriptor = texture->getDescriptorInfo();

    for(int i=0; i<swapchain->vkImages.size(); i++) {
        std::array<VkWriteDescriptorSet,1> descriptorWrites {
            VkWriteDescriptorSet {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = ret->vkDescriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &imageDescriptor,
            },
        };

        vkUpdateDescriptorSets(
            device.vkDevice,
            static_cast<uint32_t>(descriptorWrites.size()),
            descriptorWrites.data(),
            0, nullptr);
    }

    return ret;
}



