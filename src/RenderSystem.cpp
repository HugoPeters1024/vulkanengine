#include "RenderSystem.h"

RenderSystem::RenderSystem(EvDevice &device) : device(device) {
    createShaderModules();
    createSwapchain();
    createDescriptorSetLayout();
    createPipelineLayout();
    createPipeline();

    createGBuffer();
    createGBufferDescriptorSetLayout();
    createGBufferPipeline();
    allocateGBufferCommandBuffer();

    createComposeBuffer();
    createComposeDescriptorSetLayout();
    allocateComposeDescriptorSet();
    createComposeDescriptorSet();
    createComposePipeline();
    allocateComposeCommandBuffer();

    allocateCommandBuffers();
}

RenderSystem::~RenderSystem() {
    printf("Destroying the gpass\n");
    vkDestroyShaderModule(device.vkDevice, gpass.vertShader, nullptr);
    vkDestroyShaderModule(device.vkDevice, gpass.fragShader, nullptr);
    gpass.framebuffer.destroy(device);
    vkDestroyDescriptorSetLayout(device.vkDevice, gpass.descriptorSetLayout, nullptr);
    vkDestroyPipeline(device.vkDevice, gpass.pipeline, nullptr);
    vkDestroyPipelineLayout(device.vkDevice, gpass.pipelineLayout, nullptr);

    printf("Destroying the compose pass\n");
    vkDestroySampler(device.vkDevice, composepass.normalSampler, nullptr);
    vkDestroyShaderModule(device.vkDevice, composepass.vertShader, nullptr);
    vkDestroyShaderModule(device.vkDevice, composepass.fragShader, nullptr);
    composepass.framebuffer.destroy(device);
    vkDestroyDescriptorSetLayout(device.vkDevice, composepass.descriptorSetLayout, nullptr);
    vkDestroyPipeline(device.vkDevice, composepass.pipeline, nullptr);
    vkDestroyPipelineLayout(device.vkDevice, composepass.pipelineLayout, nullptr);


    printf("Destroying descriptor set layout\n");
    vkDestroyDescriptorSetLayout(device.vkDevice, vkDescriptorSetLayout, nullptr);

    printf("Destroying the shader modules\n");
    vkDestroyShaderModule(device.vkDevice, vertShaderModule, nullptr);
    vkDestroyShaderModule(device.vkDevice, fragShaderModule, nullptr);

    printf("Destroying pipeline layout\n");
    vkDestroyPipelineLayout(device.vkDevice, vkPipelineLayout, nullptr);
}


void RenderSystem::createShaderModules() {
    vertShaderModule = device.createShaderModule("assets/shaders_bin/shader.vert.spv");
    fragShaderModule = device.createShaderModule("assets/shaders_bin/shader.frag.spv");
}

void RenderSystem::createSwapchain() {
    if (swapchain == nullptr) {
        swapchain = std::make_unique<EvSwapchain>(device);
    } else {
        swapchain = std::make_unique<EvSwapchain>(device, std::move(swapchain));
    }
}

void RenderSystem::createPipelineLayout() {
    VkPushConstantRange pushConstantRange{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(PushConstant),
    };

    VkPipelineLayoutCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &vkDescriptorSetLayout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushConstantRange,
    };

    vkCheck(vkCreatePipelineLayout(device.vkDevice, &createInfo, nullptr, &vkPipelineLayout));
}

void RenderSystem::createPipeline() {

    EvRastPipelineInfo pipelineInfo;
    EvRastPipeline::defaultRastPipelineInfo(vertShaderModule, fragShaderModule, device.msaaSamples, &pipelineInfo);
    pipelineInfo.renderPass = swapchain->vkRenderPass;
    pipelineInfo.pipelineLayout = vkPipelineLayout;
    pipelineInfo.descriptorSetLayout = vkDescriptorSetLayout;
    pipelineInfo.swapchainImages = static_cast<uint32>(swapchain->vkImages.size());

    rastPipeline = std::make_unique<EvRastPipeline>(device, pipelineInfo);
}

