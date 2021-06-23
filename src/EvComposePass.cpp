#include "EvComposePass.h"

EvComposePass::EvComposePass(EvDevice &device, uint32_t width, uint32_t height, uint32_t nrImages,
                             const std::vector<VkImageView> &posViews, const std::vector<VkImageView> &normalViews,
                             const std::vector<VkImageView> &albedoViews,
                             const std::vector<EvFrameBufferAttachment> &depthAttachments)
                             : device(device) {
    createFramebuffer(width, height, nrImages, depthAttachments);

    // Light pipeline
    lightPipeline.lightDataBuffers.resize(nrImages);
    lightPipeline.lightDataBufferMemories.resize(nrImages);
    lightPipeline.lightDataMapped.resize(nrImages);
    for(int i=0; i<nrImages; i++) {
        device.createHostBuffer(MAX_LIGHTS * sizeof(LightComponent), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                &lightPipeline.lightDataBuffers[i], &lightPipeline.lightDataBufferMemories[i]);
        vkCheck(vmaMapMemory(device.vmaAllocator, lightPipeline.lightDataBufferMemories[i], &lightPipeline.lightDataMapped[i]));
    }

    createDescriptorSetLayout();
    allocateDescriptorSets(nrImages);
    createDescriptorSets(nrImages, posViews, normalViews, albedoViews);
    createLightPipeline();

    // Sky pipeline
    createSkyDescriptorSetLayout();
    createSkyPipeline();

    // Forward pipeline
    createForwardDescriptorSetLayout();
    createForwardPipeline();
}

EvComposePass::~EvComposePass() {
    framebuffer.destroy(device);
    // light pipeline
    for(int i=0; i<lightPipeline.lightDataBuffers.size(); i++) {
        vmaUnmapMemory(device.vmaAllocator, lightPipeline.lightDataBufferMemories[i]);
        vmaDestroyBuffer(device.vmaAllocator, lightPipeline.lightDataBuffers[i],
                         lightPipeline.lightDataBufferMemories[i]);
    }
    vkDestroySampler(device.vkDevice, lightPipeline.normalSampler, nullptr);
    vkDestroySampler(device.vkDevice, lightPipeline.posSampler, nullptr);
    vkDestroySampler(device.vkDevice, lightPipeline.albedoSampler, nullptr);
    vkDestroyShaderModule(device.vkDevice, lightPipeline.vertShader, nullptr);
    vkDestroyShaderModule(device.vkDevice, lightPipeline.fragShader, nullptr);
    vkDestroyDescriptorSetLayout(device.vkDevice, lightPipeline.descriptorSetLayout, nullptr);
    vkDestroyPipeline(device.vkDevice, lightPipeline.pipeline, nullptr);
    vkDestroyPipelineLayout(device.vkDevice, lightPipeline.pipelineLayout, nullptr);

    // sky pipeline
    vkDestroyShaderModule(device.vkDevice, skyPipeline.vertShader, nullptr);
    vkDestroyShaderModule(device.vkDevice, skyPipeline.fragShader, nullptr);
    vkDestroyDescriptorSetLayout(device.vkDevice, skyPipeline.descriptorSetLayout, nullptr);
    vkDestroyPipeline(device.vkDevice, skyPipeline.pipeline, nullptr);
    vkDestroyPipelineLayout(device.vkDevice, skyPipeline.pipelineLayout, nullptr);

    // forward pipeline
    vkDestroyShaderModule(device.vkDevice, forwardPipeline.vertShader, nullptr);
    vkDestroyShaderModule(device.vkDevice, forwardPipeline.fragShader, nullptr);
    vkDestroyDescriptorSetLayout(device.vkDevice, forwardPipeline.descriptorSetLayout, nullptr);
    vkDestroyPipeline(device.vkDevice, forwardPipeline.pipeline, nullptr);
    vkDestroyPipelineLayout(device.vkDevice, forwardPipeline.pipelineLayout, nullptr);
}

