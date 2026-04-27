#include "RHI/DX12/DX12CommandListManager.h"
#include "Core/Log.h"

namespace Evo {

bool DX12CommandListManager::Initialize(ID3D12Device* device, uint32 /*framesInFlight*/)
{
    if (!device)
        return false;
    m_Device = device;
    return true;
}

void DX12CommandListManager::Shutdown()
{
    for (auto& pool : m_Pools) {
        pool.available.clear();
        while (!pool.inFlight.empty()) pool.inFlight.pop();
        while (!pool.freeLists.empty()) pool.freeLists.pop();
    }
    m_FrameAllocators.clear();
    m_Device = nullptr;
}

void DX12CommandListManager::BeginFrame(uint64 completedFenceValue)
{
    // Move completed in-flight allocators back to available
    for (auto& pool : m_Pools) {
        while (!pool.inFlight.empty()) {
            auto& front = pool.inFlight.front();
            if (front.fenceValue > completedFenceValue)
                break;  // This and all subsequent are still in-flight

            // GPU is done with this allocator — reset and move to available
            front.allocator->Reset();
            front.fenceValue = 0;
            pool.available.push_back(std::move(front));
            pool.inFlight.pop();
        }
    }

    m_FrameAllocators.clear();
}

void DX12CommandListManager::EndFrame(uint64 frameFenceValue)
{
    // Tag all allocators used this frame with the fence value,
    // then move them to in-flight
    for (auto& [type, entry] : m_FrameAllocators) {
        uint32 idx = TypeIndex(type);
        entry->fenceValue = frameFenceValue;
        m_Pools[idx].inFlight.push(std::move(*entry));
    }

    // Remove moved entries from available lists
    // (they were moved-from, clean up)
    for (auto& pool : m_Pools) {
        pool.available.erase(
            std::remove_if(pool.available.begin(), pool.available.end(),
                [](const AllocatorEntry& e) { return !e.allocator; }),
            pool.available.end());
    }

    m_FrameAllocators.clear();
}

ID3D12GraphicsCommandList* DX12CommandListManager::Allocate(D3D12_COMMAND_LIST_TYPE type)
{
    AllocatorEntry* entry = AcquireAllocator(type);
    if (!entry)
        return nullptr;

    uint32 idx = TypeIndex(type);
    auto& pool = m_Pools[idx];

    m_LastAllocator = entry->allocator.Get();

    // Try to reuse a recycled command list
    if (!pool.freeLists.empty()) {
        auto cmdList = std::move(pool.freeLists.front());
        pool.freeLists.pop();

        HRESULT hr = cmdList->Reset(entry->allocator.Get(), nullptr);
        if (FAILED(hr)) {
            EVO_LOG_ERROR("Failed to reset command list: {}", GetHResultString(hr));
            return nullptr;
        }

        // Track the allocator for EndFrame tagging
        m_FrameAllocators.push_back({ type, entry });

        // Put back into free list on return (caller will End() then we recycle)
        // Actually, we need to give it back — store the raw ptr and put ComPtr back in free list later
        // For now, we leak the ComPtr out. The caller should return it via the command list wrapper.
        ID3D12GraphicsCommandList* raw = cmdList.Get();
        pool.freeLists.push(std::move(cmdList));
        return raw;
    }

    // Create a new command list
    ComPtr<ID3D12GraphicsCommandList> cmdList;
    HRESULT hr = m_Device->CreateCommandList(
        0, type, entry->allocator.Get(), nullptr,
        IID_PPV_ARGS(&cmdList));
    if (FAILED(hr)) {
        EVO_LOG_ERROR("Failed to create command list: {}", GetHResultString(hr));
        return nullptr;
    }

    m_FrameAllocators.push_back({ type, entry });

    ID3D12GraphicsCommandList* raw = cmdList.Get();
    pool.freeLists.push(std::move(cmdList));
    return raw;
}

ComPtr<ID3D12CommandAllocator> DX12CommandListManager::CreateAllocator(D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12CommandAllocator> allocator;
    HRESULT hr = m_Device->CreateCommandAllocator(type, IID_PPV_ARGS(&allocator));
    if (FAILED(hr)) {
        EVO_LOG_ERROR("Failed to create command allocator: {}", GetHResultString(hr));
        return nullptr;
    }
    return allocator;
}

DX12CommandListManager::AllocatorEntry* DX12CommandListManager::AcquireAllocator(D3D12_COMMAND_LIST_TYPE type)
{
    uint32 idx = TypeIndex(type);
    auto& pool = m_Pools[idx];

    // Reuse an available (already reset) allocator
    if (!pool.available.empty()) {
        return &pool.available.back();
    }

    // None available — create a new one
    auto allocator = CreateAllocator(type);
    if (!allocator)
        return nullptr;

    pool.available.push_back({ std::move(allocator), 0 });
    return &pool.available.back();
}

uint32 DX12CommandListManager::TypeIndex(D3D12_COMMAND_LIST_TYPE type)
{
    switch (type) {
    case D3D12_COMMAND_LIST_TYPE_DIRECT:  return 0;
    case D3D12_COMMAND_LIST_TYPE_BUNDLE:  return 1;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE: return 2;
    case D3D12_COMMAND_LIST_TYPE_COPY:    return 3;
    default: return 0;
    }
}

} // namespace Evo