void RenderSystem::createGBuffer() {
    gpass.framebuffer.width = swapchain->extent.width;
    gpass.framebuffer.height = swapchain->extent.height;
    device.createAttachment(VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, gpass.framebuffer.width, gpass.framebuffer.height, &gpass.framebuffer.normal);
    device.createAttachment(device.findDepthFormat(), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, gpass.framebuffer.width, gpass.framebuffer.height, &gpass.framebuffer.depth);

    VkAttachmentDescription defaultDescription {
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    };

    VkAttachmentDescription albedoDescription = defaultDescription;
    albedoDescription.format = gpass.framebuffer.normal.format;
    albedoDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentDescription depthDescription = defaultDescription;
    depthDescription.format = gpass.framebuffer.depth.format;
    depthDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkAttachmentDescription, 2> attachmentDescriptions{
        albedoDescription,
        depthDescription,
    };

    std::array<VkAttachmentReference, 1> colorReferences {
        VkAttachmentReference {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        }
    };

    VkAttachmentReference depthAttachmentRef {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = static_cast<uint32_t>(colorReferences.size()),
        .pColorAttachments = colorReferences.data(),
        .pDepthStencilAttachment = &depthAttachmentRef,
    };

    std::array<VkSubpassDependency, 2> dependencies {
        VkSubpassDependency {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
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

    VkRenderPassCreateInfo renderpassInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size()),
        .pAttachments = attachmentDescriptions.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = static_cast<uint32_t>(dependencies.size()),
        .pDependencies = dependencies.data(),
    };

    vkCheck(vkCreateRenderPass(device.vkDevice, &renderpassInfo, nullptr, &gpass.framebuffer.vkRenderPass));

    std::array<VkImageView, 2> attachments{
        gpass.framebuffer.normal.view,
        gpass.framebuffer.depth.view,
    };

    VkFramebufferCreateInfo framebufferInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = gpass.framebuffer.vkRenderPass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = gpass.framebuffer.width,
        .height= gpass.framebuffer.height,
        .layers = 1,
    };

    vkCheck(vkCreateFramebuffer(device.vkDevice, &framebufferInfo, nullptr, &gpass.framebuffer.vkFrameBuffer));
}

void RenderSystem::createGBufferDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding textureBinding {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
    };

    std::array<VkDescriptorSetLayoutBinding, 1> bindings = {textureBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data(),
    };

    vkCheck(vkCreateDescriptorSetLayout(device.vkDevice, &layoutInfo, nullptr, &gpass.descriptorSetLayout));
}

void RenderSystem::createGBufferPipeline() {
    VkPushConstantRange pushConstantRange{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(PushConstant),
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &gpass.descriptorSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange,
    };

    vkCheck(vkCreatePipelineLayout(device.vkDevice, &pipelineLayoutInfo, nullptr, &gpass.pipelineLayout));

    gpass.vertShader = device.createShaderModule("assets/shaders_bin/gpass.vert.spv");
    gpass.fragShader = device.createShaderModule("assets/shaders_bin/gpass.frag.spv");

    auto inputAssembly = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    auto rasterization = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    auto blendAttachment = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
    auto colorBlend = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachment);
    auto depthStencil = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    auto viewport = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
    auto multisample = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
    std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    auto dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages {
        vks::initializers::pipelineShaderStageCreateInfo(gpass.vertShader, VK_SHADER_STAGE_VERTEX_BIT),
        vks::initializers::pipelineShaderStageCreateInfo(gpass.fragShader, VK_SHADER_STAGE_FRAGMENT_BIT),
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
        .layout = gpass.pipelineLayout,
        .renderPass = gpass.framebuffer.vkRenderPass,
        .subpass = 0,
    };

    vkCheck(vkCreateGraphicsPipelines(device.vkDevice, nullptr, 1, &pipelineInfo, nullptr, &gpass.pipeline));
}

void RenderSystem::allocateGBufferCommandBuffer() {
    VkCommandBufferAllocateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = device.vkCommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
    };

    vkCheck(vkAllocateCommandBuffers(device.vkDevice, &createInfo, &gpass.commandBuffer));
}

