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

#include <string>

#define NUM_BACK_FRAMES (3)

namespace Evo {

// Convert HRESULT to a human-readable string via FormatMessage.
// Falls back to hex code if no system message is available.
inline std::string GetHResultString(HRESULT hr)
{
    char* msgBuf = nullptr;
    DWORD len = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        static_cast<DWORD>(hr),
        MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&msgBuf),
        0,
        nullptr);

    std::string result;
    if (len > 0 && msgBuf) {
        // Trim trailing \r\n
        while (len > 0 && (msgBuf[len - 1] == '\n' || msgBuf[len - 1] == '\r'))
            --len;
        result.assign(msgBuf, len);
        LocalFree(msgBuf);
    } else {
        char buf[32];
        snprintf(buf, sizeof(buf), "0x%08X", static_cast<unsigned>(hr));
        result = buf;
    }
    return result;
}

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
