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

		HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_Heap));
		if (FAILED(hr))
		{
			EVO_LOG_ERROR("DX12CpuDescriptorAllocator: failed to create heap: {}",
			              GetHResultString(hr));
			return false;
		}

		m_HeapStart      = m_Heap->GetCPUDescriptorHandleForHeapStart().ptr;
		m_DescriptorSize = device->GetDescriptorHandleIncrementSize(type);
		m_Capacity       = capacity;
		m_NextIndex      = 0;

		return true;
	}

	void Shutdown()
	{
		m_Heap.Reset();
		m_FreeList.clear();
		m_NextIndex = 0;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Allocate()
	{
		uint32 index;
		if (!m_FreeList.empty())
		{
			index = m_FreeList.back();
			m_FreeList.pop_back();
		}
		else
		{
			if (m_NextIndex >= m_Capacity)
			{
				EVO_LOG_ERROR("DX12CpuDescriptorAllocator: heap full (capacity={})", m_Capacity);
				return { 0 };
			}
			index = m_NextIndex++;
		}

		return { m_HeapStart + static_cast<SIZE_T>(index) * m_DescriptorSize };
	}

	void Free(D3D12_CPU_DESCRIPTOR_HANDLE handle)
	{
		if (handle.ptr == 0 || m_DescriptorSize == 0)
			return;

		uint32 index = static_cast<uint32>((handle.ptr - m_HeapStart) / m_DescriptorSize);
		m_FreeList.push_back(index);
	}

private:
	ComPtr<ID3D12DescriptorHeap> m_Heap;
	SIZE_T                       m_HeapStart      = 0;
	uint32                       m_DescriptorSize = 0;
	uint32                       m_Capacity       = 0;
	uint32                       m_NextIndex      = 0;
	std::vector<uint32>          m_FreeList;
};

} // namespace Evo
