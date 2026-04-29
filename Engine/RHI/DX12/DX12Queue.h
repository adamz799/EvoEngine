#pragma once

#include "RHI/RHIQueue.h"
#include "RHI/DX12/DX12Common.h"

namespace Evo {

class DX12Queue : public RHIQueue {
public:
    DX12Queue() = default;
    ~DX12Queue() override = default;

    bool Initialize(ID3D12Device* device, RHIQueueType type);
    void ShutdownQueue();

    void Submit(
        RHICommandList* const* cmdLists, uint32 cmdListCount,
        RHIFence* waitFence   = nullptr, uint64 waitValue   = 0,
        RHIFence* signalFence = nullptr, uint64 signalValue = 0
    ) override;

    void WaitIdle() override;
    RHIQueueType GetType() const override { return m_Type; }

    // Native accessor
    ID3D12CommandQueue* GetD3D12Queue() const { return m_pQueue.Get(); }

private:
    ComPtr<ID3D12CommandQueue> m_pQueue;
    RHIQueueType               m_Type = RHIQueueType::Graphics;

    // Internal fence for WaitIdle
    ID3D12Device*       m_pDevice         = nullptr;
    ComPtr<ID3D12Fence> m_pIdleFence;
    HANDLE              m_IdleEvent      = nullptr;
    uint64              m_uIdleFenceValue = 0;
};

} // namespace Evo
