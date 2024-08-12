/***************************************************************************
 *
 * Copyright (c) 2015-2024 The Khronos Group Inc.
 * Copyright (c) 2015-2024 Valve Corporation
 * Copyright (c) 2015-2024 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ****************************************************************************/

#include "generated/chassis.h"
#include "generated/layer_chassis_dispatch.h"
#include <vulkan/utility/vk_safe_struct.hpp>
#include "state_tracker/pipeline_state.h"

std::shared_mutex dispatch_lock;

#ifdef VK_USE_PLATFORM_METAL_EXT
// The vkExportMetalObjects extension returns data from the driver -- we've created a copy of the pNext chain, so
// copy the returned data to the caller
void CopyExportMetalObjects(const void *src_chain, const void *dst_chain) {
    while (src_chain && dst_chain) {
        const VkStructureType type = reinterpret_cast<const VkBaseOutStructure *>(src_chain)->sType;
        switch (type) {
            case VK_STRUCTURE_TYPE_EXPORT_METAL_DEVICE_INFO_EXT: {
                auto *pSrc = reinterpret_cast<const VkExportMetalDeviceInfoEXT *>(src_chain);
                auto *pDstConst = reinterpret_cast<const VkExportMetalDeviceInfoEXT *>(dst_chain);
                auto *pDst = const_cast<VkExportMetalDeviceInfoEXT *>(pDstConst);
                pDst->mtlDevice = pSrc->mtlDevice;
                break;
            }
            case VK_STRUCTURE_TYPE_EXPORT_METAL_COMMAND_QUEUE_INFO_EXT: {
                const auto *pSrc = reinterpret_cast<const VkExportMetalCommandQueueInfoEXT *>(src_chain);
                auto *pDstConst = reinterpret_cast<const VkExportMetalCommandQueueInfoEXT *>(dst_chain);
                auto *pDst = const_cast<VkExportMetalCommandQueueInfoEXT *>(pDstConst);
                pDst->mtlCommandQueue = pSrc->mtlCommandQueue;
                break;
            }
            case VK_STRUCTURE_TYPE_EXPORT_METAL_BUFFER_INFO_EXT: {
                const auto *pSrc = reinterpret_cast<const VkExportMetalBufferInfoEXT *>(src_chain);
                auto *pDstConst = reinterpret_cast<const VkExportMetalBufferInfoEXT *>(dst_chain);
                auto *pDst = const_cast<VkExportMetalBufferInfoEXT *>(pDstConst);
                pDst->mtlBuffer = pSrc->mtlBuffer;
                break;
            }
            case VK_STRUCTURE_TYPE_EXPORT_METAL_TEXTURE_INFO_EXT: {
                const auto *pSrc = reinterpret_cast<const VkExportMetalTextureInfoEXT *>(src_chain);
                auto *pDstConst = reinterpret_cast<const VkExportMetalTextureInfoEXT *>(dst_chain);
                auto *pDst = const_cast<VkExportMetalTextureInfoEXT *>(pDstConst);
                pDst->mtlTexture = pSrc->mtlTexture;
                break;
            }
            case VK_STRUCTURE_TYPE_EXPORT_METAL_IO_SURFACE_INFO_EXT: {
                const auto *pSrc = reinterpret_cast<const VkExportMetalIOSurfaceInfoEXT *>(src_chain);
                auto *pDstConst = reinterpret_cast<const VkExportMetalIOSurfaceInfoEXT *>(dst_chain);
                auto *pDst = const_cast<VkExportMetalIOSurfaceInfoEXT *>(pDstConst);
                pDst->ioSurface = pSrc->ioSurface;
                break;
            }
            case VK_STRUCTURE_TYPE_EXPORT_METAL_SHARED_EVENT_INFO_EXT: {
                const auto *pSrc = reinterpret_cast<const VkExportMetalSharedEventInfoEXT *>(src_chain);
                auto *pDstConst = reinterpret_cast<const VkExportMetalSharedEventInfoEXT *>(dst_chain);
                auto *pDst = const_cast<VkExportMetalSharedEventInfoEXT *>(pDstConst);
                pDst->mtlSharedEvent = pSrc->mtlSharedEvent;
                break;
            }
            default:
                assert(false);
                break;
        }

        // Handle pNext chaining
        src_chain = reinterpret_cast<const VkBaseOutStructure *>(src_chain)->pNext;
        dst_chain = reinterpret_cast<const VkBaseOutStructure *>(dst_chain)->pNext;
    }
}

void DispatchExportMetalObjectsEXT(VkDevice device, VkExportMetalObjectsInfoEXT *pMetalObjectsInfo) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles) return layer_data->device_dispatch_table.ExportMetalObjectsEXT(device, pMetalObjectsInfo);
    vku::safe_VkExportMetalObjectsInfoEXT local_pMetalObjectsInfo;
    {
        if (pMetalObjectsInfo) {
            local_pMetalObjectsInfo.initialize(pMetalObjectsInfo);
            UnwrapPnextChainHandles(layer_data, local_pMetalObjectsInfo.pNext);
        }
    }
    layer_data->device_dispatch_table.ExportMetalObjectsEXT(device, (VkExportMetalObjectsInfoEXT *)&local_pMetalObjectsInfo);
    if (pMetalObjectsInfo) {
        CopyExportMetalObjects(local_pMetalObjectsInfo.pNext, pMetalObjectsInfo->pNext);
    }
}

#endif  // VK_USE_PLATFORM_METAL_EXT

// The VK_EXT_pipeline_creation_feedback extension returns data from the driver -- we've created a copy of the pnext chain, so
// copy the returned data to the caller before freeing the copy's data.
void CopyCreatePipelineFeedbackData(const void *src_chain, const void *dst_chain) {
    auto src_feedback_struct = vku::FindStructInPNextChain<VkPipelineCreationFeedbackCreateInfoEXT>(src_chain);
    auto dst_feedback_struct = const_cast<VkPipelineCreationFeedbackCreateInfoEXT *>(
        vku::FindStructInPNextChain<VkPipelineCreationFeedbackCreateInfoEXT>(dst_chain));
    if (!src_feedback_struct || !dst_feedback_struct) return;
    ASSERT_AND_RETURN(dst_feedback_struct->pPipelineCreationFeedback && src_feedback_struct->pPipelineCreationFeedback);

    *dst_feedback_struct->pPipelineCreationFeedback = *src_feedback_struct->pPipelineCreationFeedback;
    for (uint32_t i = 0; i < src_feedback_struct->pipelineStageCreationFeedbackCount; i++) {
        dst_feedback_struct->pPipelineStageCreationFeedbacks[i] = src_feedback_struct->pPipelineStageCreationFeedbacks[i];
    }
}

VkResult DispatchCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
                                         const VkGraphicsPipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator,
                                         VkPipeline *pPipelines) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles)
        return layer_data->device_dispatch_table.CreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos,
                                                                         pAllocator, pPipelines);
    vku::safe_VkGraphicsPipelineCreateInfo *local_pCreateInfos = nullptr;
    if (pCreateInfos) {
        local_pCreateInfos = new vku::safe_VkGraphicsPipelineCreateInfo[createInfoCount];
        ReadLockGuard lock(dispatch_lock);
        for (uint32_t idx0 = 0; idx0 < createInfoCount; ++idx0) {
            bool uses_color_attachment = false;
            bool uses_depthstencil_attachment = false;
            {
                const auto subpasses_uses_it =
                    layer_data->renderpasses_states.find(layer_data->Unwrap(pCreateInfos[idx0].renderPass));
                if (subpasses_uses_it != layer_data->renderpasses_states.end()) {
                    const auto &subpasses_uses = subpasses_uses_it->second;
                    if (subpasses_uses.subpasses_using_color_attachment.count(pCreateInfos[idx0].subpass))
                        uses_color_attachment = true;
                    if (subpasses_uses.subpasses_using_depthstencil_attachment.count(pCreateInfos[idx0].subpass))
                        uses_depthstencil_attachment = true;
                }
            }

            auto dynamic_rendering = vku::FindStructInPNextChain<VkPipelineRenderingCreateInfo>(pCreateInfos[idx0].pNext);
            if (dynamic_rendering) {
                uses_color_attachment = (dynamic_rendering->colorAttachmentCount > 0);
                uses_depthstencil_attachment = (dynamic_rendering->depthAttachmentFormat != VK_FORMAT_UNDEFINED ||
                                                dynamic_rendering->stencilAttachmentFormat != VK_FORMAT_UNDEFINED);
            }

            auto &graphics_info = pCreateInfos[idx0];
            auto state_info = dynamic_cast<ValidationStateTracker *>(layer_data);
            vku::PNextCopyState pnext_copy_state = {
                [state_info, &graphics_info](VkBaseOutStructure *safe_struct, const VkBaseOutStructure *in_struct) -> bool {
                    return vvl::Pipeline::PnextRenderingInfoCustomCopy(state_info, graphics_info, safe_struct, in_struct);
                }};
            local_pCreateInfos[idx0].initialize(&pCreateInfos[idx0], uses_color_attachment, uses_depthstencil_attachment,
                                                &pnext_copy_state);

            if (pCreateInfos[idx0].basePipelineHandle) {
                local_pCreateInfos[idx0].basePipelineHandle = layer_data->Unwrap(pCreateInfos[idx0].basePipelineHandle);
            }
            if (pCreateInfos[idx0].layout) {
                local_pCreateInfos[idx0].layout = layer_data->Unwrap(pCreateInfos[idx0].layout);
            }
            if (pCreateInfos[idx0].pStages) {
                for (uint32_t idx1 = 0; idx1 < pCreateInfos[idx0].stageCount; ++idx1) {
                    if (pCreateInfos[idx0].pStages[idx1].module) {
                        local_pCreateInfos[idx0].pStages[idx1].module = layer_data->Unwrap(pCreateInfos[idx0].pStages[idx1].module);
                    }
                }
            }
            if (pCreateInfos[idx0].renderPass) {
                local_pCreateInfos[idx0].renderPass = layer_data->Unwrap(pCreateInfos[idx0].renderPass);
            }

            auto *link_info = vku::FindStructInPNextChain<VkPipelineLibraryCreateInfoKHR>(local_pCreateInfos[idx0].pNext);
            if (link_info) {
                auto *unwrapped_libs = const_cast<VkPipeline *>(link_info->pLibraries);
                for (uint32_t idx1 = 0; idx1 < link_info->libraryCount; ++idx1) {
                    unwrapped_libs[idx1] = layer_data->Unwrap(link_info->pLibraries[idx1]);
                }
            }

            auto device_generated_commands =
                vku::FindStructInPNextChain<VkGraphicsPipelineShaderGroupsCreateInfoNV>(local_pCreateInfos[idx0].pNext);
            if (device_generated_commands) {
                for (uint32_t idx1 = 0; idx1 < device_generated_commands->groupCount; ++idx1) {
                    for (uint32_t idx2 = 0; idx2 < device_generated_commands->pGroups[idx1].stageCount; ++idx2) {
                        auto unwrapped_stage =
                            const_cast<VkPipelineShaderStageCreateInfo *>(&device_generated_commands->pGroups[idx1].pStages[idx2]);
                        if (device_generated_commands->pGroups[idx1].pStages[idx2].module) {
                            unwrapped_stage->module =
                                layer_data->Unwrap(device_generated_commands->pGroups[idx1].pStages[idx2].module);
                        }
                    }
                }
                auto unwrapped_pipelines = const_cast<VkPipeline *>(device_generated_commands->pPipelines);
                for (uint32_t idx1 = 0; idx1 < device_generated_commands->pipelineCount; ++idx1) {
                    unwrapped_pipelines[idx1] = layer_data->Unwrap(device_generated_commands->pPipelines[idx1]);
                }
            }
        }
    }
    if (pipelineCache) {
        pipelineCache = layer_data->Unwrap(pipelineCache);
    }

    VkResult result = layer_data->device_dispatch_table.CreateGraphicsPipelines(device, pipelineCache, createInfoCount,
                                                                                local_pCreateInfos->ptr(), pAllocator, pPipelines);
    for (uint32_t i = 0; i < createInfoCount; ++i) {
        if (pCreateInfos[i].pNext != VK_NULL_HANDLE) {
            CopyCreatePipelineFeedbackData(local_pCreateInfos[i].pNext, pCreateInfos[i].pNext);
        }
    }

    delete[] local_pCreateInfos;
    {
        for (uint32_t i = 0; i < createInfoCount; ++i) {
            if (pPipelines[i] != VK_NULL_HANDLE) {
                pPipelines[i] = layer_data->WrapNew(pPipelines[i]);
            }
        }
    }
    return result;
}

