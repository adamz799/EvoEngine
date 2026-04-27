#pragma once

#include "Core/Types.h"
#include <string>

namespace Evo {

// ============================================================================
// Handle system
// ============================================================================

/// Lightweight typed handle. Internally an index + generation counter.
/// Generation prevents use-after-free: freed slots increment generation,
/// so stale handles fail validation.
template<typename Tag>
struct RHIHandle {
    u32 index      = UINT32_MAX;
    u16 generation = 0;

    bool IsValid() const { return index != UINT32_MAX; }
    bool operator==(const RHIHandle&) const = default;
    bool operator!=(const RHIHandle&) const = default;
};

// Typed handles ---- compile-time type safety, zero runtime cost
using RHIBufferHandle              = RHIHandle<struct BufferTag>;
using RHITextureHandle             = RHIHandle<struct TextureTag>;
using RHIShaderHandle              = RHIHandle<struct ShaderTag>;
using RHIPipelineHandle            = RHIHandle<struct PipelineTag>;
using RHIDescriptorSetLayoutHandle = RHIHandle<struct DescriptorSetLayoutTag>;
using RHIDescriptorSetHandle       = RHIHandle<struct DescriptorSetTag>;

// ============================================================================
// Enumerations
// ============================================================================

// ---- Backend type ----
enum class RHIBackendType : u8 {
    DX12,
    Vulkan,
};

// ---- Queue type ----
enum class RHIQueueType : u8 {
    Graphics,
    Compute,
    Copy,
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
    R32G32B32_FLOAT,
    R32G32_FLOAT,
    D24_UNORM_S8_UINT,
    D32_FLOAT,
    D32_FLOAT_S8X24_UINT,
};

// ---- Resource state (for barriers) ----
enum class RHIResourceState : u32 {
    Undefined = 0,
    Common,
    VertexBuffer,
    IndexBuffer,
    ConstantBuffer,
    ShaderResource,
    UnorderedAccess,
    RenderTarget,
    DepthStencilWrite,
    DepthStencilRead,
    CopySrc,
    CopyDst,
    Present,
};

// ---- Shader stage ----
enum class RHIShaderStage : u8 {
    Vertex,
    Pixel,
    Compute,
};

// ---- Memory usage ----
enum class RHIMemoryUsage : u8 {
    GpuOnly,    // DX12: DEFAULT heap  | Vulkan: DEVICE_LOCAL
    CpuToGpu,   // DX12: UPLOAD heap   | Vulkan: HOST_VISIBLE | HOST_COHERENT
    GpuToCpu,   // DX12: READBACK heap | Vulkan: HOST_VISIBLE | HOST_CACHED
};

// ---- Buffer usage flags ----
enum class RHIBufferUsage : u32 {
    Vertex   = 1 << 0,
    Index    = 1 << 1,
    Constant = 1 << 2,
    Storage  = 1 << 3,
    CopySrc  = 1 << 4,
    CopyDst  = 1 << 5,
    Indirect = 1 << 6,
};
inline RHIBufferUsage operator|(RHIBufferUsage a, RHIBufferUsage b) {
    return static_cast<RHIBufferUsage>(static_cast<u32>(a) | static_cast<u32>(b));
}
inline RHIBufferUsage operator&(RHIBufferUsage a, RHIBufferUsage b) {
    return static_cast<RHIBufferUsage>(static_cast<u32>(a) & static_cast<u32>(b));
}

// ---- Texture usage flags ----
enum class RHITextureUsage : u32 {
    ShaderResource  = 1 << 0,
    RenderTarget    = 1 << 1,
    DepthStencil    = 1 << 2,
    UnorderedAccess = 1 << 3,
    CopySrc         = 1 << 4,
    CopyDst         = 1 << 5,
};
inline RHITextureUsage operator|(RHITextureUsage a, RHITextureUsage b) {
    return static_cast<RHITextureUsage>(static_cast<u32>(a) | static_cast<u32>(b));
}
inline RHITextureUsage operator&(RHITextureUsage a, RHITextureUsage b) {
    return static_cast<RHITextureUsage>(static_cast<u32>(a) & static_cast<u32>(b));
}

// ---- Texture dimension ----
enum class RHITextureDimension : u8 {
    Tex2D,
    Tex3D,
    TexCube,
};

// ---- Index format ----
enum class RHIIndexFormat : u8 {
    U16,
    U32,
};

// ---- Primitive topology ----
enum class RHIPrimitiveTopology : u8 {
    TriangleList,
    TriangleStrip,
    LineList,
    LineStrip,
    PointList,
};

// ---- Compare operation ----
enum class RHICompareOp : u8 {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,
};

// ---- Blend ----
enum class RHIBlendFactor : u8 {
    Zero,
    One,
    SrcColor,
    InvSrcColor,
    SrcAlpha,
    InvSrcAlpha,
    DstAlpha,
    InvDstAlpha,
    DstColor,
    InvDstColor,
};

enum class RHIBlendOp : u8 {
    Add,
    Subtract,
    RevSubtract,
    Min,
    Max,
};

// ---- Fill / Cull ----
enum class RHIFillMode : u8 { Solid, Wireframe };
enum class RHICullMode : u8 { None, Front, Back };

// ---- Render pass load/store actions ----
enum class RHILoadAction : u8 { Load, Clear, DontCare };
enum class RHIStoreAction : u8 { Store, DontCare };

// ---- Descriptor type ----
enum class RHIDescriptorType : u8 {
    ConstantBuffer,      // DX12: CBV     | Vulkan: UNIFORM_BUFFER
    ShaderResource,      // DX12: SRV     | Vulkan: SAMPLED_IMAGE
    UnorderedAccess,     // DX12: UAV     | Vulkan: STORAGE_IMAGE / STORAGE_BUFFER
    Sampler,             // DX12: Sampler | Vulkan: SAMPLER
};

// ============================================================================
// Descriptor structures
// ============================================================================

// ---- Common small structs ----
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

// ---- Device ----
struct RHIDeviceDesc {
    RHIBackendType backend     = RHIBackendType::DX12;
    bool           enableDebug = true;
};

// ---- Buffer ----
struct RHIBufferDesc {
    u64            size       = 0;
    RHIBufferUsage usage      = RHIBufferUsage::Vertex;
    RHIMemoryUsage memory     = RHIMemoryUsage::GpuOnly;
    std::string    debugName;
};

// ---- Texture ----
struct RHITextureDesc {
    u32                 width            = 1;
    u32                 height           = 1;
    u32                 depthOrArraySize = 1;
    u32                 mipLevels        = 1;
    RHIFormat           format           = RHIFormat::R8G8B8A8_UNORM;
    RHITextureUsage     usage            = RHITextureUsage::ShaderResource;
    RHITextureDimension dimension        = RHITextureDimension::Tex2D;
    std::string         debugName;
};

// ---- Shader ----
struct RHIShaderDesc {
    const void*    bytecode     = nullptr;
    u64            bytecodeSize = 0;
    RHIShaderStage stage        = RHIShaderStage::Vertex;
    std::string    entryPoint   = "main";
    std::string    debugName;
};

// ---- Input layout element (vertex attribute) ----
struct RHIInputElement {
    const char* semanticName  = nullptr;   // "POSITION", "TEXCOORD", etc.
    u32         semanticIndex = 0;
    RHIFormat   format        = RHIFormat::Unknown;
    u32         byteOffset    = 0;         // offset within vertex
    u32         bufferSlot    = 0;         // vertex buffer slot
};

// ---- Rasterizer state ----
struct RHIRasterizerDesc {
    RHIFillMode fillMode              = RHIFillMode::Solid;
    RHICullMode cullMode              = RHICullMode::Back;
    bool        frontCounterClockwise = false;
    i32         depthBias             = 0;
    f32         slopeScaledDepthBias  = 0.0f;
};

// ---- Blend state (per render target) ----
struct RHIBlendTargetDesc {
    bool           blendEnable = false;
    RHIBlendFactor srcColor    = RHIBlendFactor::One;
    RHIBlendFactor dstColor    = RHIBlendFactor::Zero;
    RHIBlendOp     colorOp     = RHIBlendOp::Add;
    RHIBlendFactor srcAlpha    = RHIBlendFactor::One;
    RHIBlendFactor dstAlpha    = RHIBlendFactor::Zero;
    RHIBlendOp     alphaOp     = RHIBlendOp::Add;
    u8             writeMask   = 0xF;   // R|G|B|A
};

// ---- Depth/stencil state ----
struct RHIDepthStencilDesc {
    bool         depthTestEnable  = true;
    bool         depthWriteEnable = true;
    RHICompareOp depthCompareOp   = RHICompareOp::Less;
};

// ---- Graphics pipeline ----
struct RHIGraphicsPipelineDesc {
    RHIShaderHandle vertexShader;
    RHIShaderHandle pixelShader;

