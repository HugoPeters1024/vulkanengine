#include "RenderPasses/BloomPass.h"

BloomPass::BloomPass(EvDevice &device, uint32_t width, uint32_t height, uint32_t nrImages,
                     const std::vector<EvFrameBufferAttachment> &inputs)
                     : device(device), bloomAttachments(inputs) {
    createFramebuffer(width, height, nrImages, inputs);
    createDescriptorSetLayout();
    createPipelineLayout();
    createPipeline();
    allocateDescriptorSets(nrImages);
    createDescriptorSets(nrImages, inputs);
}

BloomPass::~BloomPass() {
    framebuffer.destroy(device);
    vkDestroyShaderModule(device.vkDevice, compShader, nullptr);
    vkDestroyDescriptorSetLayout(device.vkDevice, descriptorSetLayout, nullptr);
    vkDestroyPipeline(device.vkDevice, pipeline, nullptr);
    vkDestroyPipelineLayout(device.vkDevice, pipelineLayout, nullptr);
}

void BloomPass::createFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages, const std::vector<EvFrameBufferAttachment> &inputs) {
    assert(nrImages > 0);
    assert(inputs.size() == nrImages);

    framebuffer.width = width;
    framebuffer.height = height;

    framebuffer.tmpImages.resize(nrImages);
    framebuffer.tmpImageMemory.resize(nrImages);
    framebuffer.tmpImageViews.resize(nrImages);
    auto format = inputs[0].format;
    for(int i=0; i<nrImages; i++) {
        auto imageInfo = vks::initializers::imageCreateInfo(width, height, format, VK_IMAGE_USAGE_STORAGE_BIT);
        device.createDeviceImage(imageInfo, &framebuffer.tmpImages[i], &framebuffer.tmpImageMemory[i]);
        device.transitionImageLayout(framebuffer.tmpImages[i], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 1, 1);
        auto viewInfo = vks::initializers::imageViewCreateInfo(framebuffer.tmpImages[i], format, VK_IMAGE_ASPECT_COLOR_BIT);
        vkCheck(vkCreateImageView(device.vkDevice, &viewInfo, nullptr, &framebuffer.tmpImageViews[i]));
    }
}

void BloomPass::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding inputBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
    };

    VkDescriptorSetLayoutBinding outputBinding {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
    };

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {inputBinding, outputBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    vkCheck(vkCreateDescriptorSetLayout(device.vkDevice, &layoutInfo, nullptr, &descriptorSetLayout));
}

void BloomPass::createPipelineLayout() {
    VkPushConstantRange pushConstantRange {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(Push),
    };

    VkPipelineLayoutCreateInfo  pipelineLayoutInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange,
    };

    vkCheck(vkCreatePipelineLayout(device.vkDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout));
}

void BloomPass::createPipeline() {
    compShader = device.createShaderModule("assets/shaders_bin/blur.comp.spv");

    auto compPipelineInfo = vks::initializers::computePipelineCreateInfo(pipelineLayout);
    compPipelineInfo.stage = vks::initializers::pipelineShaderStageCreateInfo(compShader, VK_SHADER_STAGE_COMPUTE_BIT);

    vkCheck(vkCreateComputePipelines(device.vkDevice, nullptr, 1, &compPipelineInfo, nullptr, &pipeline));
}

void BloomPass::allocateDescriptorSets(uint32_t nrImages) {
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts(nrImages, descriptorSetLayout);

    horzDescriptorSets.resize(nrImages);
    vertDescriptorSets.resize(nrImages);
    auto allocInfo = vks::initializers::descriptorSetAllocateInfo(device.vkDescriptorPool, descriptorSetLayouts.data(), descriptorSetLayouts.size());

    vkCheck(vkAllocateDescriptorSets(device.vkDevice, &allocInfo, horzDescriptorSets.data()));
    vkCheck(vkAllocateDescriptorSets(device.vkDevice, &allocInfo, vertDescriptorSets.data()));
}

void BloomPass::createDescriptorSets(uint32_t nrImages, const std::vector<EvFrameBufferAttachment> &inputs) {
    for(int i=0; i<nrImages; i++) {
        auto imageInputInfo = vks::initializers::descriptorImageInfo(nullptr, inputs[i].view, VK_IMAGE_LAYOUT_GENERAL);
        auto writeInput = vks::initializers::writeDescriptorSet(horzDescriptorSets[i], VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, &imageInputInfo);

        auto imageOutputInfo = vks::initializers::descriptorImageInfo(nullptr, framebuffer.tmpImageViews[i], VK_IMAGE_LAYOUT_GENERAL);
        auto writeOutput = vks::initializers::writeDescriptorSet(horzDescriptorSets[i], VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &imageOutputInfo);

        std::array<VkWriteDescriptorSet, 2> writes { writeInput, writeOutput };
        vkUpdateDescriptorSets(device.vkDevice, writes.size(), writes.data(), 0, nullptr);
    }

    for(int i=0; i<nrImages; i++) {
        auto imageInputInfo = vks::initializers::descriptorImageInfo(nullptr, framebuffer.tmpImageViews[i], VK_IMAGE_LAYOUT_GENERAL);
        auto writeInput = vks::initializers::writeDescriptorSet(vertDescriptorSets[i], VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, &imageInputInfo);

        auto imageOutputInfo = vks::initializers::descriptorImageInfo(nullptr, inputs[i].view, VK_IMAGE_LAYOUT_GENERAL);
        auto writeOutput = vks::initializers::writeDescriptorSet(vertDescriptorSets[i], VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &imageOutputInfo);

        std::array<VkWriteDescriptorSet, 2> writes { writeInput, writeOutput };
        vkUpdateDescriptorSets(device.vkDevice, writes.size(), writes.data(), 0, nullptr);
    }
}

void BloomPass::recreateFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages, const std::vector<EvFrameBufferAttachment> &inputs) {
    framebuffer.destroy(device);
    createFramebuffer(width, height, nrImages, inputs);
    createDescriptorSets(nrImages, inputs);
}

void BloomPass::run(VkCommandBuffer cmdBuffer, uint32_t imageIdx) const {
    Push push{};
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &horzDescriptorSets[imageIdx], 0, nullptr);
    push.d.x = 0;
    vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
    vkCmdDispatch(cmdBuffer, framebuffer.width / 16 + 1, framebuffer.height / 16 + 1, 1);

    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &vertDescriptorSets[imageIdx], 0, nullptr);
    push.d.x = 1;
    vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
    vkCmdDispatch(cmdBuffer, framebuffer.width / 16 + 1, framebuffer.height / 16 + 1, 1);

    auto imageBarrier = vks::initializers::imageMemoryBarrier();
    imageBarrier.image = bloomAttachments[imageIdx].image;
    imageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageBarrier.subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &imageBarrier);
}