template <typename T>
static void UpdateCreateRenderPassState(ValidationObject *layer_data, const T *pCreateInfo, VkRenderPass renderPass) {
    auto &renderpass_state = layer_data->renderpasses_states[renderPass];

    for (uint32_t subpass = 0; subpass < pCreateInfo->subpassCount; ++subpass) {
        bool uses_color = false;
        for (uint32_t i = 0; i < pCreateInfo->pSubpasses[subpass].colorAttachmentCount && !uses_color; ++i)
            if (pCreateInfo->pSubpasses[subpass].pColorAttachments[i].attachment != VK_ATTACHMENT_UNUSED) uses_color = true;

        bool uses_depthstencil = false;
        if (pCreateInfo->pSubpasses[subpass].pDepthStencilAttachment)
            if (pCreateInfo->pSubpasses[subpass].pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED)
                uses_depthstencil = true;

        if (uses_color) renderpass_state.subpasses_using_color_attachment.insert(subpass);
        if (uses_depthstencil) renderpass_state.subpasses_using_depthstencil_attachment.insert(subpass);
    }
}

template <>
void UpdateCreateRenderPassState(ValidationObject *layer_data, const VkRenderPassCreateInfo2 *pCreateInfo,
                                 VkRenderPass renderPass) {
    auto &renderpass_state = layer_data->renderpasses_states[renderPass];

    for (uint32_t subpassIndex = 0; subpassIndex < pCreateInfo->subpassCount; ++subpassIndex) {
        bool uses_color = false;
        const VkSubpassDescription2 &subpass = pCreateInfo->pSubpasses[subpassIndex];
        for (uint32_t i = 0; i < subpass.colorAttachmentCount && !uses_color; ++i)
            if (subpass.pColorAttachments[i].attachment != VK_ATTACHMENT_UNUSED) uses_color = true;

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
        // VK_ANDROID_external_format_resolve allows for the only color attachment to be VK_ATTACHMENT_UNUSED
        // but in this case, it will use the resolve attachment as color attachment. Which means that we do
        // actually use color attachments
        if (subpass.pResolveAttachments != nullptr) {
            for (uint32_t i = 0; i < subpass.colorAttachmentCount && !uses_color; ++i) {
                uint32_t resolveAttachmentIndex = subpass.pResolveAttachments[i].attachment;
                const void *resolveAtatchmentPNextChain = pCreateInfo->pAttachments[resolveAttachmentIndex].pNext;
                if (vku::FindStructInPNextChain<VkExternalFormatANDROID>(resolveAtatchmentPNextChain)) uses_color = true;
            }
        }
#endif

        bool uses_depthstencil = false;
        if (subpass.pDepthStencilAttachment)
            if (subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) uses_depthstencil = true;

        if (uses_color) renderpass_state.subpasses_using_color_attachment.insert(subpassIndex);
        if (uses_depthstencil) renderpass_state.subpasses_using_depthstencil_attachment.insert(subpassIndex);
    }
}

VkResult DispatchCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo *pCreateInfo,
                                  const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    VkResult result = layer_data->device_dispatch_table.CreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
    if (!wrap_handles) return result;
    if (VK_SUCCESS == result) {
        WriteLockGuard lock(dispatch_lock);
        UpdateCreateRenderPassState(layer_data, pCreateInfo, *pRenderPass);
        *pRenderPass = layer_data->WrapNew(*pRenderPass);
    }
    return result;
}

VkResult DispatchCreateRenderPass2KHR(VkDevice device, const VkRenderPassCreateInfo2 *pCreateInfo,
                                      const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    VkResult result = layer_data->device_dispatch_table.CreateRenderPass2KHR(device, pCreateInfo, pAllocator, pRenderPass);
    if (!wrap_handles) return result;
    if (VK_SUCCESS == result) {
        WriteLockGuard lock(dispatch_lock);
        UpdateCreateRenderPassState(layer_data, pCreateInfo, *pRenderPass);
        *pRenderPass = layer_data->WrapNew(*pRenderPass);
    }
    return result;
}

VkResult DispatchCreateRenderPass2(VkDevice device, const VkRenderPassCreateInfo2 *pCreateInfo,
                                   const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    VkResult result = layer_data->device_dispatch_table.CreateRenderPass2(device, pCreateInfo, pAllocator, pRenderPass);
    if (!wrap_handles) return result;
    if (VK_SUCCESS == result) {
        WriteLockGuard lock(dispatch_lock);
        UpdateCreateRenderPassState(layer_data, pCreateInfo, *pRenderPass);
        *pRenderPass = layer_data->WrapNew(*pRenderPass);
    }
    return result;
}

void DispatchDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks *pAllocator) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles) return layer_data->device_dispatch_table.DestroyRenderPass(device, renderPass, pAllocator);
    uint64_t renderPass_id = CastToUint64(renderPass);

    auto iter = unique_id_mapping.pop(renderPass_id);
    if (iter != unique_id_mapping.end()) {
        renderPass = (VkRenderPass)iter->second;
    } else {
        renderPass = (VkRenderPass)0;
    }

    layer_data->device_dispatch_table.DestroyRenderPass(device, renderPass, pAllocator);

    WriteLockGuard lock(dispatch_lock);
    layer_data->renderpasses_states.erase(renderPass);
}

VkResult DispatchGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pSwapchainImageCount,
                                       VkImage *pSwapchainImages) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles)
        return layer_data->device_dispatch_table.GetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
    VkSwapchainKHR wrapped_swapchain_handle = swapchain;
    if (VK_NULL_HANDLE != swapchain) {
        swapchain = layer_data->Unwrap(swapchain);
    }
    VkResult result =
        layer_data->device_dispatch_table.GetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
    if ((VK_SUCCESS == result) || (VK_INCOMPLETE == result)) {
        if ((*pSwapchainImageCount > 0) && pSwapchainImages) {
            WriteLockGuard lock(dispatch_lock);
            auto &wrapped_swapchain_image_handles = layer_data->swapchain_wrapped_image_handle_map[wrapped_swapchain_handle];
            for (uint32_t i = static_cast<uint32_t>(wrapped_swapchain_image_handles.size()); i < *pSwapchainImageCount; i++) {
                wrapped_swapchain_image_handles.emplace_back(layer_data->WrapNew(pSwapchainImages[i]));
            }
            for (uint32_t i = 0; i < *pSwapchainImageCount; i++) {
                pSwapchainImages[i] = wrapped_swapchain_image_handles[i];
            }
        }
    }
    return result;
}

void DispatchDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks *pAllocator) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles) return layer_data->device_dispatch_table.DestroySwapchainKHR(device, swapchain, pAllocator);
    WriteLockGuard lock(dispatch_lock);

    auto &image_array = layer_data->swapchain_wrapped_image_handle_map[swapchain];
    for (auto &image_handle : image_array) {
        unique_id_mapping.erase(HandleToUint64(image_handle));
    }
    layer_data->swapchain_wrapped_image_handle_map.erase(swapchain);
    lock.unlock();

    uint64_t swapchain_id = HandleToUint64(swapchain);

    auto iter = unique_id_mapping.pop(swapchain_id);
    if (iter != unique_id_mapping.end()) {
        swapchain = (VkSwapchainKHR)iter->second;
    } else {
        swapchain = (VkSwapchainKHR)0;
    }

    layer_data->device_dispatch_table.DestroySwapchainKHR(device, swapchain, pAllocator);
}

