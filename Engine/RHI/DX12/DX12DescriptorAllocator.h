#pragma once

#include "RHI/DX12/DX12Common.h"
#include "Core/Log.h"
#include <vector>

namespace Evo {

/// Simple free-list allocator for CPU-visible descriptor heaps.
/// Used for RTV, DSV, and other non-shader-visible heaps.
class DX12CpuDescriptorAllocator {
public:
	bool Initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 capacity)
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = capacity;
		desc.Type           = type;
		desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NodeMask       = 0;

		HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_pHeap));
		if (FAILED(hr))
		{
			EVO_LOG_ERROR("DX12CpuDescriptorAllocator: failed to create heap: {}",
						  GetHResultString(hr));
			return false;
		}

		m_uHeapStart      = m_pHeap->GetCPUDescriptorHandleForHeapStart().ptr;
		m_uDescriptorSize = device->GetDescriptorHandleIncrementSize(type);
		m_uCapacity       = capacity;
		m_uNextIndex      = 0;

		return true;
	}

	void Shutdown()
	{
		m_pHeap.Reset();
		m_vFreeList.clear();
		m_uNextIndex = 0;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Allocate()
	{
		uint32 index;
		if (!m_vFreeList.empty())
		{
			index = m_vFreeList.back();
			m_vFreeList.pop_back();
		}
		else
		{
			if (m_uNextIndex >= m_uCapacity)
			{
				EVO_LOG_ERROR("DX12CpuDescriptorAllocator: heap full (capacity={})", m_uCapacity);
				return { 0 };
			}
			index = m_uNextIndex++;
		}

		return { m_uHeapStart + static_cast<SIZE_T>(index) * m_uDescriptorSize };
	}

	void Free(D3D12_CPU_DESCRIPTOR_HANDLE handle)
	{
		if (handle.ptr == 0 || m_uDescriptorSize == 0)
			return;

		uint32 index = static_cast<uint32>((handle.ptr - m_uHeapStart) / m_uDescriptorSize);
		m_vFreeList.push_back(index);
	}

private:
	ComPtr<ID3D12DescriptorHeap> m_pHeap;
	SIZE_T                       m_uHeapStart      = 0;
	uint32                       m_uDescriptorSize = 0;
	uint32                       m_uCapacity       = 0;
	uint32                       m_uNextIndex      = 0;
	std::vector<uint32>          m_vFreeList;
};

} // namespace Evo

