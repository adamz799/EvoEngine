#pragma once

#include "RHI/RHIFence.h"
#include "RHI/DX12/DX12Common.h"

namespace Evo {

class DX12Fence : public RHIFence {
public:
    DX12Fence() = default;
    ~DX12Fence() override;

    bool Initialize(ID3D12Device* device, u64 initialValue);
    void ShutdownFence();

    u64  GetCompletedValue() override;
    void CpuWait(u64 value) override;
    void CpuSignal(u64 value) override;

    // Native accessor
    ID3D12Fence* GetD3D12Fence() const { return m_Fence.Get(); }

private:
    ComPtr<ID3D12Fence> m_Fence;
    HANDLE              m_Event = nullptr;
};

} // namespace Evo