VkResult DispatchQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(queue), layer_data_map);
    if (!wrap_handles) return layer_data->device_dispatch_table.QueuePresentKHR(queue, pPresentInfo);
    vku::safe_VkPresentInfoKHR *local_pPresentInfo = nullptr;
    {
        if (pPresentInfo) {
            local_pPresentInfo = new vku::safe_VkPresentInfoKHR(pPresentInfo);
            if (local_pPresentInfo->pWaitSemaphores) {
                for (uint32_t index1 = 0; index1 < local_pPresentInfo->waitSemaphoreCount; ++index1) {
                    local_pPresentInfo->pWaitSemaphores[index1] = layer_data->Unwrap(pPresentInfo->pWaitSemaphores[index1]);
                }
            }
            if (local_pPresentInfo->pSwapchains) {
                for (uint32_t index1 = 0; index1 < local_pPresentInfo->swapchainCount; ++index1) {
                    local_pPresentInfo->pSwapchains[index1] = layer_data->Unwrap(pPresentInfo->pSwapchains[index1]);
                }
            }
            UnwrapPnextChainHandles(layer_data, local_pPresentInfo->pNext);
        }
    }
    VkResult result = layer_data->device_dispatch_table.QueuePresentKHR(queue, local_pPresentInfo->ptr());

    // pResults is an output array embedded in a structure. The code generator neglects to copy back from the vku::safe *version,
    // so handle it as a special case here:
    if (pPresentInfo && pPresentInfo->pResults) {
        for (uint32_t i = 0; i < pPresentInfo->swapchainCount; i++) {
            pPresentInfo->pResults[i] = local_pPresentInfo->pResults[i];
        }
    }
    delete local_pPresentInfo;
    return result;
}

void DispatchDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks *pAllocator) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles) return layer_data->device_dispatch_table.DestroyDescriptorPool(device, descriptorPool, pAllocator);
    WriteLockGuard lock(dispatch_lock);

    // remove references to implicitly freed descriptor sets
    for (auto descriptor_set : layer_data->pool_descriptor_sets_map[descriptorPool]) {
        unique_id_mapping.erase(CastToUint64(descriptor_set));
    }
    layer_data->pool_descriptor_sets_map.erase(descriptorPool);
    lock.unlock();

    uint64_t descriptorPool_id = CastToUint64(descriptorPool);

    auto iter = unique_id_mapping.pop(descriptorPool_id);
    if (iter != unique_id_mapping.end()) {
        descriptorPool = (VkDescriptorPool)iter->second;
    } else {
        descriptorPool = (VkDescriptorPool)0;
    }

    layer_data->device_dispatch_table.DestroyDescriptorPool(device, descriptorPool, pAllocator);
}

VkResult DispatchResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles) return layer_data->device_dispatch_table.ResetDescriptorPool(device, descriptorPool, flags);
    VkDescriptorPool local_descriptor_pool = VK_NULL_HANDLE;
    { local_descriptor_pool = layer_data->Unwrap(descriptorPool); }
    VkResult result = layer_data->device_dispatch_table.ResetDescriptorPool(device, local_descriptor_pool, flags);
    if (VK_SUCCESS == result) {
        WriteLockGuard lock(dispatch_lock);
        // remove references to implicitly freed descriptor sets
        for (auto descriptor_set : layer_data->pool_descriptor_sets_map[descriptorPool]) {
            unique_id_mapping.erase(CastToUint64(descriptor_set));
        }
        layer_data->pool_descriptor_sets_map[descriptorPool].clear();
    }

    return result;
}

VkResult DispatchAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo *pAllocateInfo,
                                        VkDescriptorSet *pDescriptorSets) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles) return layer_data->device_dispatch_table.AllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);
    vku::safe_VkDescriptorSetAllocateInfo *local_pAllocateInfo = nullptr;
    {
        if (pAllocateInfo) {
            local_pAllocateInfo = new vku::safe_VkDescriptorSetAllocateInfo(pAllocateInfo);
            if (pAllocateInfo->descriptorPool) {
                local_pAllocateInfo->descriptorPool = layer_data->Unwrap(pAllocateInfo->descriptorPool);
            }
            if (local_pAllocateInfo->pSetLayouts) {
                for (uint32_t index1 = 0; index1 < local_pAllocateInfo->descriptorSetCount; ++index1) {
                    local_pAllocateInfo->pSetLayouts[index1] = layer_data->Unwrap(local_pAllocateInfo->pSetLayouts[index1]);
                }
            }
        }
    }
    VkResult result = layer_data->device_dispatch_table.AllocateDescriptorSets(
        device, (const VkDescriptorSetAllocateInfo *)local_pAllocateInfo, pDescriptorSets);
    if (local_pAllocateInfo) {
        delete local_pAllocateInfo;
    }
    if (VK_SUCCESS == result) {
        WriteLockGuard lock(dispatch_lock);
        auto &pool_descriptor_sets = layer_data->pool_descriptor_sets_map[pAllocateInfo->descriptorPool];
        for (uint32_t index0 = 0; index0 < pAllocateInfo->descriptorSetCount; index0++) {
            pDescriptorSets[index0] = layer_data->WrapNew(pDescriptorSets[index0]);
            pool_descriptor_sets.insert(pDescriptorSets[index0]);
        }
    }
    return result;
}

VkResult DispatchFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount,
                                    const VkDescriptorSet *pDescriptorSets) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles)
        return layer_data->device_dispatch_table.FreeDescriptorSets(device, descriptorPool, descriptorSetCount, pDescriptorSets);
    VkDescriptorSet *local_pDescriptorSets = nullptr;
    VkDescriptorPool local_descriptor_pool = VK_NULL_HANDLE;
    {
        local_descriptor_pool = layer_data->Unwrap(descriptorPool);
        if (pDescriptorSets) {
            local_pDescriptorSets = new VkDescriptorSet[descriptorSetCount];
            for (uint32_t index0 = 0; index0 < descriptorSetCount; ++index0) {
                local_pDescriptorSets[index0] = layer_data->Unwrap(pDescriptorSets[index0]);
            }
        }
    }
    VkResult result = layer_data->device_dispatch_table.FreeDescriptorSets(device, local_descriptor_pool, descriptorSetCount,
                                                                           (const VkDescriptorSet *)local_pDescriptorSets);
    if (local_pDescriptorSets) delete[] local_pDescriptorSets;
    if ((VK_SUCCESS == result) && (pDescriptorSets)) {
        WriteLockGuard lock(dispatch_lock);
        auto &pool_descriptor_sets = layer_data->pool_descriptor_sets_map[descriptorPool];
        for (uint32_t index0 = 0; index0 < descriptorSetCount; index0++) {
            VkDescriptorSet handle = pDescriptorSets[index0];
            pool_descriptor_sets.erase(handle);
            uint64_t unique_id = CastToUint64(handle);
            unique_id_mapping.erase(unique_id);
        }
    }
    return result;
}

// This is the core version of this routine.  The extension version is below.
VkResult DispatchCreateDescriptorUpdateTemplate(VkDevice device, const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo,
                                                const VkAllocationCallbacks *pAllocator,
                                                VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles)
        return layer_data->device_dispatch_table.CreateDescriptorUpdateTemplate(device, pCreateInfo, pAllocator,
                                                                                pDescriptorUpdateTemplate);
    vku::safe_VkDescriptorUpdateTemplateCreateInfo var_local_pCreateInfo;
    vku::safe_VkDescriptorUpdateTemplateCreateInfo *local_pCreateInfo = nullptr;
    if (pCreateInfo) {
        local_pCreateInfo = &var_local_pCreateInfo;
        local_pCreateInfo->initialize(pCreateInfo);
        if (pCreateInfo->templateType == VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET) {
            local_pCreateInfo->descriptorSetLayout = layer_data->Unwrap(pCreateInfo->descriptorSetLayout);
        }
        if (pCreateInfo->templateType == VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_PUSH_DESCRIPTORS_KHR) {
            local_pCreateInfo->pipelineLayout = layer_data->Unwrap(pCreateInfo->pipelineLayout);
        }
    }
    VkResult result = layer_data->device_dispatch_table.CreateDescriptorUpdateTemplate(device, local_pCreateInfo->ptr(), pAllocator,
                                                                                       pDescriptorUpdateTemplate);
    if (VK_SUCCESS == result) {
        *pDescriptorUpdateTemplate = layer_data->WrapNew(*pDescriptorUpdateTemplate);

        // Shadow template createInfo for later updates
        if (local_pCreateInfo) {
            WriteLockGuard lock(dispatch_lock);
            std::unique_ptr<TemplateState> template_state(new TemplateState(*pDescriptorUpdateTemplate, local_pCreateInfo));
            layer_data->desc_template_createinfo_map[(uint64_t)*pDescriptorUpdateTemplate] = std::move(template_state);
        }
    }
    return result;
}

// This is the extension version of this routine.  The core version is above.
VkResult DispatchCreateDescriptorUpdateTemplateKHR(VkDevice device, const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo,
                                                   const VkAllocationCallbacks *pAllocator,
                                                   VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles)
        return layer_data->device_dispatch_table.CreateDescriptorUpdateTemplateKHR(device, pCreateInfo, pAllocator,
                                                                                   pDescriptorUpdateTemplate);
    vku::safe_VkDescriptorUpdateTemplateCreateInfo var_local_pCreateInfo;
    vku::safe_VkDescriptorUpdateTemplateCreateInfo *local_pCreateInfo = nullptr;
    if (pCreateInfo) {
        local_pCreateInfo = &var_local_pCreateInfo;
        local_pCreateInfo->initialize(pCreateInfo);
        if (pCreateInfo->templateType == VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET) {
            local_pCreateInfo->descriptorSetLayout = layer_data->Unwrap(pCreateInfo->descriptorSetLayout);
        }
        if (pCreateInfo->templateType == VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_PUSH_DESCRIPTORS_KHR) {
            local_pCreateInfo->pipelineLayout = layer_data->Unwrap(pCreateInfo->pipelineLayout);
        }
    }
    VkResult result = layer_data->device_dispatch_table.CreateDescriptorUpdateTemplateKHR(device, local_pCreateInfo->ptr(),
                                                                                          pAllocator, pDescriptorUpdateTemplate);

    if (VK_SUCCESS == result) {
        *pDescriptorUpdateTemplate = layer_data->WrapNew(*pDescriptorUpdateTemplate);

        // Shadow template createInfo for later updates
        if (local_pCreateInfo) {
            WriteLockGuard lock(dispatch_lock);
            std::unique_ptr<TemplateState> template_state(new TemplateState(*pDescriptorUpdateTemplate, local_pCreateInfo));
            layer_data->desc_template_createinfo_map[(uint64_t)*pDescriptorUpdateTemplate] = std::move(template_state);
        }
    }
    return result;
}

