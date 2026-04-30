#pragma once

#include "RHI/RHITypes.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIDevice.h"
#include <vector>
#include <string>
#include <functional>

namespace Evo {

// ============================================================================
// RGHandle — opaque reference to a resource in the render graph
// ============================================================================

struct RGHandle {
	uint32 index = UINT32_MAX;
	bool IsValid() const { return index != UINT32_MAX; }
};

// ============================================================================
// RGPassBuilder — used during pass setup to declare resource usage
// ============================================================================

class RGPassBuilder {
public:
	/// Declare this pass writes to a render target. Provide pre-created RTV.
	void WriteRenderTarget(RGHandle texture, RHIRenderTargetView rtv,
						   const RHIColor* pClearColor = nullptr);

	/// Declare this pass writes to a depth/stencil target. Provide pre-created DSV.
	void WriteDepthStencil(RGHandle texture, RHIDepthStencilView dsv,
						   float fClearDepth = 1.0f, uint8 uClearStencil = 0);

	/// Declare this pass reads depth (depth test on, depth write off). Provide pre-created DSV.
	void ReadDepthStencil(RGHandle texture, RHIDepthStencilView dsv);

	/// Declare this pass reads a texture as shader resource.
	void ReadTexture(RGHandle texture);

private:
	friend class RenderGraph;

	struct RTOutput {
		RGHandle            texture;
		RHIRenderTargetView rtv;
		RHIColor            clearColor = {};
		bool                bClear     = false;
	};

	struct DSOutput {
		RGHandle            texture;
		RHIDepthStencilView dsv;
		float               fClearDepth   = 1.0f;
		uint8               uClearStencil = 0;
		bool                bClear        = false;
		bool                bReadOnly     = false;
	};

	std::vector<RTOutput> m_vRenderTargets;
	std::vector<RGHandle> m_vTextureReads;
	DSOutput              m_DepthOutput;
};

// ============================================================================
// RenderGraph — declares passes + resources, compiles barriers, executes
// ============================================================================

class RenderGraph {
public:
	/// Import an externally-owned texture (e.g. back buffer).
	/// currentLayout: the layout the texture is in right now.
	/// finalLayout:   the layout the texture must be in when the graph finishes.
	RGHandle ImportTexture(const char* name, RHITextureHandle texture,
						   RHITextureLayout currentLayout = RHITextureLayout::Common,
						   RHITextureLayout finalLayout   = RHITextureLayout::Common);

	/// Add a render pass.
	///   setupFn:   declare resource reads/writes via RGPassBuilder
	///   executeFn: record draw commands via RHICommandList*
	void AddPass(const char* name,
				 std::function<void(RGPassBuilder&)> setupFn,
				 std::function<void(RHICommandList*)> executeFn);

	/// Compile: walk passes, deduce layout transitions, generate barrier list.
	void Compile();

	/// Execute: each pass acquires its own CmdList from the device pool.
	/// Returned CmdLists are in End() state, ready for submission in order.
	void Execute(RHIDevice* pDevice, std::vector<RHICommandList*>& outCmdLists);

	/// Clear all passes and resource references. Call at the start of each frame.
	void Reset();

private:
	struct RGTexture {
		RHITextureHandle texture;
		RHITextureLayout currentLayout = RHITextureLayout::Common;
		RHITextureLayout finalLayout   = RHITextureLayout::Common;
		std::string      sName;
	};

	struct RGPass {
		std::string                          sName;
		std::vector<RGPassBuilder::RTOutput> vRenderTargets;
		std::vector<RGHandle>                vTextureReads;
		RGPassBuilder::DSOutput              depthOutput;
		std::function<void(RHICommandList*)> executeFn;
	};

	struct CompiledPass {
		std::vector<RHITextureBarrier>   vBarriersBefore;
		std::vector<RHIRenderTargetView> vRTVs;
		std::vector<std::pair<RHIRenderTargetView, RHIColor>> vClears;
		RHIDepthStencilView              dsv;
		bool                             bClearDepth    = false;
		float                            fClearDepth    = 1.0f;
		uint8                            uClearStencil  = 0;
		uint32                           uPassIndex     = 0;
	};

	std::vector<RGTexture>         m_vTextures;
	std::vector<RGPass>            m_vPasses;
	std::vector<CompiledPass>      m_vCompiledPasses;
	std::vector<RHITextureBarrier> m_vFinalBarriers;
};

} // namespace Evo

