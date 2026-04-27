#include "RHI/DX12/DX12Fence.h"
#include "Core/Log.h"

namespace Evo {

DX12Fence::~DX12Fence()
{
    ShutdownFence();
}

bool DX12Fence::Initialize(ID3D12Device* device, u64 initialValue)
{
    // TODO Phase 1:
    // device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
    // m_Event = CreateEventW(nullptr, FALSE, FALSE, nullptr);

    (void)device;
    (void)initialValue;
    EVO_LOG_INFO("DX12Fence::Initialize (stub)");
    return true;
}

void DX12Fence::ShutdownFence()
{
    if (m_Event) {
        CloseHandle(m_Event);
        m_Event = nullptr;
    }
    m_Fence.Reset();
}

u64 DX12Fence::GetCompletedValue()
{
    // TODO: return m_Fence->GetCompletedValue();
    return 0;
}

void DX12Fence::CpuWait(u64 value)
{
    // TODO:
    // if (m_Fence->GetCompletedValue() < value) {
    //     m_Fence->SetEventOnCompletion(value, m_Event);
    //     WaitForSingleObject(m_Event, INFINITE);
    // }
    (void)value;
}

void DX12Fence::CpuSignal(u64 value)
{
    // TODO: m_Fence->Signal(value);
    (void)value;
}

} // namespace Evo