// This is the core version of this routine.  The extension version is below.
void DispatchDestroyDescriptorUpdateTemplate(VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate,
                                             const VkAllocationCallbacks *pAllocator) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles)
        return layer_data->device_dispatch_table.DestroyDescriptorUpdateTemplate(device, descriptorUpdateTemplate, pAllocator);
    WriteLockGuard lock(dispatch_lock);
    uint64_t descriptor_update_template_id = CastToUint64(descriptorUpdateTemplate);
    layer_data->desc_template_createinfo_map.erase(descriptor_update_template_id);
    lock.unlock();

    auto iter = unique_id_mapping.pop(descriptor_update_template_id);
    if (iter != unique_id_mapping.end()) {
        descriptorUpdateTemplate = (VkDescriptorUpdateTemplate)iter->second;
    } else {
        descriptorUpdateTemplate = (VkDescriptorUpdateTemplate)0;
    }

    layer_data->device_dispatch_table.DestroyDescriptorUpdateTemplate(device, descriptorUpdateTemplate, pAllocator);
}

// This is the extension version of this routine.  The core version is above.
void DispatchDestroyDescriptorUpdateTemplateKHR(VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate,
                                                const VkAllocationCallbacks *pAllocator) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles)
        return layer_data->device_dispatch_table.DestroyDescriptorUpdateTemplateKHR(device, descriptorUpdateTemplate, pAllocator);
    WriteLockGuard lock(dispatch_lock);
    uint64_t descriptor_update_template_id = CastToUint64(descriptorUpdateTemplate);
    layer_data->desc_template_createinfo_map.erase(descriptor_update_template_id);
    lock.unlock();

    auto iter = unique_id_mapping.pop(descriptor_update_template_id);
    if (iter != unique_id_mapping.end()) {
        descriptorUpdateTemplate = (VkDescriptorUpdateTemplate)iter->second;
    } else {
        descriptorUpdateTemplate = (VkDescriptorUpdateTemplate)0;
    }

    layer_data->device_dispatch_table.DestroyDescriptorUpdateTemplateKHR(device, descriptorUpdateTemplate, pAllocator);
}

void *BuildUnwrappedUpdateTemplateBuffer(ValidationObject *layer_data, uint64_t descriptorUpdateTemplate, const void *pData) {
    auto const template_map_entry = layer_data->desc_template_createinfo_map.find(descriptorUpdateTemplate);
    auto const &create_info = template_map_entry->second->create_info;
    size_t allocation_size = 0;
    std::vector<std::tuple<size_t, VulkanObjectType, uint64_t, size_t>> template_entries;

    for (uint32_t i = 0; i < create_info.descriptorUpdateEntryCount; i++) {
        for (uint32_t j = 0; j < create_info.pDescriptorUpdateEntries[i].descriptorCount; j++) {
            size_t offset = create_info.pDescriptorUpdateEntries[i].offset + j * create_info.pDescriptorUpdateEntries[i].stride;
            char *update_entry = (char *)(pData) + offset;

            switch (create_info.pDescriptorUpdateEntries[i].descriptorType) {
                case VK_DESCRIPTOR_TYPE_SAMPLER:
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
                    auto image_entry = reinterpret_cast<VkDescriptorImageInfo *>(update_entry);
                    allocation_size = std::max(allocation_size, offset + sizeof(VkDescriptorImageInfo));

                    VkDescriptorImageInfo *wrapped_entry = new VkDescriptorImageInfo(*image_entry);
                    wrapped_entry->sampler = layer_data->Unwrap(image_entry->sampler);
                    wrapped_entry->imageView = layer_data->Unwrap(image_entry->imageView);
                    template_entries.emplace_back(offset, kVulkanObjectTypeImage, CastToUint64(wrapped_entry), 0);
                } break;

                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
                    auto buffer_entry = reinterpret_cast<VkDescriptorBufferInfo *>(update_entry);
                    allocation_size = std::max(allocation_size, offset + sizeof(VkDescriptorBufferInfo));

                    VkDescriptorBufferInfo *wrapped_entry = new VkDescriptorBufferInfo(*buffer_entry);
                    wrapped_entry->buffer = layer_data->Unwrap(buffer_entry->buffer);
                    template_entries.emplace_back(offset, kVulkanObjectTypeBuffer, CastToUint64(wrapped_entry), 0);
                } break;

                case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: {
                    auto buffer_view_handle = reinterpret_cast<VkBufferView *>(update_entry);
                    allocation_size = std::max(allocation_size, offset + sizeof(VkBufferView));

                    VkBufferView wrapped_entry = layer_data->Unwrap(*buffer_view_handle);
                    template_entries.emplace_back(offset, kVulkanObjectTypeBufferView, CastToUint64(wrapped_entry), 0);
                } break;
                case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT: {
                    size_t numBytes = create_info.pDescriptorUpdateEntries[i].descriptorCount;
                    allocation_size = std::max(allocation_size, offset + numBytes);
                    // nothing to unwrap, just plain data
                    template_entries.emplace_back(offset, kVulkanObjectTypeUnknown, CastToUint64(update_entry), numBytes);
                    // to break out of the loop
                    j = create_info.pDescriptorUpdateEntries[i].descriptorCount;
                } break;
                case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV: {
                    auto accstruct_nv_handle = reinterpret_cast<VkAccelerationStructureNV *>(update_entry);
                    allocation_size = std::max(allocation_size, offset + sizeof(VkAccelerationStructureNV));

                    VkAccelerationStructureNV wrapped_entry = layer_data->Unwrap(*accstruct_nv_handle);
                    template_entries.emplace_back(offset, kVulkanObjectTypeAccelerationStructureNV, CastToUint64(wrapped_entry), 0);
                } break;
                case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: {
                    auto accstruct_khr_handle = reinterpret_cast<VkAccelerationStructureKHR *>(update_entry);
                    allocation_size = std::max(allocation_size, offset + sizeof(VkAccelerationStructureKHR));

                    VkAccelerationStructureKHR wrapped_entry = layer_data->Unwrap(*accstruct_khr_handle);
                    template_entries.emplace_back(offset, kVulkanObjectTypeAccelerationStructureKHR, CastToUint64(wrapped_entry),
                                                  0);
                } break;
                default:
                    assert(false);
                    break;
            }
        }
    }
    // Allocate required buffer size and populate with source/unwrapped data
    void *unwrapped_data = malloc(allocation_size);
    for (auto &this_entry : template_entries) {
        VulkanObjectType type = std::get<1>(this_entry);
        void *destination = (char *)unwrapped_data + std::get<0>(this_entry);
        uint64_t source = std::get<2>(this_entry);
        size_t size = std::get<3>(this_entry);

        if (size != 0) {
            assert(type == kVulkanObjectTypeUnknown);
            memcpy(destination, CastFromUint64<void *>(source), size);
        } else {
            switch (type) {
                case kVulkanObjectTypeImage:
                    *(reinterpret_cast<VkDescriptorImageInfo *>(destination)) =
                        *(reinterpret_cast<VkDescriptorImageInfo *>(source));
                    delete CastFromUint64<VkDescriptorImageInfo *>(source);
                    break;
                case kVulkanObjectTypeBuffer:
                    *(reinterpret_cast<VkDescriptorBufferInfo *>(destination)) =
                        *(CastFromUint64<VkDescriptorBufferInfo *>(source));
                    delete CastFromUint64<VkDescriptorBufferInfo *>(source);
                    break;
                case kVulkanObjectTypeBufferView:
                    *(reinterpret_cast<VkBufferView *>(destination)) = CastFromUint64<VkBufferView>(source);
                    break;
                case kVulkanObjectTypeAccelerationStructureKHR:
                    *(reinterpret_cast<VkAccelerationStructureKHR *>(destination)) =
                        CastFromUint64<VkAccelerationStructureKHR>(source);
                    break;
                case kVulkanObjectTypeAccelerationStructureNV:
                    *(reinterpret_cast<VkAccelerationStructureNV *>(destination)) =
                        CastFromUint64<VkAccelerationStructureNV>(source);
                    break;
                default:
                    assert(false);
                    break;
            }
        }
    }
    return (void *)unwrapped_data;
}

void DispatchUpdateDescriptorSetWithTemplate(VkDevice device, VkDescriptorSet descriptorSet,
                                             VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void *pData) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles)
        return layer_data->device_dispatch_table.UpdateDescriptorSetWithTemplate(device, descriptorSet, descriptorUpdateTemplate,
                                                                                 pData);
    uint64_t template_handle = CastToUint64(descriptorUpdateTemplate);
    void *unwrapped_buffer = nullptr;
    {
        ReadLockGuard lock(dispatch_lock);
        descriptorSet = layer_data->Unwrap(descriptorSet);
        descriptorUpdateTemplate = (VkDescriptorUpdateTemplate)layer_data->Unwrap(descriptorUpdateTemplate);
        unwrapped_buffer = BuildUnwrappedUpdateTemplateBuffer(layer_data, template_handle, pData);
    }
    layer_data->device_dispatch_table.UpdateDescriptorSetWithTemplate(device, descriptorSet, descriptorUpdateTemplate,
                                                                      unwrapped_buffer);
    free(unwrapped_buffer);
}

void DispatchUpdateDescriptorSetWithTemplateKHR(VkDevice device, VkDescriptorSet descriptorSet,
                                                VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void *pData) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles)
        return layer_data->device_dispatch_table.UpdateDescriptorSetWithTemplateKHR(device, descriptorSet, descriptorUpdateTemplate,
                                                                                    pData);
    uint64_t template_handle = CastToUint64(descriptorUpdateTemplate);
    void *unwrapped_buffer = nullptr;
    {
        ReadLockGuard lock(dispatch_lock);
        descriptorSet = layer_data->Unwrap(descriptorSet);
        descriptorUpdateTemplate = layer_data->Unwrap(descriptorUpdateTemplate);
        unwrapped_buffer = BuildUnwrappedUpdateTemplateBuffer(layer_data, template_handle, pData);
    }
    layer_data->device_dispatch_table.UpdateDescriptorSetWithTemplateKHR(device, descriptorSet, descriptorUpdateTemplate,
                                                                         unwrapped_buffer);
    free(unwrapped_buffer);
}

