#pragma once

#include "RHI/RHITypes.h"
#include <memory>

namespace Evo {

class RHIBuffer {
public:
    virtual ~RHIBuffer() = default;

    virtual u64  GetSize() const = 0;

    /// Map buffer for CPU write. Returns nullptr on failure.
    virtual void* Map() = 0;
    virtual void  Unmap() = 0;
};

} // namespace Evo
