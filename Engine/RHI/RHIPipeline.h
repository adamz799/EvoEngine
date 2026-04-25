#pragma once

#include "RHI/RHITypes.h"
#include <memory>

namespace Evo {

class RHIPipeline {
public:
    virtual ~RHIPipeline() = default;

    // TODO: Pipeline creation and binding interfaces
    // Will be expanded when implementing actual rendering passes
};

} // namespace Evo