void RenderSystem::recordGBufferCommands(const EvCamera &camera) {
    const VkCommandBuffer& commandBuffer = gpass.commandBuffer;
    vkCheck(vkResetCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));

    VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkCheck(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    std::array<VkClearValue, 2> clearValues {};
    clearValues[0] = {
            .color = {0.0f, 0.0f, 0.0f, 0.0f},
    };
    clearValues[1] = {
            .depthStencil = {1.0f, 0},
    };

    VkRenderPassBeginInfo renderPassInfo {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = gpass.framebuffer.vkRenderPass,
            .framebuffer = gpass.framebuffer.vkFrameBuffer,
            .renderArea = {
                    .offset = {0,0},
                    .extent = {gpass.framebuffer.width, gpass.framebuffer.height},
            },
            .clearValueCount = static_cast<uint32_t>(clearValues.size()),
            .pClearValues = clearValues.data(),
    };
    VkViewport viewport {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(gpass.framebuffer.width),
            .height = static_cast<float>(gpass.framebuffer.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };
    VkRect2D scissor{{0,0}, {gpass.framebuffer.width, gpass.framebuffer.height}};

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gpass.pipeline);

    PushConstant push{
            .camera = camera.getVPMatrix(device.window.getAspectRatio()),
    };

    for (const auto& entity : m_entities) {
        auto& modelComp = m_coordinator->GetComponent<ModelComponent>(entity);
        auto scaleMatrix = glm::scale(glm::mat4(1.0f), modelComp.scale);
        push.mvp = modelComp.transform * scaleMatrix;
        vkCmdPushConstants(
                commandBuffer,
                vkPipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(push),
                &push);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipelineLayout, 0, 1, &modelComp.model->vkDescriptorSets[0], 0, nullptr);
        modelComp.model->bind(commandBuffer);
        modelComp.model->draw(commandBuffer);
    }

    vkCmdEndRenderPass(commandBuffer);
    vkCheck(vkEndCommandBuffer(commandBuffer));
}

void RenderSystem::createComposeDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding normalBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };

    std::array<VkDescriptorSetLayoutBinding, 1> bindings = {normalBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };

    vkCheck(vkCreateDescriptorSetLayout(device.vkDevice, &layoutInfo, nullptr, &composepass.descriptorSetLayout));
}

void RenderSystem::allocateComposeDescriptorSet() {
    VkDescriptorSetAllocateInfo allocInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = device.vkDescriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &composepass.descriptorSetLayout,
    };

    vkCheck(vkAllocateDescriptorSets(device.vkDevice, &allocInfo, &composepass.descriptorSet));

    VkSamplerCreateInfo samplerInfo = vks::initializers::samplerCreateInfo(device.vkPhysicalDeviceProperties.limits.maxSamplerAnisotropy);
    vkCheck(vkCreateSampler(device.vkDevice, &samplerInfo, nullptr, &composepass.normalSampler));
}

