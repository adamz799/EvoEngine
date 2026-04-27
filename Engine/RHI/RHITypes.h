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
	uint64 Handle       = UINT64_MAX;
	uint16 generation   = 0;

	bool IsValid() const { return Handle != UINT64_MAX; }
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
	D24_UNORM_S8_UINT,
	D32_FLOAT,
	D32_FLOAT_S8X24_UINT,
};

// ---- Resource state (for barriers) ----
enum class RHIResourceState : uint32 {
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
	float32 x      = 0.0f;
	float32 y      = 0.0f;
	float32 width  = 0.0f;
	float32 height = 0.0f;
	float32 minDepth = 0.0f;
	float32 maxDepth = 1.0f;
};

struct RHIScissorRect {
	int32 left   = 0;
	int32 top    = 0;
	int32 right  = 0;
	int32 bottom = 0;
};

struct RHIColor {
	float32 r = 0.0f;
	float32 g = 0.0f;
	float32 b = 0.0f;
	float32 a = 1.0f;
};

// ---- Device ----
struct RHIDeviceDesc {
	RHIBackendType backend     = RHIBackendType::DX12;
	bool           enableDebug = true;
};

// ---- Buffer ----
struct RHIBufferDesc {
	uint64            size       = 0;
	RHIBufferUsage usage      = RHIBufferUsage::Vertex;
	RHIMemoryUsage memory     = RHIMemoryUsage::GpuOnly;
	std::string    debugName;
};

// ---- Texture ----
struct RHITextureDesc {
	uint32                 width            = 1;
	uint32                 height           = 1;
	uint32                 depthOrArraySize = 1;
	uint32                 mipLevels        = 1;
	RHIFormat           format           = RHIFormat::R8G8B8A8_UNORM;
	RHITextureUsage     usage            = RHITextureUsage::ShaderResource;
	RHITextureDimension dimension        = RHITextureDimension::Tex2D;
	std::string         debugName;
};

// ---- Shader ----
struct RHIShaderDesc {
	const void*    bytecode     = nullptr;
	uint64            bytecodeSize = 0;
	RHIShaderStage stage        = RHIShaderStage::Vertex;
	std::string    entryPoint   = "main";
	std::string    debugName;
};

// ---- Input layout element (vertex attribute) ----
struct RHIInputElement {
	const char* semanticName  = nullptr;   // "POSITION", "TEXCOORD", etc.
	uint32         semanticIndex = 0;
	RHIFormat   format        = RHIFormat::Unknown;
	uint32         byteOffset    = 0;         // offset within vertex
	uint32         bufferSlot    = 0;         // vertex buffer slot
};

// ---- Rasterizer state ----
struct RHIRasterizerDesc {
	RHIFillMode fillMode              = RHIFillMode::Solid;
	RHICullMode cullMode              = RHICullMode::Back;
	bool        frontCounterClockwise = false;
	int32         depthBias             = 0;
	float32         slopeScaledDepthBias  = 0.0f;
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
	uint8             writeMask   = 0xF;   // R|G|B|A
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
	uint32                    inputElementCount = 0;

	RHIRasterizerDesc   rasterizer;
	RHIDepthStencilDesc depthStencil;
	RHIBlendTargetDesc  blendTargets[8] = {};

	uint32       renderTargetCount      = 1;
	RHIFormat renderTargetFormats[8] = { RHIFormat::R8G8B8A8_UNORM };
	RHIFormat depthStencilFormat     = RHIFormat::Unknown;

	RHIPrimitiveTopology topology    = RHIPrimitiveTopology::TriangleList;

	RHIDescriptorSetLayoutHandle descriptorSetLayouts[4] = {};
	uint32                          descriptorSetLayoutCount = 0;
	uint32                          pushConstantSize         = 0;

	std::string debugName;
};

// ---- Compute pipeline ----
struct RHIComputePipelineDesc {
	RHIShaderHandle computeShader;

	RHIDescriptorSetLayoutHandle descriptorSetLayouts[4] = {};
	uint32                          descriptorSetLayoutCount = 0;
	uint32                          pushConstantSize         = 0;

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
	float32              clearDepth   = 1.0f;
	uint8               clearStencil = 0;
};

struct RHIRenderPassDesc {
	RHIColorAttachment colorAttachments[8] = {};
	uint32                colorAttachmentCount = 0;
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
	uint32       width        = 1280;
	uint32       height       = 720;
	uint32       bufferCount  = 2;
	RHIFormat format       = RHIFormat::R8G8B8A8_UNORM;
	bool      vsync        = true;
};

} // namespace Evo
