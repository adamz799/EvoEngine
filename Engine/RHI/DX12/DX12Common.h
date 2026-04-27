#pragma once

// DX12 backend common headers and utilities
//
// d3d12.h comes from the Agility SDK (ThirdParty/AgilitySDK),
// whose include path is set with BEFORE priority in CMake,
// ensuring it overrides the Windows SDK version.

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

// d3dx12 helpers from the Agility SDK (barrier helpers, root signature helpers, etc.)
#include <d3dx12/d3dx12.h>

#include "Core/Log.h"

namespace Evo {

// ComPtr alias for convenience
template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

// Helper: check HRESULT and log on failure
inline bool DX12Check(HRESULT hr, const char* expr) {
    if (FAILED(hr)) {
        EVO_LOG_ERROR("DX12 call failed: {} (HRESULT 0x{:08X})", expr, static_cast<unsigned>(hr));
        return false;
    }
    return true;
}

#define DX12_CHECK(expr) ::Evo::DX12Check((expr), #expr)

} // namespace Evo