void DispatchCmdPushDescriptorSetWithTemplateKHR(VkCommandBuffer commandBuffer, VkDescriptorUpdateTemplate descriptorUpdateTemplate,
                                                 VkPipelineLayout layout, uint32_t set, const void *pData) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(commandBuffer), layer_data_map);
    if (!wrap_handles)
        return layer_data->device_dispatch_table.CmdPushDescriptorSetWithTemplateKHR(commandBuffer, descriptorUpdateTemplate,
                                                                                     layout, set, pData);
    uint64_t template_handle = CastToUint64(descriptorUpdateTemplate);
    void *unwrapped_buffer = nullptr;
    {
        ReadLockGuard lock(dispatch_lock);
        descriptorUpdateTemplate = layer_data->Unwrap(descriptorUpdateTemplate);
        layout = layer_data->Unwrap(layout);
        unwrapped_buffer = BuildUnwrappedUpdateTemplateBuffer(layer_data, template_handle, pData);
    }
    layer_data->device_dispatch_table.CmdPushDescriptorSetWithTemplateKHR(commandBuffer, descriptorUpdateTemplate, layout, set,
                                                                          unwrapped_buffer);
    free(unwrapped_buffer);
}

void DispatchCmdPushDescriptorSetWithTemplate2KHR(
    VkCommandBuffer commandBuffer, const VkPushDescriptorSetWithTemplateInfoKHR *pPushDescriptorSetWithTemplateInfo) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(commandBuffer), layer_data_map);
    if (!wrap_handles)
        return layer_data->device_dispatch_table.CmdPushDescriptorSetWithTemplate2KHR(commandBuffer,
                                                                                      pPushDescriptorSetWithTemplateInfo);
    uint64_t template_handle = CastToUint64(pPushDescriptorSetWithTemplateInfo->descriptorUpdateTemplate);
    void *unwrapped_buffer = nullptr;
    {
        ReadLockGuard lock(dispatch_lock);
        const_cast<VkPushDescriptorSetWithTemplateInfoKHR *>(pPushDescriptorSetWithTemplateInfo)->descriptorUpdateTemplate =
            layer_data->Unwrap(pPushDescriptorSetWithTemplateInfo->descriptorUpdateTemplate);
        const_cast<VkPushDescriptorSetWithTemplateInfoKHR *>(pPushDescriptorSetWithTemplateInfo)->layout =
            layer_data->Unwrap(pPushDescriptorSetWithTemplateInfo->layout);
        unwrapped_buffer =
            BuildUnwrappedUpdateTemplateBuffer(layer_data, template_handle, pPushDescriptorSetWithTemplateInfo->pData);
        const_cast<VkPushDescriptorSetWithTemplateInfoKHR *>(pPushDescriptorSetWithTemplateInfo)->pData = unwrapped_buffer;
    }
    layer_data->device_dispatch_table.CmdPushDescriptorSetWithTemplate2KHR(commandBuffer, pPushDescriptorSetWithTemplateInfo);
    free(unwrapped_buffer);
}

VkResult DispatchGetPhysicalDeviceDisplayPropertiesKHR(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount,
                                                       VkDisplayPropertiesKHR *pProperties) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(physicalDevice), layer_data_map);
    VkResult result =
        layer_data->instance_dispatch_table.GetPhysicalDeviceDisplayPropertiesKHR(physicalDevice, pPropertyCount, pProperties);
    if (!wrap_handles) return result;
    if ((result == VK_SUCCESS || result == VK_INCOMPLETE) && pProperties) {
        for (uint32_t idx0 = 0; idx0 < *pPropertyCount; ++idx0) {
            pProperties[idx0].display = layer_data->MaybeWrapDisplay(pProperties[idx0].display);
        }
    }
    return result;
}

VkResult DispatchGetPhysicalDeviceDisplayProperties2KHR(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount,
                                                        VkDisplayProperties2KHR *pProperties) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(physicalDevice), layer_data_map);
    VkResult result =
        layer_data->instance_dispatch_table.GetPhysicalDeviceDisplayProperties2KHR(physicalDevice, pPropertyCount, pProperties);
    if (!wrap_handles) return result;
    if ((result == VK_SUCCESS || result == VK_INCOMPLETE) && pProperties) {
        for (uint32_t idx0 = 0; idx0 < *pPropertyCount; ++idx0) {
            pProperties[idx0].displayProperties.display = layer_data->MaybeWrapDisplay(pProperties[idx0].displayProperties.display);
        }
    }
    return result;
}

VkResult DispatchGetPhysicalDeviceDisplayPlanePropertiesKHR(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount,
                                                            VkDisplayPlanePropertiesKHR *pProperties) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(physicalDevice), layer_data_map);
    VkResult result =
        layer_data->instance_dispatch_table.GetPhysicalDeviceDisplayPlanePropertiesKHR(physicalDevice, pPropertyCount, pProperties);
    if (!wrap_handles) return result;
    if ((result == VK_SUCCESS || result == VK_INCOMPLETE) && pProperties) {
        for (uint32_t idx0 = 0; idx0 < *pPropertyCount; ++idx0) {
            VkDisplayKHR &opt_display = pProperties[idx0].currentDisplay;
            if (opt_display) opt_display = layer_data->MaybeWrapDisplay(opt_display);
        }
    }
    return result;
}

VkResult DispatchGetPhysicalDeviceDisplayPlaneProperties2KHR(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount,
                                                             VkDisplayPlaneProperties2KHR *pProperties) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(physicalDevice), layer_data_map);
    VkResult result = layer_data->instance_dispatch_table.GetPhysicalDeviceDisplayPlaneProperties2KHR(physicalDevice,
                                                                                                      pPropertyCount, pProperties);
    if (!wrap_handles) return result;
    if ((result == VK_SUCCESS || result == VK_INCOMPLETE) && pProperties) {
        for (uint32_t idx0 = 0; idx0 < *pPropertyCount; ++idx0) {
            VkDisplayKHR &opt_display = pProperties[idx0].displayPlaneProperties.currentDisplay;
            if (opt_display) opt_display = layer_data->MaybeWrapDisplay(opt_display);
        }
    }
    return result;
}

VkResult DispatchGetDisplayPlaneSupportedDisplaysKHR(VkPhysicalDevice physicalDevice, uint32_t planeIndex, uint32_t *pDisplayCount,
                                                     VkDisplayKHR *pDisplays) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(physicalDevice), layer_data_map);
    VkResult result = layer_data->instance_dispatch_table.GetDisplayPlaneSupportedDisplaysKHR(physicalDevice, planeIndex,
                                                                                              pDisplayCount, pDisplays);
    if ((result == VK_SUCCESS || result == VK_INCOMPLETE) && pDisplays) {
        if (!wrap_handles) return result;
        for (uint32_t i = 0; i < *pDisplayCount; ++i) {
            if (pDisplays[i]) pDisplays[i] = layer_data->MaybeWrapDisplay(pDisplays[i]);
        }
    }
    return result;
}

VkResult DispatchGetDisplayModePropertiesKHR(VkPhysicalDevice physicalDevice, VkDisplayKHR display, uint32_t *pPropertyCount,
                                             VkDisplayModePropertiesKHR *pProperties) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(physicalDevice), layer_data_map);
    if (!wrap_handles)
        return layer_data->instance_dispatch_table.GetDisplayModePropertiesKHR(physicalDevice, display, pPropertyCount,
                                                                               pProperties);
    { display = layer_data->Unwrap(display); }

    VkResult result =
        layer_data->instance_dispatch_table.GetDisplayModePropertiesKHR(physicalDevice, display, pPropertyCount, pProperties);
    if ((result == VK_SUCCESS || result == VK_INCOMPLETE) && pProperties) {
        for (uint32_t idx0 = 0; idx0 < *pPropertyCount; ++idx0) {
            pProperties[idx0].displayMode = layer_data->WrapNew(pProperties[idx0].displayMode);
        }
    }
    return result;
}

VkResult DispatchGetDisplayModeProperties2KHR(VkPhysicalDevice physicalDevice, VkDisplayKHR display, uint32_t *pPropertyCount,
                                              VkDisplayModeProperties2KHR *pProperties) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(physicalDevice), layer_data_map);
    if (!wrap_handles)
        return layer_data->instance_dispatch_table.GetDisplayModeProperties2KHR(physicalDevice, display, pPropertyCount,
                                                                                pProperties);
    { display = layer_data->Unwrap(display); }

    VkResult result =
        layer_data->instance_dispatch_table.GetDisplayModeProperties2KHR(physicalDevice, display, pPropertyCount, pProperties);
    if ((result == VK_SUCCESS || result == VK_INCOMPLETE) && pProperties) {
        for (uint32_t idx0 = 0; idx0 < *pPropertyCount; ++idx0) {
            pProperties[idx0].displayModeProperties.displayMode =
                layer_data->WrapNew(pProperties[idx0].displayModeProperties.displayMode);
        }
    }
    return result;
}

VkResult DispatchDebugMarkerSetObjectTagEXT(VkDevice device, const VkDebugMarkerObjectTagInfoEXT *pTagInfo) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles) return layer_data->device_dispatch_table.DebugMarkerSetObjectTagEXT(device, pTagInfo);
    vku::safe_VkDebugMarkerObjectTagInfoEXT local_tag_info(pTagInfo);
    {
        auto it = unique_id_mapping.find(CastToUint64(local_tag_info.object));
        if (it != unique_id_mapping.end()) {
            local_tag_info.object = it->second;
        }
    }
    VkResult result = layer_data->device_dispatch_table.DebugMarkerSetObjectTagEXT(
        device, reinterpret_cast<VkDebugMarkerObjectTagInfoEXT *>(&local_tag_info));
    return result;
}

VkResult DispatchDebugMarkerSetObjectNameEXT(VkDevice device, const VkDebugMarkerObjectNameInfoEXT *pNameInfo) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles) return layer_data->device_dispatch_table.DebugMarkerSetObjectNameEXT(device, pNameInfo);
    vku::safe_VkDebugMarkerObjectNameInfoEXT local_name_info(pNameInfo);
    {
        auto it = unique_id_mapping.find(CastToUint64(local_name_info.object));
        if (it != unique_id_mapping.end()) {
            local_name_info.object = it->second;
        }
    }
    VkResult result = layer_data->device_dispatch_table.DebugMarkerSetObjectNameEXT(
        device, reinterpret_cast<VkDebugMarkerObjectNameInfoEXT *>(&local_name_info));
    return result;
}