void EvComposePass::createFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages, const std::vector<EvFrameBufferAttachment> &depthAttachments) {
    assert(nrImages > 0);
    assert(depthAttachments.size() == nrImages);
    framebuffer.width = width;
    framebuffer.height = height;

    framebuffer.composeds.resize(nrImages);

    for(int i=0; i<nrImages; i++) {
        device.createAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, framebuffer.width, framebuffer.height, &framebuffer.composeds[i]);
    };

    VkAttachmentDescription defaultDescription {
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    };

    VkAttachmentDescription composedDescription = defaultDescription;
    composedDescription.format = framebuffer.composeds[0].format;
    composedDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    composedDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference composedReference {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentDescription depthDescription = defaultDescription;
    depthDescription.format = depthAttachments[0].format;
    depthDescription.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    depthDescription.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference  depthAttachmentRef {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &composedReference,
        .pDepthStencilAttachment = &depthAttachmentRef,
    };

    std::array<VkSubpassDependency, 2> dependencies {
        VkSubpassDependency {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        },
        VkSubpassDependency {
            .srcSubpass = 0,
            .dstSubpass = VK_SUBPASS_EXTERNAL,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        }
    };

    std::array<VkAttachmentDescription,2> attachmentDescriptions {composedDescription, depthDescription };
    VkRenderPassCreateInfo renderPassInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size()),
        .pAttachments = attachmentDescriptions.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = static_cast<uint32_t>(dependencies.size()),
        .pDependencies = dependencies.data(),
    };

    vkCheck(vkCreateRenderPass(device.vkDevice, &renderPassInfo, nullptr, &framebuffer.vkRenderPass));

    framebuffer.vkFrameBuffers.resize(nrImages);

    for(int i=0; i<nrImages; i++) {
        std::array<VkImageView, 2> attachments{
                framebuffer.composeds[i].view,
                depthAttachments[i].view,
        };

        VkFramebufferCreateInfo framebufferInfo{
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = framebuffer.vkRenderPass,
                .attachmentCount = static_cast<uint32_t>(attachments.size()),
                .pAttachments = attachments.data(),
                .width = framebuffer.width,
                .height = framebuffer.height,
                .layers = 1,
        };

        vkCheck(vkCreateFramebuffer(device.vkDevice, &framebufferInfo, nullptr, &framebuffer.vkFrameBuffers[i]));
    }
}

void EvComposePass::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding posBinding {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
    };

    VkDescriptorSetLayoutBinding normalBinding {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
    };

    VkDescriptorSetLayoutBinding albedoBinding {
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
    };

    VkDescriptorSetLayoutBinding lightDataBinding {
        .binding = 3,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr,
    };

    std::array<VkDescriptorSetLayoutBinding, 4> bindings = {posBinding, normalBinding, albedoBinding, lightDataBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()
    };

    vkCheck(vkCreateDescriptorSetLayout(device.vkDevice, &layoutInfo, nullptr, &lightPipeline.descriptorSetLayout));
}

void EvComposePass::allocateDescriptorSets(uint32_t nrImages) {
    lightPipeline.descriptorSets.resize(nrImages);
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts(nrImages, lightPipeline.descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = device.vkDescriptorPool,
            .descriptorSetCount = nrImages,
            .pSetLayouts = descriptorSetLayouts.data(),
    };

    vkCheck(vkAllocateDescriptorSets(device.vkDevice, &allocInfo, lightPipeline.descriptorSets.data()));

    VkSamplerCreateInfo samplerInfo = vks::initializers::samplerCreateInfo(0.0f);
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.anisotropyEnable = false;
    vkCheck(vkCreateSampler(device.vkDevice, &samplerInfo, nullptr, &lightPipeline.normalSampler));
    vkCheck(vkCreateSampler(device.vkDevice, &samplerInfo, nullptr, &lightPipeline.posSampler));
    vkCheck(vkCreateSampler(device.vkDevice, &samplerInfo, nullptr, &lightPipeline.albedoSampler));
}

