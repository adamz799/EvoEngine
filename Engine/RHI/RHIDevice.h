#pragma once

#include "RHI/RHITypes.h"
#include "RHI/RHIDescriptor.h"
#include "RHI/RHIQueue.h"
#include "RHI/RHIFence.h"
#include "RHI/RHISwapChain.h"
#include "RHI/RHICommandList.h"
#include <memory>
#include <string>

namespace Evo {

/// GPU device — the central factory and resource management hub.
///
/// Creates all RHI objects. Handle-managed resources (Buffer, Texture, Shader,
/// Pipeline, DescriptorSetLayout, DescriptorSet) support deferred destruction:
/// DestroyXxx() enqueues the handle for deletion once the GPU finishes using it.
///
/// DX12:   ID3D12Device
/// Vulkan: VkDevice + VkPhysicalDevice + VkInstance
class RHIDevice {
public:
    virtual ~RHIDevice() = default;

    virtual bool Initialize(const RHIDeviceDesc& desc) = 0;
    virtual void Shutdown() = 0;

    // ---- Info ----
    virtual RHIBackendType     GetBackendType() const = 0;
    virtual const std::string& GetAdapterName() const = 0;

    // ---- Queue access ----
    // Device creates and owns queues internally; callers get non-owning pointers.
    virtual RHIQueue* GetGraphicsQueue() = 0;
    virtual RHIQueue* GetComputeQueue()  = 0;   // may return nullptr
    virtual RHIQueue* GetCopyQueue()     = 0;   // may return nullptr

    // ---- Direct object creation (unique_ptr ownership) ----
    virtual std::unique_ptr<RHISwapChain>   CreateSwapChain(const RHISwapChainDesc& desc) = 0;
    virtual std::unique_ptr<RHICommandList> CreateCommandList(RHIQueueType type) = 0;
    virtual std::unique_ptr<RHIFence>       CreateFence(uint64 initialValue = 0) = 0;

    // ---- Handle-managed resource creation ----
    virtual RHIBufferHandle   CreateBuffer(const RHIBufferDesc& desc) = 0;
    virtual RHITextureHandle  CreateTexture(const RHITextureDesc& desc) = 0;
    virtual RHIShaderHandle   CreateShader(const RHIShaderDesc& desc) = 0;
    virtual RHIPipelineHandle CreateGraphicsPipeline(const RHIGraphicsPipelineDesc& desc) = 0;
    virtual RHIPipelineHandle CreateComputePipeline(const RHIComputePipelineDesc& desc) = 0;

    // ---- Handle-managed resource destruction (deferred) ----
    virtual void DestroyBuffer(RHIBufferHandle handle) = 0;
    virtual void DestroyTexture(RHITextureHandle handle) = 0;
    virtual void DestroyShader(RHIShaderHandle handle) = 0;
    virtual void DestroyPipeline(RHIPipelineHandle handle) = 0;

    // ---- View creation / destruction ----
    virtual RHIRenderTargetView CreateRenderTargetView(RHITextureHandle texture) = 0;
    virtual void                DestroyRenderTargetView(RHIRenderTargetView rtv) = 0;

    virtual RHIDepthStencilView CreateDepthStencilView(RHITextureHandle texture) = 0;
    virtual void                DestroyDepthStencilView(RHIDepthStencilView dsv) = 0;

    // ---- Buffer operations ----
    virtual void* MapBuffer(RHIBufferHandle handle) = 0;
    virtual void  UnmapBuffer(RHIBufferHandle handle) = 0;

    // ---- Descriptor management ----
    virtual RHIDescriptorSetLayoutHandle CreateDescriptorSetLayout(
        const RHIDescriptorSetLayoutDesc& desc) = 0;
    virtual void DestroyDescriptorSetLayout(RHIDescriptorSetLayoutHandle handle) = 0;

    virtual RHIDescriptorSetHandle AllocateDescriptorSet(
        RHIDescriptorSetLayoutHandle layout) = 0;
    virtual void FreeDescriptorSet(RHIDescriptorSetHandle handle) = 0;
    virtual void WriteDescriptorSet(RHIDescriptorSetHandle set,
        const RHIDescriptorWrite* writes, uint32 writeCount) = 0;

    // ---- CommandList pool ----
    /// Acquire a command list from the pool, ready for recording (already Begin'd).
    virtual RHICommandList* AcquireCommandList(RHIQueueType type = RHIQueueType::Graphics) = 0;

    // ---- Frame management ----
    /// Recycle pool entries whose GPU work has completed.
    virtual void BeginFrame(uint64 uCompletedFenceValue) = 0;
    /// Mark acquired pool entries as in-flight.
    virtual void EndFrame(uint64 uFrameFenceValue) = 0;

    // ---- Global sync ----
    virtual void WaitIdle() = 0;

    // ---- Descriptor heap binding ----
    /// Bind the GPU-visible descriptor heap on a command list (required before SetDescriptorSet).
    virtual void BindGpuDescriptorHeap(RHICommandList* /*pCmdList*/) {}
};

/// Factory: create an RHI device for the specified backend.
std::unique_ptr<RHIDevice> CreateRHIDevice(RHIBackendType backend);

} // namespace Evo