    const RHIInputElement* inputElements    = nullptr;
    u32                    inputElementCount = 0;

    RHIRasterizerDesc   rasterizer;
    RHIDepthStencilDesc depthStencil;
    RHIBlendTargetDesc  blendTargets[8] = {};

    u32       renderTargetCount      = 1;
    RHIFormat renderTargetFormats[8] = { RHIFormat::R8G8B8A8_UNORM };
    RHIFormat depthStencilFormat     = RHIFormat::Unknown;

    RHIPrimitiveTopology topology    = RHIPrimitiveTopology::TriangleList;

    RHIDescriptorSetLayoutHandle descriptorSetLayouts[4] = {};
    u32                          descriptorSetLayoutCount = 0;
    u32                          pushConstantSize         = 0;

    std::string debugName;
};

// ---- Compute pipeline ----
struct RHIComputePipelineDesc {
    RHIShaderHandle computeShader;

    RHIDescriptorSetLayoutHandle descriptorSetLayouts[4] = {};
    u32                          descriptorSetLayoutCount = 0;
    u32                          pushConstantSize         = 0;

    std::string debugName;
};

// ---- Render pass ----
struct RHIColorAttachment {
    RHITextureHandle texture;
    RHILoadAction    loadAction  = RHILoadAction::Clear;
    RHIStoreAction   storeAction = RHIStoreAction::Store;
    RHIColor         clearColor  = { 0.0f, 0.0f, 0.0f, 1.0f };
};

struct RHIDepthAttachment {
    RHITextureHandle texture;
    RHILoadAction    loadAction   = RHILoadAction::Clear;
    RHIStoreAction   storeAction  = RHIStoreAction::Store;
    f32              clearDepth   = 1.0f;
    u8               clearStencil = 0;
};

struct RHIRenderPassDesc {
    RHIColorAttachment colorAttachments[8] = {};
    u32                colorAttachmentCount = 0;
    RHIDepthAttachment depthAttachment;
    bool               hasDepthAttachment = false;
};

// ---- Barrier ----
struct RHITextureBarrier {
    RHITextureHandle texture;
    RHIResourceState before;
    RHIResourceState after;
};

struct RHIBufferBarrier {
    RHIBufferHandle  buffer;
    RHIResourceState before;
    RHIResourceState after;
};

// ---- Swap chain ----
struct RHISwapChainDesc {
    void*     windowHandle = nullptr;
    u32       width        = 1280;
    u32       height       = 720;
    u32       bufferCount  = 2;
    RHIFormat format       = RHIFormat::R8G8B8A8_UNORM;
    bool      vsync        = true;
};

} // namespace Evo
