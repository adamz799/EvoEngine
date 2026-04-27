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
    ID3D12CommandQueue* GetD3D12Queue() const { return m_Queue.Get(); }

private:
    ComPtr<ID3D12CommandQueue> m_Queue;
    RHIQueueType               m_Type = RHIQueueType::Graphics;
};

} // namespace Evo
