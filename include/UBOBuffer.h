#pragma once

#include "core.h"
#include "EvDevice.h"

template<typename T>
class UBOBuffer : NoCopy {
    EvDevice& device;

    std::vector<VkBuffer> buffers;
    std::vector<VmaAllocation> allocations;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<T*> mappedMemory;

    void createBuffers(uint32_t nrElements, uint32_t duplication) {
        buffers.resize(duplication);
        allocations.resize(duplication);
        mappedMemory.resize(duplication);
        VkDeviceSize bufferSize = nrElements * sizeof(T);
        for(int i=0; i<duplication; i++) {
            device.createHostBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &buffers[i], &allocations[i]);
            vkCheck(vmaMapMemory(device.vmaAllocator, allocations[i], (void**)&mappedMemory[i]));
        }
    }
    void allocateDescriptorSets(uint32_t duplication, VkDescriptorSetLayout layout) {
        descriptorSets.resize(duplication);
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts(duplication, layout);
        VkDescriptorSetAllocateInfo allocInfo {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = device.vkDescriptorPool,
                .descriptorSetCount = duplication,
                .pSetLayouts = descriptorSetLayouts.data(),
        };

        vkCheck(vkAllocateDescriptorSets(device.vkDevice, &allocInfo, descriptorSets.data()));
    }
    void createDescriptorSets(uint32_t binding) {
        for(int i=0; i<descriptorSets.size(); i++) {
            VkDescriptorBufferInfo bufferDescriptor {
                    .buffer = buffers[i],
                    .offset = 0,
                    .range = VK_WHOLE_SIZE,
            };

            auto write = vks::initializers::writeDescriptorSet(descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, binding, &bufferDescriptor, 1);
            vkUpdateDescriptorSets(device.vkDevice, 1, &write, 0, nullptr);
        }
    }


public:
    UBOBuffer(EvDevice& device, uint32_t nrElements, uint32_t duplication, uint32_t binding, VkDescriptorSetLayout layout) : device(device) {
        createBuffers(nrElements, duplication);
        allocateDescriptorSets(duplication, layout);
        createDescriptorSets(binding);
    }

    inline VkDescriptorSet* getDescriptorSet(uint32_t imageIdx) { assert(imageIdx < descriptorSets.size()); return &descriptorSets[imageIdx]; }
    inline T* getPtr(uint32_t imageIdx) const { assert(imageIdx < descriptorSets.size()); return mappedMemory[imageIdx];}

    void destroy() {
        for(int i=0; i<buffers.size(); i++) {
            vmaUnmapMemory(device.vmaAllocator, allocations[i]);
            vmaDestroyBuffer(device.vmaAllocator, buffers[i], allocations[i]);
        }
    }
};