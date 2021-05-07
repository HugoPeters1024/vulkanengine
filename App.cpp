#include "App.h"
#include "utils.h"

void App::Run() {
    createPipeline();

    while (!window.shouldClose()) {
        window.processEvents();
    }
}

void App::createPipeline() {
    pipelineInfo = defaultRastPipelineInfo(swapchain.extent.width, swapchain.extent.height);
    pipelineInfo.renderPass = swapchain.vkRenderPass;
    rastPipeline = std::make_unique<EvRastPipeline>(
            device, pipelineInfo, "shaders/shader.vert.spv", "shaders/shader.frag.spv");
}