// VK_EXT_debug_utils
VkResult DispatchSetDebugUtilsObjectTagEXT(VkDevice device, const VkDebugUtilsObjectTagInfoEXT *pTagInfo) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles) return layer_data->device_dispatch_table.SetDebugUtilsObjectTagEXT(device, pTagInfo);
    vku::safe_VkDebugUtilsObjectTagInfoEXT local_tag_info(pTagInfo);
    {
        auto it = unique_id_mapping.find(CastToUint64(local_tag_info.objectHandle));
        if (it != unique_id_mapping.end()) {
            local_tag_info.objectHandle = it->second;
        }
    }
    VkResult result = layer_data->device_dispatch_table.SetDebugUtilsObjectTagEXT(
        device, reinterpret_cast<const VkDebugUtilsObjectTagInfoEXT *>(&local_tag_info));
    return result;
}

VkResult DispatchSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT *pNameInfo) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles) return layer_data->device_dispatch_table.SetDebugUtilsObjectNameEXT(device, pNameInfo);
    vku::safe_VkDebugUtilsObjectNameInfoEXT local_name_info(pNameInfo);
    {
        auto it = unique_id_mapping.find(CastToUint64(local_name_info.objectHandle));
        if (it != unique_id_mapping.end()) {
            local_name_info.objectHandle = it->second;
        }
    }
    VkResult result = layer_data->device_dispatch_table.SetDebugUtilsObjectNameEXT(
        device, reinterpret_cast<const VkDebugUtilsObjectNameInfoEXT *>(&local_name_info));
    return result;
}

VkResult DispatchGetPhysicalDeviceToolPropertiesEXT(VkPhysicalDevice physicalDevice, uint32_t *pToolCount,
                                                    VkPhysicalDeviceToolPropertiesEXT *pToolProperties) {
    VkResult result = VK_SUCCESS;
    auto layer_data = GetLayerDataPtr(GetDispatchKey(physicalDevice), layer_data_map);
    if (layer_data->instance_dispatch_table.GetPhysicalDeviceToolPropertiesEXT == nullptr) {
        // This layer is the terminator. Set pToolCount to zero.
        *pToolCount = 0;
    } else {
        result =
            layer_data->instance_dispatch_table.GetPhysicalDeviceToolPropertiesEXT(physicalDevice, pToolCount, pToolProperties);
    }

    return result;
}

VkResult DispatchGetPhysicalDeviceToolProperties(VkPhysicalDevice physicalDevice, uint32_t *pToolCount,
                                                 VkPhysicalDeviceToolProperties *pToolProperties) {
    VkResult result = VK_SUCCESS;
    auto layer_data = GetLayerDataPtr(GetDispatchKey(physicalDevice), layer_data_map);
    if (layer_data->instance_dispatch_table.GetPhysicalDeviceToolProperties == nullptr) {
        // This layer is the terminator. Set pToolCount to zero.
        *pToolCount = 0;
    } else {
        result = layer_data->instance_dispatch_table.GetPhysicalDeviceToolProperties(physicalDevice, pToolCount, pToolProperties);
    }

    return result;
}

VkResult DispatchAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pAllocateInfo,
                                        VkCommandBuffer *pCommandBuffers) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles) return layer_data->device_dispatch_table.AllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
    vku::safe_VkCommandBufferAllocateInfo local_pAllocateInfo;
    if (pAllocateInfo) {
        local_pAllocateInfo.initialize(pAllocateInfo);
        if (pAllocateInfo->commandPool) {
            local_pAllocateInfo.commandPool = layer_data->Unwrap(pAllocateInfo->commandPool);
        }
    }
    VkResult result = layer_data->device_dispatch_table.AllocateCommandBuffers(
        device, (const VkCommandBufferAllocateInfo *)&local_pAllocateInfo, pCommandBuffers);
    return result;
}

void DispatchFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount,
                                const VkCommandBuffer *pCommandBuffers) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles)
        return layer_data->device_dispatch_table.FreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
    commandPool = layer_data->Unwrap(commandPool);
    layer_data->device_dispatch_table.FreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
}

void DispatchDestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks *pAllocator) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles) return layer_data->device_dispatch_table.DestroyCommandPool(device, commandPool, pAllocator);
    uint64_t commandPool_id = CastToUint64(commandPool);
    auto iter = unique_id_mapping.pop(commandPool_id);
    if (iter != unique_id_mapping.end()) {
        commandPool = (VkCommandPool)iter->second;
    } else {
        commandPool = (VkCommandPool)0;
    }
    layer_data->device_dispatch_table.DestroyCommandPool(device, commandPool, pAllocator);
}

VkResult DispatchBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo, bool is_secondary) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(commandBuffer), layer_data_map);
    if (!wrap_handles || !is_secondary) return layer_data->device_dispatch_table.BeginCommandBuffer(commandBuffer, pBeginInfo);
    vku::safe_VkCommandBufferBeginInfo local_pBeginInfo;
    if (pBeginInfo) {
        local_pBeginInfo.initialize(pBeginInfo);
        if (local_pBeginInfo.pInheritanceInfo) {
            if (pBeginInfo->pInheritanceInfo->renderPass) {
                local_pBeginInfo.pInheritanceInfo->renderPass = layer_data->Unwrap(pBeginInfo->pInheritanceInfo->renderPass);
            }
            if (pBeginInfo->pInheritanceInfo->framebuffer) {
                local_pBeginInfo.pInheritanceInfo->framebuffer = layer_data->Unwrap(pBeginInfo->pInheritanceInfo->framebuffer);
            }
        }
    }
    VkResult result =
        layer_data->device_dispatch_table.BeginCommandBuffer(commandBuffer, (const VkCommandBufferBeginInfo *)&local_pBeginInfo);
    return result;
}

VkResult DispatchCreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferredOperation,
                                              VkPipelineCache pipelineCache, uint32_t createInfoCount,
                                              const VkRayTracingPipelineCreateInfoKHR *pCreateInfos,
                                              const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    vku::safe_VkRayTracingPipelineCreateInfoKHR *local_pCreateInfos = (vku::safe_VkRayTracingPipelineCreateInfoKHR *)(pCreateInfos);
    if (wrap_handles) {
        deferredOperation = layer_data->Unwrap(deferredOperation);
        pipelineCache = layer_data->Unwrap(pipelineCache);
        if (pCreateInfos) {
            local_pCreateInfos = new vku::safe_VkRayTracingPipelineCreateInfoKHR[createInfoCount];
            for (uint32_t index0 = 0; index0 < createInfoCount; ++index0) {
                local_pCreateInfos[index0].initialize(&pCreateInfos[index0]);
                if (local_pCreateInfos[index0].pStages) {
                    for (uint32_t index1 = 0; index1 < local_pCreateInfos[index0].stageCount; ++index1) {
                        if (pCreateInfos[index0].pStages[index1].module) {
                            local_pCreateInfos[index0].pStages[index1].module =
                                layer_data->Unwrap(pCreateInfos[index0].pStages[index1].module);
                        }
                    }
                }
                if (local_pCreateInfos[index0].pLibraryInfo) {
                    if (local_pCreateInfos[index0].pLibraryInfo->pLibraries) {
                        for (uint32_t index2 = 0; index2 < local_pCreateInfos[index0].pLibraryInfo->libraryCount; ++index2) {
                            local_pCreateInfos[index0].pLibraryInfo->pLibraries[index2] =
                                layer_data->Unwrap(local_pCreateInfos[index0].pLibraryInfo->pLibraries[index2]);
                        }
                    }
                }
                if (pCreateInfos[index0].layout) {
                    local_pCreateInfos[index0].layout = layer_data->Unwrap(pCreateInfos[index0].layout);
                }
                if (pCreateInfos[index0].basePipelineHandle) {
                    local_pCreateInfos[index0].basePipelineHandle = layer_data->Unwrap(pCreateInfos[index0].basePipelineHandle);
                }
            }
        }
    }

    // For deferred pipeline creation, if handle wrapping is ON:
    // VVL will return wrapped handles when vkCreateRayTracingPipelinesKHR returns.
    // Even though the pipelines are not yet created, this is our only chance to return wrapped handles to the user
    // But when performing the deferred operation, if we do nothing the driver will read the pPipelines paramater,
    // thus will read wrapped handles
    // => we need to give the driver the list of unwrapped handles,
    // AND make sure this list has not been freed/reallocated before the driver is done.
    // Done with this shared unwrapped_pipelines pointer
    VkPipeline *returned_pipelines = pPipelines;
    std::shared_ptr<std::vector<VkPipeline>> unwrapped_pipelines;
    // Operation may be deffered, will know when looking at dispatch VkResult,
    // still we need to prepare
    if (deferredOperation != VK_NULL_HANDLE) {
        unwrapped_pipelines = std::make_shared<std::vector<VkPipeline>>(createInfoCount);
        returned_pipelines = unwrapped_pipelines->data();
    }

    VkResult result = layer_data->device_dispatch_table.CreateRayTracingPipelinesKHR(
        device, deferredOperation, pipelineCache, createInfoCount, (const VkRayTracingPipelineCreateInfoKHR *)local_pCreateInfos,
        pAllocator, returned_pipelines);

    for (uint32_t i = 0; i < createInfoCount; ++i) {
        if (deferredOperation != VK_NULL_HANDLE) {
            // Need to copy back returned pipeline handles in app provided array
            pPipelines[i] = unwrapped_pipelines->at(i);
        }
    }

    if (wrap_handles) {
        for (uint32_t i = 0; i < createInfoCount; i++) {
            if (pPipelines[i] != VK_NULL_HANDLE) {
                pPipelines[i] = layer_data->WrapNew(pPipelines[i]);
            }
        }

        for (uint32_t i = 0; i < createInfoCount; ++i) {
            if (pCreateInfos[i].pNext != VK_NULL_HANDLE) {
                CopyCreatePipelineFeedbackData(local_pCreateInfos[i].pNext, pCreateInfos[i].pNext);
            }
        }
    }

    // Fix check for deferred ray tracing pipeline creation
    // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/5817
    const bool is_operation_deferred = (deferredOperation != VK_NULL_HANDLE) && (result == VK_OPERATION_DEFERRED_KHR);
    if (is_operation_deferred) {
        std::vector<std::function<void()>> post_completion_fns;
        auto completion_find = layer_data->deferred_operation_post_completion.pop(deferredOperation);
        if (completion_find->first) {
            post_completion_fns = std::move(completion_find->second);
        }

        if (wrap_handles) {
            std::vector<VkPipeline> copied_wrapped_pipelines(createInfoCount);
            for (uint32_t i = 0; i < createInfoCount; ++i) {
                copied_wrapped_pipelines[i] = pPipelines[i];
            }
            auto cleanup_fn = [local_pCreateInfos, captured_copied_wrapped_pipelines = std::move(copied_wrapped_pipelines),
                               deferredOperation, layer_data, unwrapped_pipelines]() {
                (void)unwrapped_pipelines;
                if (local_pCreateInfos) {
                    delete[] local_pCreateInfos;
                }
                layer_data->deferred_operation_pipelines.insert(deferredOperation, std::move(captured_copied_wrapped_pipelines));
            };
            post_completion_fns.emplace_back(cleanup_fn);
        } else {
            auto cleanup_fn = [deferredOperation, layer_data, unwrapped_pipelines]() {
                layer_data->deferred_operation_pipelines.insert(deferredOperation, std::move(*unwrapped_pipelines));
            };
            post_completion_fns.emplace_back(cleanup_fn);
        }
        layer_data->deferred_operation_post_completion.insert(deferredOperation, std::move(post_completion_fns));
    }

    // If operation is deferred, local resources free is postponed
    if (!is_operation_deferred && wrap_handles) {
        if (local_pCreateInfos) {
            delete[] local_pCreateInfos;
        }
    }

    return result;
}

