#include "RHI/DX12/DX12CommandListPool.h"
#include "RHI/DX12/DX12CommandList.h"
#include "RHI/DX12/DX12Device.h"
#include "RHI/DX12/DX12TypeMap.h"
#include "Core/Log.h"

namespace Evo {

void DX12CommandListPool::Initialize(DX12Device* pDevice, D3D12_COMMAND_LIST_TYPE type)
{
	m_pDevice = pDevice;
	m_Type    = type;
}

void DX12CommandListPool::Shutdown()
{
	std::lock_guard lock(m_Mutex);
	for (auto& entry : m_vEntries)
	{
		if (entry->pCmdList)
			entry->pCmdList->ShutdownCommandList();
	}
	m_vEntries.clear();
}

RHICommandList* DX12CommandListPool::Acquire()
{
	std::lock_guard lock(m_Mutex);

	// Try to reuse a free entry
	for (auto& entry : m_vEntries)
	{
		if (entry->state == EntryState::Free)
		{
			entry->state = EntryState::InUse;
			entry->pCmdList->Begin();
			return entry->pCmdList.get();
		}
	}

	// No free entry — create new (Allocator + CmdList) pair
	auto* pD3DDevice = m_pDevice->GetD3D12Device();

	auto newEntry = std::make_unique<Entry>();
	HRESULT hr = pD3DDevice->CreateCommandAllocator(m_Type, IID_PPV_ARGS(&newEntry->pAllocator));
	if (FAILED(hr))
	{
		EVO_LOG_ERROR("DX12CommandListPool: failed to create allocator: {}", GetHResultString(hr));
		return nullptr;
	}

	// Map D3D12 type to RHI type
	RHIQueueType queueType = RHIQueueType::Graphics;
	if (m_Type == D3D12_COMMAND_LIST_TYPE_COMPUTE) queueType = RHIQueueType::Compute;
	else if (m_Type == D3D12_COMMAND_LIST_TYPE_COPY) queueType = RHIQueueType::Copy;

	newEntry->pCmdList = std::make_unique<DX12CommandList>();
	if (!newEntry->pCmdList->InitializePooled(m_pDevice, queueType, newEntry->pAllocator.Get()))
	{
		EVO_LOG_ERROR("DX12CommandListPool: failed to create pooled command list");
		return nullptr;
	}

	newEntry->state = EntryState::InUse;
	newEntry->pCmdList->Begin();

	auto* pResult = newEntry->pCmdList.get();
	m_vEntries.push_back(std::move(newEntry));

	EVO_LOG_INFO("DX12CommandListPool: grew to {} entries", m_vEntries.size());
	return pResult;
}

void DX12CommandListPool::BeginFrame(uint64 uCompletedFenceValue)
{
	std::lock_guard lock(m_Mutex);
	for (auto& entry : m_vEntries)
	{
		if (entry->state == EntryState::InFlight &&
			entry->uFenceValue <= uCompletedFenceValue)
		{
			entry->state       = EntryState::Free;
			entry->uFenceValue = 0;
		}
	}
}

void DX12CommandListPool::EndFrame(uint64 uFrameFenceValue)
{
	std::lock_guard lock(m_Mutex);
	for (auto& entry : m_vEntries)
	{
		if (entry->state == EntryState::InUse)
		{
			entry->uFenceValue = uFrameFenceValue;
			entry->state       = EntryState::InFlight;
		}
	}
}

} // namespace Evo