void RenderSystem::createComposeDescriptorSet() {

    VkDescriptorImageInfo normalDescriptorInfo {
        .sampler = composepass.normalSampler,
        .imageView = gpass.framebuffer.normal.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet writeNormalDescriptor {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = composepass.descriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &normalDescriptorInfo,
    };

    std::array<VkWriteDescriptorSet, 1> descriptorWrites {
        writeNormalDescriptor,
    };

    vkUpdateDescriptorSets(device.vkDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void RenderSystem::createComposeBuffer() {
    composepass.framebuffer.width = swapchain->extent.width;
    composepass.framebuffer.height = swapchain->extent.height;

    device.createAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, composepass.framebuffer.width, composepass.framebuffer.height, &composepass.framebuffer.composed);

    VkAttachmentDescription composedDescription {
        .format = composepass.framebuffer.composed.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkAttachmentReference composedReference {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    std::array<VkAttachmentDescription, 1> attachmentDescriptions {composedDescription };
    std::array<VkAttachmentReference, 1> colorReferences { composedReference };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = static_cast<uint32_t>(colorReferences.size()),
        .pColorAttachments = colorReferences.data(),
        .pDepthStencilAttachment = nullptr,
    };

    std::array<VkSubpassDependency, 2> dependencies {
            VkSubpassDependency {
                    .srcSubpass = VK_SUBPASS_EXTERNAL,
                    .dstSubpass = 0,
                    .srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
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

    VkRenderPassCreateInfo renderPassInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size()),
        .pAttachments = attachmentDescriptions.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = static_cast<uint32_t>(dependencies.size()),
        .pDependencies = dependencies.data(),
    };

    vkCheck(vkCreateRenderPass(device.vkDevice, &renderPassInfo, nullptr, &composepass.framebuffer.vkRenderPass));

    std::array<VkImageView, 1> attachments {
        composepass.framebuffer.composed.view,
    };

    VkFramebufferCreateInfo framebufferInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = composepass.framebuffer.vkRenderPass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = composepass.framebuffer.width,
        .height = composepass.framebuffer.height,
        .layers = 1,
    };

    vkCheck(vkCreateFramebuffer(device.vkDevice, &framebufferInfo, nullptr, &composepass.framebuffer.vkFrameBuffer));
}

void RenderSystem::createComposePipeline() {
    VkPushConstantRange pushConstantRange{
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(glm::vec2),
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &composepass.descriptorSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange,
    };

    vkCheck(vkCreatePipelineLayout(device.vkDevice, &pipelineLayoutInfo, nullptr, &composepass.pipelineLayout));

    composepass.vertShader = device.createShaderModule("assets/shaders_bin/compose.vert.spv");
    composepass.fragShader = device.createShaderModule("assets/shaders_bin/compose.frag.spv");

    auto inputAssembly = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    auto rasterization = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
    auto blendAttachment = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
    auto colorBlend = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachment);
    auto depthStencil = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);
    auto viewport = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
    auto multisample = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
    std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    auto dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages {
        vks::initializers::pipelineShaderStageCreateInfo(composepass.vertShader, VK_SHADER_STAGE_VERTEX_BIT),
        vks::initializers::pipelineShaderStageCreateInfo(composepass.fragShader, VK_SHADER_STAGE_FRAGMENT_BIT),
    };
    auto vertexInput = vks::initializers::pipelineVertexInputStateCreateInfo();

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
        .layout = composepass.pipelineLayout,
        .renderPass = composepass.framebuffer.vkRenderPass,
        .subpass = 0
    };

    vkCheck(vkCreateGraphicsPipelines(device.vkDevice, nullptr, 1, &pipelineInfo, nullptr, &composepass.pipeline));
}

void RenderSystem::allocateComposeCommandBuffer() {
    VkCommandBufferAllocateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = device.vkCommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
    };

    vkCheck(vkAllocateCommandBuffers(device.vkDevice, &createInfo, &composepass.commandBuffer));
}

void RenderSystem::recordComposeCommands() {
    const VkCommandBuffer& commandBuffer = composepass.commandBuffer;
    vkCheck(vkResetCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));

    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkCheck(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    std::array<VkClearValue, 1> clearValues {};
    clearValues[0] = {
            .color = {0.0f, 0.0f, 0.0f, 0.0f},
    };

    VkRenderPassBeginInfo renderPassInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = composepass.framebuffer.vkRenderPass,
        .framebuffer = composepass.framebuffer.vkFrameBuffer,
        .renderArea = {
                .offset = {0,0},
                .extent = {composepass.framebuffer.width, composepass.framebuffer.height},
        },
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data(),
    };
    VkViewport viewport {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(composepass.framebuffer.width),
            .height = static_cast<float>(composepass.framebuffer.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };
    VkRect2D scissor{{0,0}, {composepass.framebuffer.width, composepass.framebuffer.height}};

    glm::vec2 invScreenSize = 1.0f / glm::vec2(composepass.framebuffer.width, composepass.framebuffer.height);
    vkCmdPushConstants(commandBuffer, composepass.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::vec2), &invScreenSize);

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, composepass.pipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, composepass.pipelineLayout, 0, 1, &composepass.descriptorSet, 0, nullptr);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
    vkCheck(vkEndCommandBuffer(commandBuffer));

}