void EvComposePass::createDescriptorSets(uint32_t nrImages, const std::vector<VkImageView> &posViews,
                                         const std::vector<VkImageView> &normalViews,
                                         const std::vector<VkImageView> &albedoViews) {

    assert(posViews.size() == nrImages);
    assert(albedoViews.size() == nrImages);

    for(int i=0; i<nrImages; i++) {
        VkDescriptorImageInfo posDescriptorInfo{
                .sampler = lightPipeline.posSampler,
                .imageView = posViews[i],
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VkDescriptorImageInfo normalDescriptorInfo{
                .sampler = lightPipeline.normalSampler,
                .imageView = normalViews[i],
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VkDescriptorImageInfo albedoDescriptorInfo{
                .sampler = lightPipeline.albedoSampler,
                .imageView = albedoViews[i],
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VkDescriptorBufferInfo lightDataDescriptorInfo {
            .buffer = lightPipeline.lightDataBuffers[i],
            .offset = 0,
            .range = MAX_LIGHTS * sizeof(LightComponent),
        };

        std::array<VkDescriptorImageInfo, 3> descriptors{
                posDescriptorInfo,
                normalDescriptorInfo,
                albedoDescriptorInfo,
        };

        VkWriteDescriptorSet writeDescriptorImages{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = lightPipeline.descriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = static_cast<uint32_t>(descriptors.size()),
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = descriptors.data(),
        };

        VkWriteDescriptorSet writeDescriptorBuffers{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = lightPipeline.descriptorSets[i],
                .dstBinding = 3,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &lightDataDescriptorInfo,
        };

        std::array<VkWriteDescriptorSet,2> writes { writeDescriptorImages, writeDescriptorBuffers };

        vkUpdateDescriptorSets(device.vkDevice, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

void EvComposePass::createSkyDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding cubemap {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 1,
            .pBindings = &cubemap
    };

    vkCheck(vkCreateDescriptorSetLayout(device.vkDevice, &layoutInfo, nullptr, &skyPipeline.descriptorSetLayout));
}

void EvComposePass::createSkyPipeline() {
    VkPushConstantRange pushConstantRange{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(SkyPush),
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &skyPipeline.descriptorSetLayout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushConstantRange,
    };

    vkCheck(vkCreatePipelineLayout(device.vkDevice, &pipelineLayoutInfo, nullptr, &skyPipeline.pipelineLayout));

    skyPipeline.vertShader = device.createShaderModule("assets/shaders_bin/skybox.vert.spv");
    skyPipeline.fragShader = device.createShaderModule("assets/shaders_bin/skybox.frag.spv");

    auto inputAssembly = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    auto rasterization = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    auto blendAttachmentComposed = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
    std::array<VkPipelineColorBlendAttachmentState, 1> blendAttachments { blendAttachmentComposed };
    auto colorBlend = vks::initializers::pipelineColorBlendStateCreateInfo(static_cast<uint32_t>(blendAttachments.size()), blendAttachments.data());
    auto depthStencil = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
    auto viewport = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
    auto multisample = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
    std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    auto dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages {
            vks::initializers::pipelineShaderStageCreateInfo(skyPipeline.vertShader, VK_SHADER_STAGE_VERTEX_BIT),
            vks::initializers::pipelineShaderStageCreateInfo(skyPipeline.fragShader, VK_SHADER_STAGE_FRAGMENT_BIT),
    };

    auto bindingDescriptions= Vertex::getBindingDescriptions();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    auto vertexInput = vks::initializers::pipelineVertexInputStateCreateInfo(bindingDescriptions, attributeDescriptions);

    VkGraphicsPipelineCreateInfo pipelineInfo {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = static_cast<uint32_t>(shaderStages.size()),
            .pStages = shaderStages.data(),
            .pVertexInputState = &vertexInput,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewport,
            .pRasterizationState = &rasterization,
            .pMultisampleState = &multisample,
            .pDepthStencilState = &depthStencil,
            .pColorBlendState = &colorBlend,
            .pDynamicState = &dynamicState,
            .layout = skyPipeline.pipelineLayout,
            .renderPass = framebuffer.vkRenderPass,
            .subpass = 0,
    };

    vkCheck(vkCreateGraphicsPipelines(device.vkDevice, nullptr, 1, &pipelineInfo, nullptr, &skyPipeline.pipeline));

}


void EvComposePass::createLightPipeline() {
    VkPushConstantRange pushConstant {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(ComposePush),
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &lightPipeline.descriptorSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstant,
    };

    vkCheck(vkCreatePipelineLayout(device.vkDevice, &pipelineLayoutInfo, nullptr, &lightPipeline.pipelineLayout));

    lightPipeline.vertShader = device.createShaderModule("assets/shaders_bin/compose.vert.spv");
    lightPipeline.fragShader = device.createShaderModule("assets/shaders_bin/compose.frag.spv");

    auto inputAssembly = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    auto rasterization = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
    auto blendAttachment = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_TRUE);
    blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    auto colorBlend = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachment);
    auto depthStencil = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_FALSE, VK_COMPARE_OP_GREATER_OR_EQUAL);
    auto viewport = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
    auto multisample = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
    std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    auto dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages {
            vks::initializers::pipelineShaderStageCreateInfo(lightPipeline.vertShader, VK_SHADER_STAGE_VERTEX_BIT),
            vks::initializers::pipelineShaderStageCreateInfo(lightPipeline.fragShader, VK_SHADER_STAGE_FRAGMENT_BIT),
    };

    auto bindingDescriptions= Vertex::getBindingDescriptions();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    auto vertexInput = vks::initializers::pipelineVertexInputStateCreateInfo(bindingDescriptions, attributeDescriptions);

    VkGraphicsPipelineCreateInfo pipelineInfo {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInput,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewport,
        .pRasterizationState = &rasterization,
        .pMultisampleState = &multisample,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlend,
        .pDynamicState = &dynamicState,
        .layout = lightPipeline.pipelineLayout,
        .renderPass = framebuffer.vkRenderPass,
        .subpass = 0
    };

    vkCheck(vkCreateGraphicsPipelines(device.vkDevice, nullptr, 1, &pipelineInfo, nullptr, &lightPipeline.pipeline));
}

void EvComposePass::createForwardDescriptorSetLayout() {
    VkDescriptorSetLayoutCreateInfo layoutInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 0,
    };

    vkCheck(vkCreateDescriptorSetLayout(device.vkDevice, &layoutInfo, nullptr, &forwardPipeline.descriptorSetLayout));
}

void EvComposePass::createForwardPipeline() {
    VkPushConstantRange pushConstantRange{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(ForwardPush),
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &forwardPipeline.descriptorSetLayout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushConstantRange,
    };

    vkCheck(vkCreatePipelineLayout(device.vkDevice, &pipelineLayoutInfo, nullptr, &forwardPipeline.pipelineLayout));

    forwardPipeline.vertShader = device.createShaderModule("assets/shaders_bin/forward.vert.spv");
    forwardPipeline.fragShader = device.createShaderModule("assets/shaders_bin/forward.frag.spv");

    auto inputAssembly = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    auto rasterization = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    auto blendAttachmentComposed = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
    std::array<VkPipelineColorBlendAttachmentState, 1> blendAttachments { blendAttachmentComposed };
    auto colorBlend = vks::initializers::pipelineColorBlendStateCreateInfo(static_cast<uint32_t>(blendAttachments.size()), blendAttachments.data());
    auto depthStencil = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    auto viewport = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
    auto multisample = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
    std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    auto dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages {
            vks::initializers::pipelineShaderStageCreateInfo(forwardPipeline.vertShader, VK_SHADER_STAGE_VERTEX_BIT),
            vks::initializers::pipelineShaderStageCreateInfo(forwardPipeline.fragShader, VK_SHADER_STAGE_FRAGMENT_BIT),
    };

    auto bindingDescriptions= Vertex::getBindingDescriptions();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    auto vertexInput = vks::initializers::pipelineVertexInputStateCreateInfo(bindingDescriptions, attributeDescriptions);

    VkGraphicsPipelineCreateInfo pipelineInfo {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = static_cast<uint32_t>(shaderStages.size()),
            .pStages = shaderStages.data(),
            .pVertexInputState = &vertexInput,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewport,
            .pRasterizationState = &rasterization,
            .pMultisampleState = &multisample,
            .pDepthStencilState = &depthStencil,
            .pColorBlendState = &colorBlend,
            .pDynamicState = &dynamicState,
            .layout = forwardPipeline.pipelineLayout,
            .renderPass = framebuffer.vkRenderPass,
            .subpass = 0,
    };

    vkCheck(vkCreateGraphicsPipelines(device.vkDevice, nullptr, 1, &pipelineInfo, nullptr, &forwardPipeline.pipeline));
}

void EvComposePass::recreateFramebuffer(uint32_t width, uint32_t height, uint32_t nrImages,
                                        const std::vector<VkImageView> &posViews,
                                        const std::vector<VkImageView> &normalViews,
                                        const std::vector<VkImageView> &albedoViews,
                                        const std::vector<EvFrameBufferAttachment> &depthAttachments) {
    framebuffer.destroy(device);
    createFramebuffer(width, height, nrImages, depthAttachments);
    createDescriptorSets(nrImages, posViews, normalViews, albedoViews);
}



void EvComposePass::beginPass(VkCommandBuffer commandBuffer, uint32_t imageIdx, const EvCamera& camera) const {
    assert(imageIdx >= 0 && imageIdx < framebuffer.vkFrameBuffers.size());
    std::array<VkClearValue, 2> clearValues {
        VkClearValue { .color = {0.0f, 0.0f, 0.0f, 0.0f}, },
        // should be ignore, load op is not CLEAR
        VkClearValue { .depthStencil = {0.0f, 1}, },
    };

    VkRenderPassBeginInfo renderPassInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = framebuffer.vkRenderPass,
        .framebuffer = framebuffer.vkFrameBuffers[imageIdx],
        .renderArea = {
                .offset = {0,0},
                .extent = {framebuffer.width, framebuffer.height},
        },
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data(),
    };
    VkViewport viewport {
            .x = 0.0f,
            .y = static_cast<float>(framebuffer.height),
            .width = static_cast<float>(framebuffer.width),
            .height = -static_cast<float>(framebuffer.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };
    VkRect2D scissor{{0,0}, {framebuffer.width, framebuffer.height}};

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void EvComposePass::endPass(VkCommandBuffer commandBuffer) {
    vkCmdEndRenderPass(commandBuffer);
}

void EvComposePass::bindLightPipeline(VkCommandBuffer commandBuffer, uint32_t imageIdx) const {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lightPipeline.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lightPipeline.pipelineLayout, 0, 1, &lightPipeline.descriptorSets[imageIdx], 0, nullptr);
}

void EvComposePass::bindSkyPipeline(VkCommandBuffer commandBuffer) const {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyPipeline.pipeline);
}

void EvComposePass::bindForwardPipeline(VkCommandBuffer commandBuffer) const {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, forwardPipeline.pipeline);
}


void EvComposePass::updateLights(LightComponent *data, uint nrLights, uint32_t imageIdx) {
    assert(nrLights <= MAX_LIGHTS);
    memcpy(lightPipeline.lightDataMapped[imageIdx], data, nrLights * sizeof(LightComponent));
}

