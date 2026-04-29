#pragma once

#include "RHI/DX12/DX12Common.h"
#include "RHI/DX12/DX12CommandList.h"
#include "RHI/RHITypes.h"
#include <vector>
#include <mutex>
#include <memory>

namespace Evo {

class DX12Device;
class RHICommandList;

/// Pool of (Allocator + CommandList) pairs.
///
/// Each entry permanently binds one Allocator to one CommandList.
/// Lifecycle:
///   BeginFrame(completedFenceValue) — recycle entries whose GPU work is done
///   Acquire()                       — get a ready-to-record CmdList (already Begin'd)
///   ...record...End()
///   EndFrame(frameFenceValue)       — mark all acquired entries as in-flight
///
/// Thread-safe: Acquire() can be called from multiple threads.
class DX12CommandListPool {
public:
	void Initialize(DX12Device* pDevice, D3D12_COMMAND_LIST_TYPE type);
	void Shutdown();

	/// Get a command list ready for recording (already Begin'd). Thread-safe.
	RHICommandList* Acquire();

	/// Recycle entries whose GPU work has completed.
	void BeginFrame(uint64 uCompletedFenceValue);

	/// Mark all acquired entries as in-flight with the given fence value.
	void EndFrame(uint64 uFrameFenceValue);

private:
	enum class EntryState : uint8_t { Free, InUse, InFlight };

	struct Entry {
		ComPtr<ID3D12CommandAllocator>    pAllocator;
		std::unique_ptr<DX12CommandList>  pCmdList;
		uint64     uFenceValue = 0;
		EntryState state       = EntryState::Free;
	};

	DX12Device*             m_pDevice = nullptr;
	D3D12_COMMAND_LIST_TYPE m_Type    = D3D12_COMMAND_LIST_TYPE_DIRECT;
	std::mutex              m_Mutex;
	std::vector<std::unique_ptr<Entry>> m_vEntries;
};

} // namespace Evo
