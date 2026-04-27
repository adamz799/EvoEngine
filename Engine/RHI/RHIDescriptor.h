#pragma once

#include "RHI/RHITypes.h"

namespace Evo {

/// Descriptor binding — defines one slot within a descriptor set layout.
struct RHIDescriptorBinding {
    uint32               binding    = 0;      // Slot number
    RHIDescriptorType type       = RHIDescriptorType::ConstantBuffer;
    uint32               count      = 1;      // Array size (>1 for bindless arrays)
    RHIShaderStage    stageFlags = RHIShaderStage::Vertex;
};

/// Descriptor set layout — describes the shape of a descriptor set.
struct RHIDescriptorSetLayoutDesc {
    const RHIDescriptorBinding* bindings     = nullptr;
    uint32                         bindingCount = 0;
};

/// Descriptor write — specifies what resource to bind into a descriptor set slot.
struct RHIDescriptorWrite {
    uint32               binding     = 0;
    RHIDescriptorType type        = RHIDescriptorType::ConstantBuffer;
    uint32               arrayIndex  = 0;
    // Fill the relevant fields based on `type`:
    RHIBufferHandle   buffer;              // ConstantBuffer / UAV (buffer)
    uint64               bufferOffset = 0;
    uint64               bufferRange  = 0;    // 0 = whole buffer
    RHITextureHandle  texture;             // ShaderResource / UAV (texture)
};

} // namespace Evo
