#pragma once

#include "Core/Types.h"
#include <string>

namespace Evo {

// ============================================================================
// Constants
// ============================================================================

/// Number of back buffers (frames in flight) for swap chain double/triple buffering.
constexpr uint32 NUM_BACK_FRAMES = 3;

// ============================================================================
// Handle system
// ============================================================================

/// Lightweight typed handle. Internally an index + generation counter.
/// Generation prevents use-after-free: freed slots increment generation,
/// so stale handles fail validation.
template<typename Tag>
struct RHIHandle {
	uint64 uHandle       = UINT64_MAX;
	uint16 uGeneration   = 0;

	bool IsValid() const { return uHandle != UINT64_MAX; }
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
// View types — lightweight value wrappers around backend-specific descriptors.
// Resource (Handle) and View are 1:N — a single texture can have multiple views.
// ============================================================================

/// Render target view. DX12: wraps D3D12_CPU_DESCRIPTOR_HANDLE. Vulkan: wraps VkImageView.
struct RHIRenderTargetView {
	uint64 uOpaque = 0;
	bool IsValid() const { return uOpaque != 0; }
};

/// Depth/stencil view.
struct RHIDepthStencilView {
	uint64 uOpaque = 0;
	bool IsValid() const { return uOpaque != 0; }
};

// ============================================================================
// Enumerations
// ============================================================================

// ---- Backend type ----
enum class RHIBackendType : uint8 {
	DX12,
	Vulkan,
};

// ---- Queue type ----
enum class RHIQueueType : uint8 {
	Graphics,
	Compute,
	Copy,
};

// ---- Pixel formats ----
enum class RHIFormat : uint32 {
	Unknown = 0,
	R8G8B8A8_UNORM,
	R8G8B8A8_SRGB,
	B8G8R8A8_UNORM,
	B8G8R8A8_SRGB,
	R16G16B16A16_FLOAT,
	R32G32B32A32_FLOAT,
	R32G32B32_FLOAT,
	R32G32_FLOAT,
	R32_FLOAT,
	D24_UNORM_S8_UINT,
	D32_FLOAT,
	D32_FLOAT_S8X24_UINT,
};

// ---- Barrier sync stages (flags, combinable) ----
enum class RHIBarrierSync : uint32 {
	None            = 0,
	All             = 1 << 0,
	Draw            = 1 << 1,
	VertexShading   = 1 << 2,
	PixelShading    = 1 << 3,
	DepthStencil    = 1 << 4,
	RenderTarget    = 1 << 5,
	ComputeShading  = 1 << 6,
	Copy            = 1 << 7,
	AllShading      = 1 << 8,
};
inline RHIBarrierSync operator|(RHIBarrierSync a, RHIBarrierSync b) {
	return static_cast<RHIBarrierSync>(static_cast<uint32>(a) | static_cast<uint32>(b));
}
inline RHIBarrierSync operator&(RHIBarrierSync a, RHIBarrierSync b) {
	return static_cast<RHIBarrierSync>(static_cast<uint32>(a) & static_cast<uint32>(b));
}

// ---- Barrier access types (flags, combinable) ----
enum class RHIBarrierAccess : uint32 {
	Common            = 0,
	VertexBuffer      = 1 << 0,
	ConstantBuffer    = 1 << 1,
	IndexBuffer       = 1 << 2,
	RenderTarget      = 1 << 3,
	UnorderedAccess   = 1 << 4,
	DepthStencilWrite = 1 << 5,
	DepthStencilRead  = 1 << 6,
	ShaderResource    = 1 << 7,
	CopyDest          = 1 << 8,
	CopySource        = 1 << 9,
	NoAccess          = 1u << 31,
};
inline RHIBarrierAccess operator|(RHIBarrierAccess a, RHIBarrierAccess b) {
	return static_cast<RHIBarrierAccess>(static_cast<uint32>(a) | static_cast<uint32>(b));
}
inline RHIBarrierAccess operator&(RHIBarrierAccess a, RHIBarrierAccess b) {
	return static_cast<RHIBarrierAccess>(static_cast<uint32>(a) & static_cast<uint32>(b));
}

// ---- Texture memory layout (mutually exclusive, not flags) ----
enum class RHITextureLayout : uint32 {
	Undefined         = 0xFFFFFFFF,
	Common            = 0,       // also used for Present
	RenderTarget,
	UnorderedAccess,
	DepthStencilWrite,
	DepthStencilRead,
	ShaderResource,
	CopySource,
	CopyDest,
};

// ---- Shader stage ----
enum class RHIShaderStage : uint8 {
	Vertex,
	Pixel,
	Compute,
};

// ---- Memory usage ----
enum class RHIMemoryUsage : uint8 {
	GpuOnly,    // DX12: DEFAULT heap  | Vulkan: DEVICE_LOCAL
	CpuToGpu,   // DX12: UPLOAD heap   | Vulkan: HOST_VISIBLE | HOST_COHERENT
	GpuToCpu,   // DX12: READBACK heap | Vulkan: HOST_VISIBLE | HOST_CACHED
};

// ---- Buffer usage flags ----
enum class RHIBufferUsage : uint32 {
	Vertex   = 1 << 0,
	Index    = 1 << 1,
	Constant = 1 << 2,
	Storage  = 1 << 3,
	CopySrc  = 1 << 4,
	CopyDst  = 1 << 5,
	Indirect = 1 << 6,
};
inline RHIBufferUsage operator|(RHIBufferUsage a, RHIBufferUsage b) {
	return static_cast<RHIBufferUsage>(static_cast<uint32>(a) | static_cast<uint32>(b));
}
inline RHIBufferUsage operator&(RHIBufferUsage a, RHIBufferUsage b) {
	return static_cast<RHIBufferUsage>(static_cast<uint32>(a) & static_cast<uint32>(b));
}

// ---- Texture usage flags ----
enum class RHITextureUsage : uint32 {
	ShaderResource  = 1 << 0,
	RenderTarget    = 1 << 1,
	DepthStencil    = 1 << 2,
	UnorderedAccess = 1 << 3,
	CopySrc         = 1 << 4,
	CopyDst         = 1 << 5,
};
inline RHITextureUsage operator|(RHITextureUsage a, RHITextureUsage b) {
	return static_cast<RHITextureUsage>(static_cast<uint32>(a) | static_cast<uint32>(b));
}
inline RHITextureUsage operator&(RHITextureUsage a, RHITextureUsage b) {
	return static_cast<RHITextureUsage>(static_cast<uint32>(a) & static_cast<uint32>(b));
}

// ---- Texture dimension ----
enum class RHITextureDimension : uint8 {
	Tex2D,
	Tex3D,
	TexCube,
};

// ---- Index format ----
enum class RHIIndexFormat : uint8 {
	U16,
	U32,
};

// ============================================================================
// Buffer view types — describe how to interpret a buffer for binding.
// ============================================================================

/// Vertex buffer view — describes how to interpret a buffer as vertex data.
struct RHIVertexBufferView {
	RHIBufferHandle buffer;
	uint64          uOffset = 0;
	uint32          uSize   = 0;    // 0 = whole buffer from offset
	uint32          uStride = 0;
};

/// Index buffer view — describes how to interpret a buffer as index data.
struct RHIIndexBufferView {
	RHIBufferHandle buffer;
	uint64          uOffset = 0;
	uint32          uSize   = 0;
	RHIIndexFormat  format  = RHIIndexFormat::U32;
};

// ---- Primitive topology ----
enum class RHIPrimitiveTopology : uint8 {
	TriangleList,
	TriangleStrip,
	LineList,
	LineStrip,
	PointList,
};

// ---- Compare operation ----
enum class RHICompareOp : uint8 {
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
enum class RHIBlendFactor : uint8 {
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

enum class RHIBlendOp : uint8 {
	Add,
	Subtract,
	RevSubtract,
	Min,
	Max,
};

// ---- Fill / Cull ----
enum class RHIFillMode : uint8 { Solid, Wireframe };
enum class RHICullMode : uint8 { None, Front, Back };

// ---- Render pass load/store actions ----
enum class RHILoadAction : uint8 { Load, Clear, DontCare };
enum class RHIStoreAction : uint8 { Store, DontCare };

// ---- Descriptor type ----
enum class RHIDescriptorType : uint8 {
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
	float32 fX      = 0.0f;
	float32 fY      = 0.0f;
	float32 fWidth  = 0.0f;
	float32 fHeight = 0.0f;
	float32 fMinDepth = 0.0f;
	float32 fMaxDepth = 1.0f;
};

struct RHIScissorRect {
	int32 left   = 0;
	int32 top    = 0;
	int32 right  = 0;
	int32 bottom = 0;
};

struct RHIColor {
	float32 fR = 0.0f;
	float32 fG = 0.0f;
	float32 fB = 0.0f;
	float32 fA = 1.0f;
};

// ---- Device ----
struct RHIDeviceDesc {
	RHIBackendType backend     = RHIBackendType::DX12;
	bool           bEnableDebug = true;
};

// ---- Buffer ----
struct RHIBufferDesc {
	uint64            uSize      = 0;
	RHIBufferUsage usage      = RHIBufferUsage::Vertex;
	RHIMemoryUsage memory     = RHIMemoryUsage::GpuOnly;
	std::string    sDebugName;
};

// ---- Texture ----
struct RHITextureDesc {
	uint32                 uWidth           = 1;
	uint32                 uHeight          = 1;
	uint32                 uDepthOrArraySize = 1;
	uint32                 uMipLevels       = 1;
	RHIFormat           format           = RHIFormat::R8G8B8A8_UNORM;
	RHITextureUsage     usage            = RHITextureUsage::ShaderResource;
	RHITextureDimension dimension        = RHITextureDimension::Tex2D;
	RHIColor            clearColor       = {};   // optimized clear value for RT/DS
	std::string         sDebugName;
};

// ---- Shader ----
struct RHIShaderDesc {
	const void*    pBytecode     = nullptr;
	uint64            uBytecodeSize = 0;
	RHIShaderStage stage        = RHIShaderStage::Vertex;
	std::string    sEntryPoint  = "main";
	std::string    sDebugName;
};

// ---- Input layout element (vertex attribute) ----
struct RHIInputElement {
	const char* pSemanticName  = nullptr;   // "POSITION", "TEXCOORD", etc.
	uint32         uSemanticIndex = 0;
	RHIFormat   format         = RHIFormat::Unknown;
	uint32         uByteOffset    = 0;         // offset within vertex
	uint32         uBufferSlot    = 0;         // vertex buffer slot
};

// ---- Rasterizer state ----
struct RHIRasterizerDesc {
	RHIFillMode fillMode               = RHIFillMode::Solid;
	RHICullMode cullMode               = RHICullMode::Back;
	bool        bFrontCounterClockwise = false;
	int32         depthBias              = 0;
	float32         fSlopeScaledDepthBias  = 0.0f;
};

// ---- Blend state (per render target) ----
struct RHIBlendTargetDesc {
	bool           bBlendEnable = false;
	RHIBlendFactor srcColor     = RHIBlendFactor::One;
	RHIBlendFactor dstColor     = RHIBlendFactor::Zero;
	RHIBlendOp     colorOp      = RHIBlendOp::Add;
	RHIBlendFactor srcAlpha     = RHIBlendFactor::One;
	RHIBlendFactor dstAlpha     = RHIBlendFactor::Zero;
	RHIBlendOp     alphaOp      = RHIBlendOp::Add;
	uint8             uWriteMask   = 0xF;   // R|G|B|A
};

// ---- Depth/stencil state ----
struct RHIDepthStencilDesc {
	bool         bDepthTestEnable  = true;
	bool         bDepthWriteEnable = true;
	RHICompareOp depthCompareOp    = RHICompareOp::Less;
};

// ---- Graphics pipeline ----
struct RHIGraphicsPipelineDesc {
	RHIShaderHandle vertexShader;
	RHIShaderHandle pixelShader;

