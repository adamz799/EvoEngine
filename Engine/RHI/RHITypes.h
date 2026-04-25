#pragma once

#include "Core/Types.h"
#include <string>

namespace Evo {

// ---- Backend type ----
enum class RHIBackendType : u8 {
    DX12,
    Vulkan,
};

// ---- Pixel formats ----
enum class RHIFormat : u32 {
    Unknown = 0,
    R8G8B8A8_UNORM,
    R8G8B8A8_SRGB,
    B8G8R8A8_UNORM,
    B8G8R8A8_SRGB,
    R16G16B16A16_FLOAT,
    R32G32B32A32_FLOAT,
    D24_UNORM_S8_UINT,
    D32_FLOAT,
    D32_FLOAT_S8X24_UINT,
};

// ---- Buffer usage ----
enum class RHIBufferUsage : u32 {
    Vertex   = 1 << 0,
    Index    = 1 << 1,
    Constant = 1 << 2,
    Storage  = 1 << 3,
    CopySrc  = 1 << 4,
    CopyDst  = 1 << 5,
};

// ---- Texture usage ----
enum class RHITextureUsage : u32 {
    ShaderResource = 1 << 0,
    RenderTarget   = 1 << 1,
    DepthStencil   = 1 << 2,
    UnorderedAccess= 1 << 3,
    CopySrc        = 1 << 4,
    CopyDst        = 1 << 5,
};

// ---- Command list type ----
enum class RHICommandListType : u8 {
    Graphics,
    Compute,
    Copy,
};

// ---- Descriptors ----
struct RHIViewport {
    f32 x      = 0.0f;
    f32 y      = 0.0f;
    f32 width  = 0.0f;
    f32 height = 0.0f;
    f32 minDepth = 0.0f;
    f32 maxDepth = 1.0f;
};

struct RHIScissorRect {
    i32 left   = 0;
    i32 top    = 0;
    i32 right  = 0;
    i32 bottom = 0;
};

struct RHIColor {
    f32 r = 0.0f;
    f32 g = 0.0f;
    f32 b = 0.0f;
    f32 a = 1.0f;
};

// ---- Creation descriptors ----
struct RHIDeviceDesc {
    RHIBackendType backend   = RHIBackendType::DX12;
    bool           enableDebug = true;
    void*          windowHandle = nullptr;  // HWND on Windows
    u32            windowWidth  = 1280;
    u32            windowHeight = 720;
};

struct RHIBufferDesc {
    u64            size  = 0;
    RHIBufferUsage usage = RHIBufferUsage::Vertex;
    std::string    name;
};

struct RHITextureDesc {
    u32             width  = 1;
    u32             height = 1;
    u32             depth  = 1;
    u32             mipLevels = 1;
    RHIFormat       format = RHIFormat::R8G8B8A8_UNORM;
    RHITextureUsage usage  = RHITextureUsage::ShaderResource;
    std::string     name;
};

struct RHISwapChainDesc {
    void* windowHandle = nullptr;
    u32   width        = 1280;
    u32   height       = 720;
    u32   bufferCount  = 2;
    RHIFormat format   = RHIFormat::R8G8B8A8_UNORM;
};

} // namespace Evo