VkResult DispatchDeferredOperationJoinKHR(VkDevice device, VkDeferredOperationKHR operation) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (wrap_handles) {
        operation = layer_data->Unwrap(operation);
    }
    VkResult result = layer_data->device_dispatch_table.DeferredOperationJoinKHR(device, operation);

    // If this thread completed the operation, free any retained memory.
    if (result == VK_SUCCESS) {
        auto post_op_completion_fns = layer_data->deferred_operation_post_completion.pop(operation);
        if (post_op_completion_fns != layer_data->deferred_operation_post_completion.end()) {
            for (auto &post_op_completion_fn : post_op_completion_fns->second) {
                post_op_completion_fn();
            }
        }
    }

    return result;
}

VkResult DispatchGetDeferredOperationResultKHR(VkDevice device, VkDeferredOperationKHR operation) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (wrap_handles) {
        operation = layer_data->Unwrap(operation);
    }
    VkResult result = layer_data->device_dispatch_table.GetDeferredOperationResultKHR(device, operation);
    // Add created pipelines if successful
    if (result == VK_SUCCESS) {
        // Perfectly valid to never call vkDeferredOperationJoin before getting the result,
        // so we need to make sure functions associated to the current operation and
        // stored in deferred_operation_post_completion have been called
        auto post_op_completion_fns = layer_data->deferred_operation_post_completion.pop(operation);
        if (post_op_completion_fns != layer_data->deferred_operation_post_completion.end()) {
            for (auto &post_op__completion_fn : post_op_completion_fns->second) {
                post_op__completion_fn();
            }
        }

        auto post_check_fns = layer_data->deferred_operation_post_check.pop(operation);
        auto pipelines_to_updates = layer_data->deferred_operation_pipelines.pop(operation);
        if (post_check_fns->first && pipelines_to_updates->first) {
            for (auto &post_check_fn : post_check_fns->second) {
                post_check_fn(pipelines_to_updates->second);
            }
        }
    }

    return result;
}

VkResult DispatchBuildAccelerationStructuresKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, uint32_t infoCount,
                                                const VkAccelerationStructureBuildGeometryInfoKHR *pInfos,
                                                const VkAccelerationStructureBuildRangeInfoKHR *const *ppBuildRangeInfos) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles)
        return layer_data->device_dispatch_table.BuildAccelerationStructuresKHR(device, deferredOperation, infoCount, pInfos,
                                                                                ppBuildRangeInfos);
    vku::safe_VkAccelerationStructureBuildGeometryInfoKHR *local_pInfos = nullptr;
    {
        deferredOperation = layer_data->Unwrap(deferredOperation);
        if (pInfos) {
            local_pInfos = new vku::safe_VkAccelerationStructureBuildGeometryInfoKHR[infoCount];
            for (uint32_t index0 = 0; index0 < infoCount; ++index0) {
                local_pInfos[index0].initialize(&pInfos[index0], true, ppBuildRangeInfos[index0]);
                if (pInfos[index0].srcAccelerationStructure) {
                    local_pInfos[index0].srcAccelerationStructure = layer_data->Unwrap(pInfos[index0].srcAccelerationStructure);
                }
                if (pInfos[index0].dstAccelerationStructure) {
                    local_pInfos[index0].dstAccelerationStructure = layer_data->Unwrap(pInfos[index0].dstAccelerationStructure);
                }
                for (uint32_t geometry_index = 0; geometry_index < local_pInfos[index0].geometryCount; ++geometry_index) {
                    vku::safe_VkAccelerationStructureGeometryKHR &geometry_info =
                        local_pInfos[index0].pGeometries != nullptr ? local_pInfos[index0].pGeometries[geometry_index]
                                                                    : *(local_pInfos[index0].ppGeometries[geometry_index]);
                    if (geometry_info.geometryType == VK_GEOMETRY_TYPE_INSTANCES_KHR) {
                        if (geometry_info.geometry.instances.arrayOfPointers) {
                            const uint8_t *byte_ptr =
                                reinterpret_cast<const uint8_t *>(geometry_info.geometry.instances.data.hostAddress);
                            VkAccelerationStructureInstanceKHR **instances =
                                (VkAccelerationStructureInstanceKHR **)(byte_ptr +
                                                                        ppBuildRangeInfos[index0][geometry_index].primitiveOffset);
                            for (uint32_t instance_index = 0;
                                 instance_index < ppBuildRangeInfos[index0][geometry_index].primitiveCount; ++instance_index) {
                                instances[instance_index]->accelerationStructureReference =
                                    layer_data->Unwrap(instances[instance_index]->accelerationStructureReference);
                            }
                        } else {
                            const uint8_t *byte_ptr =
                                reinterpret_cast<const uint8_t *>(geometry_info.geometry.instances.data.hostAddress);
                            VkAccelerationStructureInstanceKHR *instances =
                                (VkAccelerationStructureInstanceKHR *)(byte_ptr +
                                                                       ppBuildRangeInfos[index0][geometry_index].primitiveOffset);
                            for (uint32_t instance_index = 0;
                                 instance_index < ppBuildRangeInfos[index0][geometry_index].primitiveCount; ++instance_index) {
                                instances[instance_index].accelerationStructureReference =
                                    layer_data->Unwrap(instances[instance_index].accelerationStructureReference);
                            }
                        }
                    }
                }
            }
        }
    }
    VkResult result = layer_data->device_dispatch_table.BuildAccelerationStructuresKHR(
        device, deferredOperation, infoCount, (const VkAccelerationStructureBuildGeometryInfoKHR *)local_pInfos, ppBuildRangeInfos);
    if (local_pInfos) {
        // Fix check for deferred ray tracing pipeline creation
        // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/5817
        const bool is_operation_deferred = (deferredOperation != VK_NULL_HANDLE) && (result == VK_OPERATION_DEFERRED_KHR);
        if (is_operation_deferred) {
            std::vector<std::function<void()>> cleanup{[local_pInfos]() { delete[] local_pInfos; }};
            layer_data->deferred_operation_post_completion.insert(deferredOperation, cleanup);
        } else {
            delete[] local_pInfos;
        }
    }
    return result;
}

void DispatchGetAccelerationStructureBuildSizesKHR(VkDevice device, VkAccelerationStructureBuildTypeKHR buildType,
                                                   const VkAccelerationStructureBuildGeometryInfoKHR *pBuildInfo,
                                                   const uint32_t *pMaxPrimitiveCounts,
                                                   VkAccelerationStructureBuildSizesInfoKHR *pSizeInfo) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles)
        return layer_data->device_dispatch_table.GetAccelerationStructureBuildSizesKHR(device, buildType, pBuildInfo,
                                                                                       pMaxPrimitiveCounts, pSizeInfo);
    vku::safe_VkAccelerationStructureBuildGeometryInfoKHR local_pBuildInfo;
    {
        if (pBuildInfo) {
            local_pBuildInfo.initialize(pBuildInfo, false, nullptr);
            if (pBuildInfo->srcAccelerationStructure) {
                local_pBuildInfo.srcAccelerationStructure = layer_data->Unwrap(pBuildInfo->srcAccelerationStructure);
            }
            if (pBuildInfo->dstAccelerationStructure) {
                local_pBuildInfo.dstAccelerationStructure = layer_data->Unwrap(pBuildInfo->dstAccelerationStructure);
            }
            for (uint32_t geometry_index = 0; geometry_index < local_pBuildInfo.geometryCount; ++geometry_index) {
                vku::safe_VkAccelerationStructureGeometryKHR &geometry_info =
                    local_pBuildInfo.pGeometries != nullptr ? local_pBuildInfo.pGeometries[geometry_index]
                                                            : *(local_pBuildInfo.ppGeometries[geometry_index]);
                if (geometry_info.geometryType == VK_GEOMETRY_TYPE_TRIANGLES_KHR) {
                    UnwrapPnextChainHandles(layer_data, geometry_info.geometry.triangles.pNext);
                }
            }
        }
    }
    layer_data->device_dispatch_table.GetAccelerationStructureBuildSizesKHR(
        device, buildType, (const VkAccelerationStructureBuildGeometryInfoKHR *)&local_pBuildInfo, pMaxPrimitiveCounts, pSizeInfo);
}

