#include "RHI/DX12/DX12Fence.h"
#include "Core/Log.h"

namespace Evo {

DX12Fence::~DX12Fence()
{
	ShutdownFence();
}

bool DX12Fence::Initialize(ID3D12Device* device, uint64 initialValue)
{
	if (!device)
		return false;

	HRESULT hr = device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence));
	if (FAILED(hr))
	{
		EVO_LOG_ERROR("Failed to create D3D12 fence: {}", GetHResultString(hr));
		return false;
	}
	m_Event = CreateEventW(nullptr, FALSE, FALSE, nullptr);

	return true;
}

void DX12Fence::ShutdownFence()
{
	if (m_Event) {
		CloseHandle(m_Event);
		m_Event = nullptr;
	}
	m_pFence.Reset();
}

uint64 DX12Fence::GetCompletedValue()
{
	return m_pFence->GetCompletedValue();
}

void DX12Fence::CpuWait(uint64 value)
{
	if (m_pFence->GetCompletedValue() < value)
	{
		m_pFence->SetEventOnCompletion(value, m_Event);
		WaitForSingleObject(m_Event, INFINITE);
	}
}

void DX12Fence::CpuSignal(uint64 value)
{
	m_pFence->Signal(value);
}

} // namespace Evo
