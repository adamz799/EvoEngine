#pragma once

// ============================================================================
// RHI → DX12 type mapping utilities
// ============================================================================
//
// Pure mapping functions. No state, no side effects.
// Each function translates an RHI enum value to its DX12 equivalent.

#include "RHI/RHITypes.h"
#include "RHI/DX12/DX12Common.h"

namespace Evo {

// ---- Queue / Command list type ----
inline D3D12_COMMAND_LIST_TYPE MapQueueType(RHIQueueType type)
{
    switch (type) {
    case RHIQueueType::Graphics: return D3D12_COMMAND_LIST_TYPE_DIRECT;
    case RHIQueueType::Compute:  return D3D12_COMMAND_LIST_TYPE_COMPUTE;
    case RHIQueueType::Copy:     return D3D12_COMMAND_LIST_TYPE_COPY;
    }
    return D3D12_COMMAND_LIST_TYPE_DIRECT;
}

// ---- Pixel format ----
inline DXGI_FORMAT MapFormat(RHIFormat format)
{
    switch (format) {
    case RHIFormat::Unknown:            return DXGI_FORMAT_UNKNOWN;
    case RHIFormat::R8G8B8A8_UNORM:     return DXGI_FORMAT_R8G8B8A8_UNORM;
    case RHIFormat::R8G8B8A8_SRGB:      return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case RHIFormat::B8G8R8A8_UNORM:     return DXGI_FORMAT_B8G8R8A8_UNORM;
    case RHIFormat::B8G8R8A8_SRGB:      return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    case RHIFormat::R16G16B16A16_FLOAT: return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case RHIFormat::R32G32B32A32_FLOAT: return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case RHIFormat::R32G32B32_FLOAT:    return DXGI_FORMAT_R32G32B32_FLOAT;
    case RHIFormat::R32G32_FLOAT:       return DXGI_FORMAT_R32G32_FLOAT;
    case RHIFormat::D24_UNORM_S8_UINT:  return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case RHIFormat::D32_FLOAT:          return DXGI_FORMAT_D32_FLOAT;
    case RHIFormat::D32_FLOAT_S8X24_UINT: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    }
    return DXGI_FORMAT_UNKNOWN;
}

// ---- Resource state → D3D12_RESOURCE_STATES ----
inline D3D12_RESOURCE_STATES MapResourceState(RHIResourceState state)
{
    switch (state) {
    case RHIResourceState::Undefined:        return D3D12_RESOURCE_STATE_COMMON;
    case RHIResourceState::Common:           return D3D12_RESOURCE_STATE_COMMON;
    case RHIResourceState::VertexBuffer:     return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    case RHIResourceState::IndexBuffer:      return D3D12_RESOURCE_STATE_INDEX_BUFFER;
    case RHIResourceState::ConstantBuffer:   return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    case RHIResourceState::ShaderResource:   return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
                                                  | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    case RHIResourceState::UnorderedAccess:  return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    case RHIResourceState::RenderTarget:     return D3D12_RESOURCE_STATE_RENDER_TARGET;
    case RHIResourceState::DepthStencilWrite:return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    case RHIResourceState::DepthStencilRead: return D3D12_RESOURCE_STATE_DEPTH_READ;
    case RHIResourceState::CopySrc:          return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case RHIResourceState::CopyDst:          return D3D12_RESOURCE_STATE_COPY_DEST;
    case RHIResourceState::Present:          return D3D12_RESOURCE_STATE_PRESENT;
    }
    return D3D12_RESOURCE_STATE_COMMON;
}

// ---- Memory usage → D3D12_HEAP_TYPE ----
inline D3D12_HEAP_TYPE MapMemoryUsage(RHIMemoryUsage usage)
{
    switch (usage) {
    case RHIMemoryUsage::GpuOnly:  return D3D12_HEAP_TYPE_DEFAULT;
    case RHIMemoryUsage::CpuToGpu: return D3D12_HEAP_TYPE_UPLOAD;
    case RHIMemoryUsage::GpuToCpu: return D3D12_HEAP_TYPE_READBACK;
    }
    return D3D12_HEAP_TYPE_DEFAULT;
}

// ---- Primitive topology ----
inline D3D12_PRIMITIVE_TOPOLOGY MapPrimitiveTopology(RHIPrimitiveTopology topology)
{
    switch (topology) {
    case RHIPrimitiveTopology::TriangleList:  return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case RHIPrimitiveTopology::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    case RHIPrimitiveTopology::LineList:      return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case RHIPrimitiveTopology::LineStrip:     return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case RHIPrimitiveTopology::PointList:     return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    }
    return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

// ---- Primitive topology type (for PSO) ----
inline D3D12_PRIMITIVE_TOPOLOGY_TYPE MapPrimitiveTopologyType(RHIPrimitiveTopology topology)
{
    switch (topology) {
    case RHIPrimitiveTopology::TriangleList:
    case RHIPrimitiveTopology::TriangleStrip: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    case RHIPrimitiveTopology::LineList:
    case RHIPrimitiveTopology::LineStrip:     return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    case RHIPrimitiveTopology::PointList:     return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    }
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
}

// ---- Compare operation ----
inline D3D12_COMPARISON_FUNC MapCompareOp(RHICompareOp op)
{
    switch (op) {
    case RHICompareOp::Never:        return D3D12_COMPARISON_FUNC_NEVER;
    case RHICompareOp::Less:         return D3D12_COMPARISON_FUNC_LESS;
    case RHICompareOp::Equal:        return D3D12_COMPARISON_FUNC_EQUAL;
    case RHICompareOp::LessEqual:    return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case RHICompareOp::Greater:      return D3D12_COMPARISON_FUNC_GREATER;
    case RHICompareOp::NotEqual:     return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case RHICompareOp::GreaterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case RHICompareOp::Always:       return D3D12_COMPARISON_FUNC_ALWAYS;
    }
    return D3D12_COMPARISON_FUNC_LESS;
}

// ---- Blend factor ----
inline D3D12_BLEND MapBlendFactor(RHIBlendFactor factor)
{
    switch (factor) {
    case RHIBlendFactor::Zero:        return D3D12_BLEND_ZERO;
    case RHIBlendFactor::One:         return D3D12_BLEND_ONE;
    case RHIBlendFactor::SrcColor:    return D3D12_BLEND_SRC_COLOR;
    case RHIBlendFactor::InvSrcColor: return D3D12_BLEND_INV_SRC_COLOR;
    case RHIBlendFactor::SrcAlpha:    return D3D12_BLEND_SRC_ALPHA;
    case RHIBlendFactor::InvSrcAlpha: return D3D12_BLEND_INV_SRC_ALPHA;
    case RHIBlendFactor::DstAlpha:    return D3D12_BLEND_DEST_ALPHA;
    case RHIBlendFactor::InvDstAlpha: return D3D12_BLEND_INV_DEST_ALPHA;
    case RHIBlendFactor::DstColor:    return D3D12_BLEND_DEST_COLOR;
    case RHIBlendFactor::InvDstColor: return D3D12_BLEND_INV_DEST_COLOR;
    }
    return D3D12_BLEND_ONE;
}

// ---- Blend operation ----
inline D3D12_BLEND_OP MapBlendOp(RHIBlendOp op)
{
    switch (op) {
    case RHIBlendOp::Add:         return D3D12_BLEND_OP_ADD;
    case RHIBlendOp::Subtract:    return D3D12_BLEND_OP_SUBTRACT;
    case RHIBlendOp::RevSubtract: return D3D12_BLEND_OP_REV_SUBTRACT;
    case RHIBlendOp::Min:         return D3D12_BLEND_OP_MIN;
    case RHIBlendOp::Max:         return D3D12_BLEND_OP_MAX;
    }
    return D3D12_BLEND_OP_ADD;
}

// ---- Fill / Cull ----
inline D3D12_FILL_MODE MapFillMode(RHIFillMode mode)
{
    switch (mode) {
    case RHIFillMode::Solid:     return D3D12_FILL_MODE_SOLID;
    case RHIFillMode::Wireframe: return D3D12_FILL_MODE_WIREFRAME;
    }
    return D3D12_FILL_MODE_SOLID;
}

inline D3D12_CULL_MODE MapCullMode(RHICullMode mode)
{
    switch (mode) {
    case RHICullMode::None:  return D3D12_CULL_MODE_NONE;
    case RHICullMode::Front: return D3D12_CULL_MODE_FRONT;
    case RHICullMode::Back:  return D3D12_CULL_MODE_BACK;
    }
    return D3D12_CULL_MODE_BACK;
}

// ---- Index format ----
inline DXGI_FORMAT MapIndexFormat(RHIIndexFormat format)
{
    switch (format) {
    case RHIIndexFormat::U16: return DXGI_FORMAT_R16_UINT;
    case RHIIndexFormat::U32: return DXGI_FORMAT_R32_UINT;
    }
    return DXGI_FORMAT_R32_UINT;
}

} // namespace Evo
