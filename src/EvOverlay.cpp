#include "EvOverlay.h"

// sourced from https://vkguide.dev/docs/extra-chapter/implementing_imgui/

EvOverlay::EvOverlay(EvDevice &device, const EvSwapchain &swapchain) : device(device) {
    createDescriptorPool();
    initImGui(swapchain);
}

EvOverlay::~EvOverlay() {
    vkDestroyDescriptorPool(device.vkDevice, imguiPool, nullptr);
    ImGui_ImplVulkan_Shutdown();
    ImGui::DestroyContext();
}

void EvOverlay::createDescriptorPool() {
    //1: create descriptor pool for IMGUI
    // the size of the pool is very oversize, but it's copied from imgui demo itself.
    VkDescriptorPoolSize pool_sizes[] =
            {
                    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
            };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    vkCheck(vkCreateDescriptorPool(device.vkDevice, &pool_info, nullptr, &imguiPool));


}

void EvOverlay::initImGui(const EvSwapchain &swapchain) {
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForVulkan(device.window.glfwWindow, true);

    ImGui_ImplVulkan_InitInfo initInfo {
        .Instance = device.vkInstance,
        .PhysicalDevice = device.vkPhysicalDevice,
        .Device = device.vkDevice,
        .Queue = device.graphicsQueue,
        .DescriptorPool = imguiPool,
        .MinImageCount= static_cast<uint32_t>(swapchain.vkImages.size()),
        .ImageCount= static_cast<uint32_t>(swapchain.vkImages.size()),
        .MSAASamples = device.msaaSamples,
    };

    ImGui_ImplVulkan_Init(&initInfo, swapchain.vkRenderPass);

    auto initCmd = device.beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture(initCmd);
    device.endSingleTimeCommands(initCmd);

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void EvOverlay::NewFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();
    ImGui::Render();
}

void EvOverlay::Draw(VkCommandBuffer commandBuffer) {
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}
