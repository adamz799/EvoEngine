#pragma once

#include "RHI/RHITypes.h"
#include "RHI/RHIResourcePool.h"
#include "RHI/DX12/DX12Common.h"
#include <vector>
#include <string>
#include "D3D12MemAlloc.h"

namespace Evo
{
	struct DX12BufferEntry
	{
		ComPtr<ID3D12Resource>    pResource;
		D3D12MA::Allocation*      pAllocation = nullptr;
		D3D12_GPU_VIRTUAL_ADDRESS uGpuAddress = 0;
		uint64                    uSize = 0;
		void*                     pMappedPtr = nullptr;
		std::string               sDebugName;
		RHIBarrierState           barrierState;
		uint16                    uGeneration = 0;
		bool                      bAlive = false;

		DX12BufferEntry() = default;

		DX12BufferEntry(DX12BufferEntry&& other) noexcept
			: pResource(std::move(other.pResource))
			, pAllocation(other.pAllocation)
			, uGpuAddress(other.uGpuAddress)
			, uSize(other.uSize)
			, pMappedPtr(other.pMappedPtr)
			, sDebugName(std::move(other.sDebugName))
			, barrierState(other.barrierState)
			, uGeneration(other.uGeneration)
			, bAlive(other.bAlive)
		{
			other.pAllocation = nullptr;
			other.pMappedPtr = nullptr;
			other.uGpuAddress = 0;
			other.uSize = 0;
		}

		DX12BufferEntry& operator=(DX12BufferEntry&& other) noexcept
		{
			if (this != &other)
			{
				ReleaseBackendResources();
				pResource   = std::move(other.pResource);
				pAllocation = other.pAllocation;
				uGpuAddress = other.uGpuAddress;
				uSize       = other.uSize;
				pMappedPtr  = other.pMappedPtr;
				sDebugName  = std::move(other.sDebugName);
				barrierState = other.barrierState;
				uGeneration = other.uGeneration;
				bAlive      = other.bAlive;

				other.pAllocation = nullptr;
				other.pMappedPtr  = nullptr;
				other.uGpuAddress = 0;
				other.uSize       = 0;
			}
			return *this;
		}

		~DX12BufferEntry()
		{
			ReleaseBackendResources();
		}

		void ReleaseBackendResources()
		{
			SAFE_RELEASE(pAllocation);
			pResource.Reset();
			uGpuAddress = 0;
			uSize = 0;
			pMappedPtr = nullptr;
		}
	};

	using DX12BufferPool = RHIResourcePool<struct BufferTag, DX12BufferEntry>;

	/// One slot in the texture pool.
	struct DX12TextureEntry
	{
		ComPtr<ID3D12Resource> pResource;
		std::string            sDebugName;
		uint16                 uGeneration = 0;
		bool                   bAlive = false;
		RHIBarrierState        barrierState;
		RHIFormat              rhiFormat = RHIFormat::Unknown;

		~DX12TextureEntry()
		{
			ReleaseBackendResources();
		}

		void ReleaseBackendResources()
		{
			pResource.Reset();
		}
	};

	using DX12TexturePool = RHIResourcePool<struct TextureTag, DX12TextureEntry>;

} // namespace Evo