	const RHIInputElement* pInputElements     = nullptr;
	uint32                    uInputElementCount = 0;

	RHIRasterizerDesc   rasterizer;
	RHIDepthStencilDesc depthStencil;
	RHIBlendTargetDesc  blendTargets[8] = {};

	uint32       uRenderTargetCount      = 1;
	RHIFormat renderTargetFormats[8] = { RHIFormat::R8G8B8A8_UNORM };
	RHIFormat depthStencilFormat     = RHIFormat::Unknown;

	RHIPrimitiveTopology topology    = RHIPrimitiveTopology::TriangleList;

	RHIDescriptorSetLayoutHandle descriptorSetLayouts[4] = {};
	uint32                          uDescriptorSetLayoutCount = 0;
	uint32                          uPushConstantSize         = 0;

	std::string sDebugName;
};

// ---- Compute pipeline ----
struct RHIComputePipelineDesc {
	RHIShaderHandle computeShader;

	RHIDescriptorSetLayoutHandle descriptorSetLayouts[4] = {};
	uint32                          uDescriptorSetLayoutCount = 0;
	uint32                          uPushConstantSize         = 0;

	std::string sDebugName;
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
	float32              fClearDepth  = 1.0f;
	uint8               uClearStencil = 0;
};

struct RHIRenderPassDesc {
	RHIColorAttachment colorAttachments[8] = {};
	uint32                uColorAttachmentCount = 0;
	RHIDepthAttachment depthAttachment;
	bool               bHasDepthAttachment = false;
};

// ---- Barrier ----
struct RHITextureBarrier {
	RHITextureHandle   texture;
	RHIBarrierSync     syncBefore;
	RHIBarrierSync     syncAfter;
	RHIBarrierAccess   accessBefore;
	RHIBarrierAccess   accessAfter;
	RHITextureLayout   layoutBefore;
	RHITextureLayout   layoutAfter;
};

struct RHIBufferBarrier {
	RHIBufferHandle    buffer;
	RHIBarrierSync     syncBefore;
	RHIBarrierSync     syncAfter;
	RHIBarrierAccess   accessBefore;
	RHIBarrierAccess   accessAfter;
};

// ---- Swap chain ----
struct RHISwapChainDesc {
	void*     pWindowHandle = nullptr;
	uint32       uWidth        = 1280;
	uint32       uHeight       = 720;
	uint32       uBufferCount  = 2;
	RHIFormat format        = RHIFormat::R8G8B8A8_UNORM;
	bool      bVsync        = true;
};

} // namespace Evo
