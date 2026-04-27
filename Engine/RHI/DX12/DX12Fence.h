#pragma once

#include "RHI/RHIFence.h"
#include "RHI/DX12/DX12Common.h"

namespace Evo {

class DX12Fence : public RHIFence {
public:
    DX12Fence() = default;
    ~DX12Fence() override;

    bool Initialize(ID3D12Device* device, uint64 initialValue = 0);
    void ShutdownFence();

    uint64  GetCompletedValue() override;
    void CpuWait(uint64 value) override;
    void CpuSignal(uint64 value) override;

    // Native accessor
    ID3D12Fence* GetD3D12Fence() const { return m_Fence.Get(); }

private:
    ComPtr<ID3D12Fence> m_Fence;
    HANDLE              m_Event = nullptr;
};

} // namespace Evo
