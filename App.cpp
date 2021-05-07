#include "App.h"
#include "utils.h"

App::App() {

}

App::~App() {
    printf("Destroying pipeline layout\n");
    vkDestroyPipelineLayout(device.vkDevice, vkPipelineLayout, nullptr);
}

void App::Run() {
    createPipelineLayout();
    createPipeline();

    while (!window.shouldClose()) {
        window.processEvents();
    }
}

void App::createPipelineLayout() {
    VkPipelineLayoutCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 0,
            .pSetLayouts = nullptr,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
    };

    vkCheck(vkCreatePipelineLayout(device.vkDevice, &createInfo, nullptr, &vkPipelineLayout));
}

void App::createPipeline() {
    pipelineInfo = defaultRastPipelineInfo(swapchain.extent.width, swapchain.extent.height);
    pipelineInfo.renderPass = swapchain.vkRenderPass;
    pipelineInfo.layout = vkPipelineLayout;

    rastPipeline = std::make_unique<EvRastPipeline>(
            device, pipelineInfo, "shaders/shader.vert.spv", "shaders/shader.frag.spv");
}