void RenderSystem::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding textureBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };

    std::array<VkDescriptorSetLayoutBinding, 1> bindings = {textureBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    vkCheck(vkCreateDescriptorSetLayout(device.vkDevice, &layoutInfo, nullptr, &vkDescriptorSetLayout));
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

void RenderSystem::recordCommandBuffer(uint32_t imageIndex, const EvCamera &camera, EvOverlay *overlay) const {
    const VkCommandBuffer& commandBuffer = commandBuffers[imageIndex];
    vkCheck(vkResetCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));

    VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkCheck(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    std::array<VkClearValue, 3> clearValues {};
    clearValues[0] = {
            .color = {0.0f, 0.1f, 0.1f, 1.0f},
    };
    clearValues[1] = {
            .color = {0.0f, 0.1f, 0.1f, 1.0f},
    };
    clearValues[2] = {
            .depthStencil = {1.0f, 0},
    };

    VkRenderPassBeginInfo renderPassInfo {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = swapchain->vkRenderPass,
            .framebuffer = swapchain->vkFramebuffers[imageIndex],
            .renderArea = {
                    .offset = {0,0},
                    .extent = swapchain->extent,
            },
            .clearValueCount = static_cast<uint32_t>(clearValues.size()),
            .pClearValues = clearValues.data(),
    };
    VkViewport viewport {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(swapchain->extent.width),
            .height = static_cast<float>(swapchain->extent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };
    VkRect2D scissor{{0,0}, swapchain->extent};

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    rastPipeline->bind(commandBuffer);

    PushConstant push{
        .camera = camera.getVPMatrix(device.window.getAspectRatio()),
    };

    for (const auto& entity : m_entities) {
        auto& modelComp = m_coordinator->GetComponent<ModelComponent>(entity);
        auto scaleMatrix = glm::scale(glm::mat4(1.0f), modelComp.scale);
        push.mvp = modelComp.transform * scaleMatrix;
        vkCmdPushConstants(
                commandBuffer,
                vkPipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(push),
                &push);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipelineLayout, 0, 1, &modelComp.model->vkDescriptorSets[imageIndex], 0, nullptr);
        modelComp.model->bind(commandBuffer);
        modelComp.model->draw(commandBuffer);
    }

    if (overlay)
        overlay->Draw(commandBuffer);
    vkCmdEndRenderPass(commandBuffer);
    vkCheck(vkEndCommandBuffer(commandBuffer));
}

void RenderSystem::recreateSwapchain() {
    vkCheck(vkDeviceWaitIdle(device.vkDevice));
    device.window.waitForEvent();
    gpass.framebuffer.destroy(device);
    composepass.framebuffer.destroy(device);
    createSwapchain();
    createGBuffer();
    createComposeBuffer();
    createComposeDescriptorSet();
}

void RenderSystem::Render(const EvCamera &camera, EvOverlay* overlay) {
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


    recordCommandBuffer(imageIndex, camera, overlay);

    VkResult presentResult = swapchain->presentCommandBuffer(commandBuffers[imageIndex], imageIndex);

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || device.window.wasResized) {
        device.window.wasResized = false;
        recreateSwapchain();
        return;
    } else if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image");
    }

    std::array<VkCommandBuffer, 2> defferedPasses {
        gpass.commandBuffer,
        composepass.commandBuffer,
    };
    vkDeviceWaitIdle(device.vkDevice);
    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = static_cast<uint32_t>(defferedPasses.size()),
        .pCommandBuffers = defferedPasses.data(),
    };

    recordGBufferCommands(camera);
    recordComposeCommands();
    vkCheck(vkQueueSubmit(device.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));

    vkDeviceWaitIdle(device.vkDevice);

}

Signature RenderSystem::GetSignature() const {
    Signature signature{};
    signature.set(m_coordinator->GetComponentType<ModelComponent>());
    return signature;
}

std::unique_ptr<EvModel> RenderSystem::createModel(const std::string &filename, const EvTexture *texture) {
    auto ret = std::make_unique<EvModel>(device, filename, texture);
    ret->vkDescriptorSets.resize(swapchain->vkImages.size());

    std::vector<VkDescriptorSetLayout> layouts(ret->vkDescriptorSets.size(), vkDescriptorSetLayout);
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



