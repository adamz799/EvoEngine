#include "RHI/DX12/DX12Queue.h"
#include "RHI/DX12/DX12Fence.h"
#include "RHI/DX12/DX12CommandList.h"
#include "RHI/DX12/DX12TypeMap.h"
#include "Core/Log.h"

namespace Evo {

bool DX12Queue::Initialize(ID3D12Device* device, RHIQueueType type)
{
    if (!device)
        return false;
    m_pDevice = device;
    m_Type = type;

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type     = MapQueueType(type);
    queueDesc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.NodeMask = 0;

    HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_Queue));
    if (FAILED(hr))
    {
        EVO_LOG_ERROR("Failed to create D3D12 command queue: {}", GetHResultString(hr));
        return false;
    }

    // Create internal fence for WaitIdle
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_IdleFence));
    if (FAILED(hr))
    {
        EVO_LOG_ERROR("Failed to create idle fence: {}", GetHResultString(hr));
        return false;
    }
    m_IdleEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    m_IdleFenceValue = 0;

    EVO_LOG_INFO("DX12Queue created (type={})", static_cast<int>(type));
    return true;
}

void DX12Queue::ShutdownQueue()
{
    if (m_IdleEvent) {
        CloseHandle(m_IdleEvent);
        m_IdleEvent = nullptr;
    }
    m_IdleFence.Reset();
    m_Queue.Reset();
    m_pDevice = nullptr;
}

void DX12Queue::Submit(
    RHICommandList* const* cmdLists, uint32 cmdListCount,
    RHIFence* waitFence, uint64 waitValue,
    RHIFence* signalFence, uint64 signalValue)
{
    if (cmdListCount == 0)
        return;

    std::vector<ID3D12CommandList*> dx12CmdLists(cmdListCount);
    for (uint32 i = 0; i < cmdListCount; ++i)
    {
        dx12CmdLists[i] = static_cast<DX12CommandList*>(cmdLists[i])->GetD3D12CommandList();
    }

    if (waitFence)
        m_Queue->Wait(static_cast<DX12Fence*>(waitFence)->GetD3D12Fence(), waitValue);

    m_Queue->ExecuteCommandLists(cmdListCount, dx12CmdLists.data());

    if (signalFence)
        m_Queue->Signal(static_cast<DX12Fence*>(signalFence)->GetD3D12Fence(), signalValue);
}

void DX12Queue::WaitIdle()
{
    if (!m_Queue || !m_IdleFence)
        return;

    m_IdleFenceValue++;
    m_Queue->Signal(m_IdleFence.Get(), m_IdleFenceValue);

    if (m_IdleFence->GetCompletedValue() < m_IdleFenceValue)
    {
        m_IdleFence->SetEventOnCompletion(m_IdleFenceValue, m_IdleEvent);
        WaitForSingleObject(m_IdleEvent, INFINITE);
    }
}

} // namespace Evo
