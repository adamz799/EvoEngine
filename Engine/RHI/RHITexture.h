#pragma once

#include "RHI/RHITypes.h"
#include <memory>

namespace Evo {

class RHITexture {
public:
    virtual ~RHITexture() = default;

    virtual RHIFormat GetFormat() const = 0;
    virtual u32       GetWidth()  const = 0;
    virtual u32       GetHeight() const = 0;
};

} // namespace Evo
