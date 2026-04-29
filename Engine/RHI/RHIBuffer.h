#pragma once

#include "RHI/RHITypes.h"

namespace Evo {

class RHIDevice;

/// Thin wrapper around RHIBufferHandle for Renderer-level ergonomics.
///
/// Holds the Handle (for Device operations) and the creation descriptor
/// (so callers can query size, usage, etc. without going through Device).
/// Does NOT own the underlying GPU resource — the Device's Handle pool does.
/// Destruction must go through Device::DestroyBuffer(GetHandle()).
class RHIBuffer {
public:
    RHIBuffer() = default;
    RHIBuffer(RHIBufferHandle handle, const RHIBufferDesc& desc)
        : m_Handle(handle), m_Desc(desc) {}

    RHIBufferHandle      GetHandle() const { return m_Handle; }
    bool                 IsValid()   const { return m_Handle.IsValid(); }

    uint64                  GetSize()   const { return m_Desc.uSize; }
    RHIBufferUsage       GetUsage()  const { return m_Desc.usage; }
    RHIMemoryUsage       GetMemory() const { return m_Desc.memory; }
    const std::string&   GetName()   const { return m_Desc.sDebugName; }
    const RHIBufferDesc& GetDesc()   const { return m_Desc; }

private:
    RHIBufferHandle m_Handle;
    RHIBufferDesc   m_Desc;
};

} // namespace Evo
