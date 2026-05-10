#pragma once

#include "RHI/DX12/DX12Common.h"
#include "Core/Log.h"
#include <atomic>
#include <vector>

namespace Evo {

/// Bitmap-based GPU descriptor heap.
///
/// Persistent region (majority): O(1) bitmap alloc/free, zero fragmentation, permanent GPU handles.
/// Ring region (tail): per-frame bump staging for descriptor table bindings.
///
/// Bind flow:
///   1. WriteDescriptorSet writes directly to persistent heap CPU handles.
///   2. SetDescriptorSet stages (CopyDescriptors from persistent → ring), caches per-set per-frame.
///   3. SetGraphicsRootDescriptorTable binds the ring's contiguous GPU address.
///
/// Evolution to bindless (SM 6.6):
///   Allocator + indices unchanged. Only shader + root signature change — zero migration.
class DX12GpuDescriptorHeap {
public:
	static constexpr uint32 kTotalCapacity      = 65536;  // 64K total
	static constexpr uint32 kRingDescriptorsPerFrame = 4096;
	static constexpr uint32 kRingTotal           = kRingDescriptorsPerFrame * NUM_BACK_FRAMES;
	static constexpr uint32 kPersistentCapacity  = kTotalCapacity - kRingTotal;

	// ---- Types ----

	struct Descriptor {
		uint32                      index     = 0;
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = {};
		bool IsValid() const { return cpuHandle.ptr != 0; }
	};

	struct RingAllocation {
		D3D12_GPU_DESCRIPTOR_HANDLE gpuBase = {};
		uint32                      count   = 0;
		bool IsValid() const { return gpuBase.ptr != 0 && count > 0; }
	};

	// ---- Lifecycle ----

	bool Initialize(ID3D12Device* device)
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = kTotalCapacity;
		desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc.NodeMask       = 0;

		HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_Heap));
		if (FAILED(hr))
		{
			EVO_LOG_ERROR("DX12GpuDescriptorHeap: failed to create heap: {}", GetHResultString(hr));
			return false;
		}

		m_CpuStart       = m_Heap->GetCPUDescriptorHandleForHeapStart().ptr;
		m_GpuStart       = m_Heap->GetGPUDescriptorHandleForHeapStart().ptr;
		m_DescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_RingStartIndex = kPersistentCapacity;  // ring lives at tail of heap

		for (uint32 i = 0; i < NUM_BACK_FRAMES; ++i)
			m_RingOffsets[i] = kRingDescriptorsPerFrame * i;

		memset(m_Bitmap, 0, sizeof(m_Bitmap));
		m_BitmapHint = 0;

		return true;
	}

	void Shutdown()
	{
		m_Heap.Reset();
		memset(m_Bitmap, 0, sizeof(m_Bitmap));
		m_BitmapHint = 0;
	}

	// ---- Frame ring buffer ----

	void BeginFrame()
	{
		m_CurrentFrame = (m_CurrentFrame + 1) % NUM_BACK_FRAMES;
		m_RingOffset.store(m_RingOffsets[m_CurrentFrame], std::memory_order_relaxed);
		m_CurrentFrameFence++;
	}

	uint64 GetCurrentFrameFence() const { return m_CurrentFrameFence; }

	// ---- Persistent descriptors ----

	Descriptor Allocate()
	{
		// Scan bitmap for a free bit, starting from hint
		for (uint32 w = m_BitmapHint; w < kBitmapWords; ++w)
		{
			uint64_t word = m_Bitmap[w];
			if (word == UINT64_MAX)
				continue;  // all used in this word

			uint32 bit = BitScanForward(~word);
			m_Bitmap[w] |= (uint64_t(1) << bit);
			m_BitmapHint = w;  // next scan starts here

			uint32 index = w * 64 + bit;
			return MakeDescriptor(index);
		}

		// Wrap around and scan from beginning
		for (uint32 w = 0; w < m_BitmapHint; ++w)
		{
			uint64_t word = m_Bitmap[w];
			if (word == UINT64_MAX)
				continue;

			uint32 bit = BitScanForward(~word);
			m_Bitmap[w] |= (uint64_t(1) << bit);
			m_BitmapHint = w;

			uint32 index = w * 64 + bit;
			return MakeDescriptor(index);
		}

		EVO_LOG_ERROR("DX12GpuDescriptorHeap: persistent region full (capacity={})", kPersistentCapacity);
		return {};
	}

	void Free(uint32 index)
	{
		if (index >= kPersistentCapacity)
			return;

		uint32 w = index / 64;
		uint32 b = index % 64;
		m_Bitmap[w] &= ~(uint64_t(1) << b);

		// Move hint back so this word gets priority for next alloc
		if (w < m_BitmapHint)
			m_BitmapHint = w;
	}

	/// Convenience for ImGui callbacks — derive index from CPU handle and free.
	void FreeByCpuHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
	{
		if (cpuHandle.ptr == 0 || m_DescriptorSize == 0)
			return;
		uint32 index = static_cast<uint32>((cpuHandle.ptr - m_CpuStart) / m_DescriptorSize);
		Free(index);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(uint32 index) const
	{
		return { m_CpuStart + static_cast<SIZE_T>(index) * m_DescriptorSize };
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(uint32 index) const
	{
		return { m_GpuStart + static_cast<UINT64>(index) * m_DescriptorSize };
	}

	// ---- Staging (SetDescriptorSet time) ----

	/// Stage a group of persistent descriptors to a contiguous ring region.
	/// Thread-safe via atomic bump allocation on the ring.
	RingAllocation Stage(const uint32* indices, uint32 count, ID3D12Device* device)
	{
		if (count == 0 || !indices)
			return {};

		uint32 offset = m_RingOffset.fetch_add(count, std::memory_order_relaxed);
		uint32 ringEnd = m_RingOffsets[m_CurrentFrame] + kRingDescriptorsPerFrame;
		if (offset + count > ringEnd)
		{
			EVO_LOG_ERROR("DX12GpuDescriptorHeap: ring overflow (need={}, avail={})",
			              count, ringEnd - offset);
			return {};
		}

		// Batch copy: destination is 1 contiguous range in ring,
		// sources are 'count' individual descriptors scattered in persistent heap.
		static constexpr uint32 kMaxBatch = 128;
		D3D12_CPU_DESCRIPTOR_HANDLE srcHandles[kMaxBatch];
		UINT                        srcSizes[kMaxBatch];

		uint32 remaining = count;
		uint32 copied = 0;

		while (remaining > 0)
		{
			uint32 batch = remaining < kMaxBatch ? remaining : kMaxBatch;
			for (uint32 i = 0; i < batch; ++i)
			{
				srcHandles[i] = GetCpuHandle(indices[copied + i]);
				srcSizes[i]   = 1;
			}

			UINT dstSize = batch;
			D3D12_CPU_DESCRIPTOR_HANDLE dst = GetCpuHandle(m_RingStartIndex + offset + copied);

			device->CopyDescriptors(
				1, &dst, &dstSize,           // 1 dest range of 'batch' contiguous descriptors
				batch, srcHandles, srcSizes,  // 'batch' source ranges, each size 1
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			copied += batch;
			remaining -= batch;
		}

		RingAllocation result;
		result.gpuBase = GetGpuHandle(m_RingStartIndex + offset);
		result.count   = count;
		return result;
	}

	// ---- Binding ----

	ID3D12DescriptorHeap* GetHeap() const { return m_Heap.Get(); }

private:
	Descriptor MakeDescriptor(uint32 index)
	{
		Descriptor d;
		d.index     = index;
		d.cpuHandle = GetCpuHandle(index);
		d.gpuHandle = GetGpuHandle(index);
		return d;
	}

#ifdef _MSC_VER
#include <intrin.h>
	static uint32 BitScanForward(uint64_t mask)
	{
		unsigned long idx;
		_BitScanForward64(&idx, mask);
		return static_cast<uint32>(idx);
	}
#else
	static uint32 BitScanForward(uint64_t mask)
	{
		return static_cast<uint32>(__builtin_ctzll(mask));
	}
#endif

	ComPtr<ID3D12DescriptorHeap> m_Heap;
	SIZE_T  m_CpuStart = 0;
	UINT64  m_GpuStart = 0;
	uint32  m_DescriptorSize = 0;

	// Persistent region
	static constexpr uint32 kBitmapWords = (kPersistentCapacity + 63) / 64;
	uint64_t m_Bitmap[kBitmapWords] = {};
	uint32   m_BitmapHint = 0;

	// Ring buffer
	uint32 m_RingStartIndex = 0;
	uint32 m_RingOffsets[NUM_BACK_FRAMES] = {};  // start offset within ring for each frame
	uint32 m_CurrentFrame = 0;
	std::atomic<uint32> m_RingOffset = 0;  // bump pointer within current frame's ring
	uint64 m_CurrentFrameFence = 0;        // incremented each BeginFrame, used for staging cache
};

} // namespace Evo
