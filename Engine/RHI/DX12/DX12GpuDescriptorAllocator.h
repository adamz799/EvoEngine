#pragma once

#include "RHI/DX12/DX12Common.h"
#include "Core/Log.h"
#include <vector>

namespace Evo {

/// Simple free-list allocator for GPU-visible (shader-visible) descriptor heaps.
/// Used for SRV/CBV/UAV descriptors that need to be accessible from shaders.
/// Returns both CPU and GPU handles for each allocation.
class DX12GpuDescriptorAllocator {
public:
	struct Allocation {
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = {};

		bool IsValid() const { return cpuHandle.ptr != 0; }
	};

	struct RangeAllocation {
		D3D12_CPU_DESCRIPTOR_HANDLE cpuStart = {};
		D3D12_GPU_DESCRIPTOR_HANDLE gpuStart = {};
		uint32 uCount     = 0;
		uint32 uBaseIndex = 0;

		bool IsValid() const { return uCount > 0 && cpuStart.ptr != 0; }
	};

	bool Initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 capacity)
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = capacity;
		desc.Type           = type;
		desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc.NodeMask       = 0;

		HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_pHeap));
		if (FAILED(hr))
		{
			EVO_LOG_ERROR("DX12GpuDescriptorAllocator: failed to create heap: {}",
						  GetHResultString(hr));
			return false;
		}

		m_uCpuStart       = m_pHeap->GetCPUDescriptorHandleForHeapStart().ptr;
		m_uGpuStart       = m_pHeap->GetGPUDescriptorHandleForHeapStart().ptr;
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

	Allocation Allocate()
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
				EVO_LOG_ERROR("DX12GpuDescriptorAllocator: heap full (capacity={})", m_uCapacity);
				return {};
			}
			index = m_uNextIndex++;
		}

		Allocation alloc;
		alloc.cpuHandle.ptr = m_uCpuStart + static_cast<SIZE_T>(index) * m_uDescriptorSize;
		alloc.gpuHandle.ptr = m_uGpuStart + static_cast<UINT64>(index) * m_uDescriptorSize;
		return alloc;
	}

	void Free(const Allocation& alloc)
	{
		if (alloc.cpuHandle.ptr == 0 || m_uDescriptorSize == 0)
			return;

		uint32 index = static_cast<uint32>((alloc.cpuHandle.ptr - m_uCpuStart) / m_uDescriptorSize);
		m_vFreeList.push_back(index);
	}

	/// Allocate a contiguous range of descriptors (bump-only, no free-list reuse).
	RangeAllocation AllocateRange(uint32 count)
	{
		if (count == 0 || m_uNextIndex + count > m_uCapacity)
		{
			EVO_LOG_ERROR("DX12GpuDescriptorAllocator: range alloc failed (need={}, avail={})",
						  count, m_uCapacity - m_uNextIndex);
			return {};
		}

		uint32 base = m_uNextIndex;
		m_uNextIndex += count;

		RangeAllocation alloc;
		alloc.cpuStart.ptr = m_uCpuStart + static_cast<SIZE_T>(base) * m_uDescriptorSize;
		alloc.gpuStart.ptr = m_uGpuStart + static_cast<UINT64>(base) * m_uDescriptorSize;
		alloc.uCount       = count;
		alloc.uBaseIndex   = base;
		return alloc;
	}

	/// Get CPU handle at offset within a range allocation.
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(const RangeAllocation& range, uint32 offset) const
	{
		return { range.cpuStart.ptr + static_cast<SIZE_T>(offset) * m_uDescriptorSize };
	}

	/// Get GPU handle at offset within a range allocation.
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(const RangeAllocation& range, uint32 offset) const
	{
		return { range.gpuStart.ptr + static_cast<UINT64>(offset) * m_uDescriptorSize };
	}

	ID3D12DescriptorHeap* GetHeap() const { return m_pHeap.Get(); }

private:
	ComPtr<ID3D12DescriptorHeap> m_pHeap;
	SIZE_T                       m_uCpuStart       = 0;
	UINT64                       m_uGpuStart       = 0;
	uint32                       m_uDescriptorSize = 0;
	uint32                       m_uCapacity       = 0;
	uint32                       m_uNextIndex      = 0;
	std::vector<uint32>          m_vFreeList;
};

} // namespace Evo