void DispatchGetDescriptorEXT(VkDevice device, const VkDescriptorGetInfoEXT *pDescriptorInfo, size_t dataSize, void *pDescriptor) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles) return layer_data->device_dispatch_table.GetDescriptorEXT(device, pDescriptorInfo, dataSize, pDescriptor);
    // When using a union of pointer we still need to unwrap the handles, but since it is a pointer, we can just use the pointer
    // from the incoming parameter instead of using safe structs as it is less complex doing it here
    vku::safe_VkDescriptorGetInfoEXT local_pDescriptorInfo;
    // TODO - Use safe struct once VUL is updated
    // There are no pNext for this function so nothing in short term will break
    // local_pDescriptorInfo.initialize(pDescriptorInfo);
    local_pDescriptorInfo.pNext = nullptr;
    local_pDescriptorInfo.sType = pDescriptorInfo->sType;
    local_pDescriptorInfo.type = pDescriptorInfo->type;

    // need in local scope to call down whatever we use
    VkSampler sampler;
    VkDescriptorImageInfo image_info;
    vku::safe_VkDescriptorAddressInfoEXT address_info;

    switch (pDescriptorInfo->type) {
        case VK_DESCRIPTOR_TYPE_SAMPLER: {
            // if using null descriptors can be null
            if (pDescriptorInfo->data.pSampler) {
                sampler = layer_data->Unwrap(*pDescriptorInfo->data.pSampler);
                local_pDescriptorInfo.data.pSampler = &sampler;
            }
            break;
        }
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
            if (pDescriptorInfo->data.pCombinedImageSampler) {
                image_info.sampler = layer_data->Unwrap(pDescriptorInfo->data.pCombinedImageSampler->sampler);
                image_info.imageView = layer_data->Unwrap(pDescriptorInfo->data.pCombinedImageSampler->imageView);
                image_info.imageLayout = pDescriptorInfo->data.pCombinedImageSampler->imageLayout;
                local_pDescriptorInfo.data.pCombinedImageSampler = &image_info;
            }
            break;
        }
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: {
            if (pDescriptorInfo->data.pSampledImage) {
                image_info.sampler = layer_data->Unwrap(pDescriptorInfo->data.pSampledImage->sampler);
                image_info.imageView = layer_data->Unwrap(pDescriptorInfo->data.pSampledImage->imageView);
                image_info.imageLayout = pDescriptorInfo->data.pSampledImage->imageLayout;
                local_pDescriptorInfo.data.pSampledImage = &image_info;
            }
            break;
        }
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
            if (pDescriptorInfo->data.pStorageImage) {
                image_info.sampler = layer_data->Unwrap(pDescriptorInfo->data.pStorageImage->sampler);
                image_info.imageView = layer_data->Unwrap(pDescriptorInfo->data.pStorageImage->imageView);
                image_info.imageLayout = pDescriptorInfo->data.pStorageImage->imageLayout;
                local_pDescriptorInfo.data.pStorageImage = &image_info;
            }
            break;
        }
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
            if (pDescriptorInfo->data.pInputAttachmentImage) {
                image_info.sampler = layer_data->Unwrap(pDescriptorInfo->data.pInputAttachmentImage->sampler);
                image_info.imageView = layer_data->Unwrap(pDescriptorInfo->data.pInputAttachmentImage->imageView);
                image_info.imageLayout = pDescriptorInfo->data.pInputAttachmentImage->imageLayout;
                local_pDescriptorInfo.data.pInputAttachmentImage = &image_info;
            }
            break;
        }
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            if (pDescriptorInfo->data.pUniformTexelBuffer) {
                address_info.initialize(pDescriptorInfo->data.pUniformTexelBuffer);
                local_pDescriptorInfo.data.pUniformTexelBuffer = address_info.ptr();
            }
            break;
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            if (pDescriptorInfo->data.pStorageTexelBuffer) {
                address_info.initialize(pDescriptorInfo->data.pStorageTexelBuffer);
                local_pDescriptorInfo.data.pStorageTexelBuffer = address_info.ptr();
            }
            break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            if (pDescriptorInfo->data.pUniformBuffer) {
                address_info.initialize(pDescriptorInfo->data.pUniformBuffer);
                local_pDescriptorInfo.data.pUniformBuffer = address_info.ptr();
            }
            break;
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            if (pDescriptorInfo->data.pStorageBuffer) {
                address_info.initialize(pDescriptorInfo->data.pStorageBuffer);
                local_pDescriptorInfo.data.pStorageBuffer = address_info.ptr();
            }
            break;
        case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
        case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV:
            local_pDescriptorInfo.data.accelerationStructure = pDescriptorInfo->data.accelerationStructure;
            break;
        default:
            break;
    }

    layer_data->device_dispatch_table.GetDescriptorEXT(device, (const VkDescriptorGetInfoEXT *)&local_pDescriptorInfo, dataSize,
                                                       pDescriptor);
}

VkResult DispatchCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
                                        const VkComputePipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator,
                                        VkPipeline *pPipelines) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles)
        return layer_data->device_dispatch_table.CreateComputePipelines(device, pipelineCache, createInfoCount, pCreateInfos,
                                                                        pAllocator, pPipelines);
    vku::safe_VkComputePipelineCreateInfo *local_pCreateInfos = nullptr;
    {
        pipelineCache = layer_data->Unwrap(pipelineCache);
        if (pCreateInfos) {
            local_pCreateInfos = new vku::safe_VkComputePipelineCreateInfo[createInfoCount];
            for (uint32_t index0 = 0; index0 < createInfoCount; ++index0) {
                local_pCreateInfos[index0].initialize(&pCreateInfos[index0]);
                UnwrapPnextChainHandles(layer_data, local_pCreateInfos[index0].pNext);
                if (pCreateInfos[index0].stage.module) {
                    local_pCreateInfos[index0].stage.module = layer_data->Unwrap(pCreateInfos[index0].stage.module);
                }
                UnwrapPnextChainHandles(layer_data, local_pCreateInfos[index0].stage.pNext);
                if (pCreateInfos[index0].layout) {
                    local_pCreateInfos[index0].layout = layer_data->Unwrap(pCreateInfos[index0].layout);
                }
                if (pCreateInfos[index0].basePipelineHandle) {
                    local_pCreateInfos[index0].basePipelineHandle = layer_data->Unwrap(pCreateInfos[index0].basePipelineHandle);
                }
            }
        }
    }
    VkResult result = layer_data->device_dispatch_table.CreateComputePipelines(
        device, pipelineCache, createInfoCount, (const VkComputePipelineCreateInfo *)local_pCreateInfos, pAllocator, pPipelines);
    for (uint32_t i = 0; i < createInfoCount; ++i) {
        if (pCreateInfos[i].pNext != VK_NULL_HANDLE) {
            CopyCreatePipelineFeedbackData(local_pCreateInfos[i].pNext, pCreateInfos[i].pNext);
        }
    }

    if (local_pCreateInfos) {
        delete[] local_pCreateInfos;
    }
    {
        for (uint32_t index0 = 0; index0 < createInfoCount; index0++) {
            if (pPipelines[index0] != VK_NULL_HANDLE) {
                pPipelines[index0] = layer_data->WrapNew(pPipelines[index0]);
            }
        }
    }
    return result;
}

VkResult DispatchCreateRayTracingPipelinesNV(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
                                             const VkRayTracingPipelineCreateInfoNV *pCreateInfos,
                                             const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles)
        return layer_data->device_dispatch_table.CreateRayTracingPipelinesNV(device, pipelineCache, createInfoCount, pCreateInfos,
                                                                             pAllocator, pPipelines);
    vku::safe_VkRayTracingPipelineCreateInfoNV *local_pCreateInfos = nullptr;
    {
        pipelineCache = layer_data->Unwrap(pipelineCache);
        if (pCreateInfos) {
            local_pCreateInfos = new vku::safe_VkRayTracingPipelineCreateInfoNV[createInfoCount];
            for (uint32_t index0 = 0; index0 < createInfoCount; ++index0) {
                local_pCreateInfos[index0].initialize(&pCreateInfos[index0]);
                if (local_pCreateInfos[index0].pStages) {
                    for (uint32_t index1 = 0; index1 < local_pCreateInfos[index0].stageCount; ++index1) {
                        if (pCreateInfos[index0].pStages[index1].module) {
                            local_pCreateInfos[index0].pStages[index1].module =
                                layer_data->Unwrap(pCreateInfos[index0].pStages[index1].module);
                        }
                    }
                }
                if (pCreateInfos[index0].layout) {
                    local_pCreateInfos[index0].layout = layer_data->Unwrap(pCreateInfos[index0].layout);
                }
                if (pCreateInfos[index0].basePipelineHandle) {
                    local_pCreateInfos[index0].basePipelineHandle = layer_data->Unwrap(pCreateInfos[index0].basePipelineHandle);
                }
            }
        }
    }
    VkResult result = layer_data->device_dispatch_table.CreateRayTracingPipelinesNV(
        device, pipelineCache, createInfoCount, (const VkRayTracingPipelineCreateInfoNV *)local_pCreateInfos, pAllocator,
        pPipelines);
    for (uint32_t i = 0; i < createInfoCount; ++i) {
        if (pCreateInfos[i].pNext != VK_NULL_HANDLE) {
            CopyCreatePipelineFeedbackData(local_pCreateInfos[i].pNext, pCreateInfos[i].pNext);
        }
    }

    if (local_pCreateInfos) {
        delete[] local_pCreateInfos;
    }
    {
        for (uint32_t index0 = 0; index0 < createInfoCount; index0++) {
            if (pPipelines[index0] != VK_NULL_HANDLE) {
                pPipelines[index0] = layer_data->WrapNew(pPipelines[index0]);
            }
        }
    }
    return result;
}

VkResult DispatchReleasePerformanceConfigurationINTEL(VkDevice device, VkPerformanceConfigurationINTEL configuration) {
    auto layer_data = GetLayerDataPtr(GetDispatchKey(device), layer_data_map);
    if (!wrap_handles) return layer_data->device_dispatch_table.ReleasePerformanceConfigurationINTEL(device, configuration);
    { configuration = layer_data->Unwrap(configuration); }
    VkResult result = layer_data->device_dispatch_table.ReleasePerformanceConfigurationINTEL(device, configuration);

    return result;
}
