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

// ---- Enhanced Barrier: sync stages ----
inline D3D12_BARRIER_SYNC MapBarrierSync(RHIBarrierSync s)
{
    D3D12_BARRIER_SYNC result = D3D12_BARRIER_SYNC_NONE;
    auto flags = static_cast<uint32>(s);
    if (flags & static_cast<uint32>(RHIBarrierSync::All))            result |= D3D12_BARRIER_SYNC_ALL;
    if (flags & static_cast<uint32>(RHIBarrierSync::Draw))           result |= D3D12_BARRIER_SYNC_DRAW;
    if (flags & static_cast<uint32>(RHIBarrierSync::VertexShading))  result |= D3D12_BARRIER_SYNC_VERTEX_SHADING;
    if (flags & static_cast<uint32>(RHIBarrierSync::PixelShading))   result |= D3D12_BARRIER_SYNC_PIXEL_SHADING;
    if (flags & static_cast<uint32>(RHIBarrierSync::DepthStencil))   result |= D3D12_BARRIER_SYNC_DEPTH_STENCIL;
    if (flags & static_cast<uint32>(RHIBarrierSync::RenderTarget))   result |= D3D12_BARRIER_SYNC_RENDER_TARGET;
    if (flags & static_cast<uint32>(RHIBarrierSync::ComputeShading)) result |= D3D12_BARRIER_SYNC_COMPUTE_SHADING;
    if (flags & static_cast<uint32>(RHIBarrierSync::Copy))           result |= D3D12_BARRIER_SYNC_COPY;
    if (flags & static_cast<uint32>(RHIBarrierSync::AllShading))     result |= D3D12_BARRIER_SYNC_ALL_SHADING;
    return result;
}

// ---- Enhanced Barrier: access types ----
inline D3D12_BARRIER_ACCESS MapBarrierAccess(RHIBarrierAccess a)
{
    if (a == RHIBarrierAccess::Common)   return D3D12_BARRIER_ACCESS_COMMON;
    if (a == RHIBarrierAccess::NoAccess) return D3D12_BARRIER_ACCESS_NO_ACCESS;

    D3D12_BARRIER_ACCESS result = D3D12_BARRIER_ACCESS_COMMON;
    auto flags = static_cast<uint32>(a);
    if (flags & static_cast<uint32>(RHIBarrierAccess::VertexBuffer))      result |= D3D12_BARRIER_ACCESS_VERTEX_BUFFER;
    if (flags & static_cast<uint32>(RHIBarrierAccess::ConstantBuffer))    result |= D3D12_BARRIER_ACCESS_CONSTANT_BUFFER;
    if (flags & static_cast<uint32>(RHIBarrierAccess::IndexBuffer))       result |= D3D12_BARRIER_ACCESS_INDEX_BUFFER;
    if (flags & static_cast<uint32>(RHIBarrierAccess::RenderTarget))      result |= D3D12_BARRIER_ACCESS_RENDER_TARGET;
    if (flags & static_cast<uint32>(RHIBarrierAccess::UnorderedAccess))   result |= D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
    if (flags & static_cast<uint32>(RHIBarrierAccess::DepthStencilWrite)) result |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE;
    if (flags & static_cast<uint32>(RHIBarrierAccess::DepthStencilRead))  result |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ;
    if (flags & static_cast<uint32>(RHIBarrierAccess::ShaderResource))    result |= D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
    if (flags & static_cast<uint32>(RHIBarrierAccess::CopyDest))          result |= D3D12_BARRIER_ACCESS_COPY_DEST;
    if (flags & static_cast<uint32>(RHIBarrierAccess::CopySource))        result |= D3D12_BARRIER_ACCESS_COPY_SOURCE;
    return result;
}

// ---- Enhanced Barrier: texture layout ----
inline D3D12_BARRIER_LAYOUT MapTextureLayout(RHITextureLayout l)
{
    switch (l) {
    case RHITextureLayout::Undefined:         return D3D12_BARRIER_LAYOUT_UNDEFINED;
    case RHITextureLayout::Common:            return D3D12_BARRIER_LAYOUT_COMMON;
    case RHITextureLayout::RenderTarget:      return D3D12_BARRIER_LAYOUT_RENDER_TARGET;
    case RHITextureLayout::UnorderedAccess:   return D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;
    case RHITextureLayout::DepthStencilWrite: return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;
    case RHITextureLayout::DepthStencilRead:  return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ;
    case RHITextureLayout::ShaderResource:    return D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
    case RHITextureLayout::CopySource:        return D3D12_BARRIER_LAYOUT_COPY_SOURCE;
    case RHITextureLayout::CopyDest:          return D3D12_BARRIER_LAYOUT_COPY_DEST;
    }
    return D3D12_BARRIER_LAYOUT_UNDEFINED;
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

// ---- Depth format helpers: typeless, DSV, and SRV-compatible formats ----

/// Returns true if the RHI format is a depth/stencil format.
inline bool IsDepthFormat(RHIFormat format)
{
    return format == RHIFormat::D32_FLOAT
        || format == RHIFormat::D24_UNORM_S8_UINT
        || format == RHIFormat::D32_FLOAT_S8X24_UINT;
}

/// Returns the typeless DXGI format for a depth format (for resource creation when SRV access is needed).
inline DXGI_FORMAT MapDepthToTypeless(RHIFormat format)
{
    switch (format) {
    case RHIFormat::D32_FLOAT:          return DXGI_FORMAT_R32_TYPELESS;
    case RHIFormat::D24_UNORM_S8_UINT:  return DXGI_FORMAT_R24G8_TYPELESS;
    case RHIFormat::D32_FLOAT_S8X24_UINT: return DXGI_FORMAT_R32G8X24_TYPELESS;
    default:                            return MapFormat(format);
    }
}

/// Returns the SRV-compatible DXGI format for sampling a depth texture.
inline DXGI_FORMAT MapDepthToSRVFormat(RHIFormat format)
{
    switch (format) {
    case RHIFormat::D32_FLOAT:          return DXGI_FORMAT_R32_FLOAT;
    case RHIFormat::D24_UNORM_S8_UINT:  return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    case RHIFormat::D32_FLOAT_S8X24_UINT: return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    default:                            return MapFormat(format);
    }
}

// ---- View wrapping: RHI View ↔ D3D12_CPU_DESCRIPTOR_HANDLE ----

inline RHIRenderTargetView WrapRTV(D3D12_CPU_DESCRIPTOR_HANDLE h)
{
	return { static_cast<uint64>(h.ptr) };
}

inline D3D12_CPU_DESCRIPTOR_HANDLE UnwrapRTV(RHIRenderTargetView v)
{
	return { static_cast<SIZE_T>(v.uOpaque) };
}

inline RHIDepthStencilView WrapDSV(D3D12_CPU_DESCRIPTOR_HANDLE h)
{
	return { static_cast<uint64>(h.ptr) };
}

inline D3D12_CPU_DESCRIPTOR_HANDLE UnwrapDSV(RHIDepthStencilView v)
{
	return { static_cast<SIZE_T>(v.uOpaque) };
}

} // namespace Evo
