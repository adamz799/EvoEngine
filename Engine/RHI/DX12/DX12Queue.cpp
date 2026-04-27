#include "RHI/DX12/DX12Queue.h"
#include "RHI/DX12/DX12Fence.h"
#include "RHI/RHICommandList.h"
#include "Core/Log.h"

namespace Evo {

bool DX12Queue::Initialize(ID3D12Device* device, RHIQueueType type)
{
    m_Type = type;

    // TODO Phase 1:
    // D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    // queueDesc.Type = (type == Graphics) ? DIRECT : (type == Compute) ? COMPUTE : COPY;
    // device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_Queue));

    (void)device;
    EVO_LOG_INFO("DX12Queue::Initialize (stub)");
    return true;
}

void DX12Queue::ShutdownQueue()
{
    m_Queue.Reset();
}

void DX12Queue::Submit(
    RHICommandList* const* /*cmdLists*/, u32 /*cmdListCount*/,
    RHIFence* /*waitFence*/, u64 /*waitValue*/,
    RHIFence* /*signalFence*/, u64 /*signalValue*/)
{
    // TODO Phase 1:
    // 1. If waitFence: m_Queue->Wait(dx12Fence, waitValue)
    // 2. Collect ID3D12CommandList* from each RHICommandList
    // 3. m_Queue->ExecuteCommandLists(count, cmdLists)
    // 4. If signalFence: m_Queue->Signal(dx12Fence, signalValue)
}

void DX12Queue::WaitIdle()
{
    // TODO Phase 1: create temp fence, signal, wait
}

} // namespace Evo
