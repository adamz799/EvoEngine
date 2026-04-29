#include "Renderer/RenderGraph.h"
#include "Core/Log.h"

namespace Evo {

/// Map a tracked texture layout to its corresponding barrier access flags.
/// Enhanced Barriers require accessBefore to be compatible with layoutBefore.
static RHIBarrierAccess AccessFromLayout(RHITextureLayout layout)
{
	switch (layout)
	{
		case RHITextureLayout::Common:            return RHIBarrierAccess::Common;
		case RHITextureLayout::RenderTarget:      return RHIBarrierAccess::RenderTarget;
		case RHITextureLayout::ShaderResource:    return RHIBarrierAccess::ShaderResource;
		case RHITextureLayout::UnorderedAccess:   return RHIBarrierAccess::UnorderedAccess;
		case RHITextureLayout::DepthStencilWrite: return RHIBarrierAccess::DepthStencilWrite;
		case RHITextureLayout::DepthStencilRead:  return RHIBarrierAccess::DepthStencilRead;
		case RHITextureLayout::CopySource:        return RHIBarrierAccess::CopySource;
		case RHITextureLayout::CopyDest:          return RHIBarrierAccess::CopyDest;
		default:                                  return RHIBarrierAccess::NoAccess;
	}
}

// ============================================================================
// RGPassBuilder
// ============================================================================

void RGPassBuilder::WriteRenderTarget(RGHandle texture, RHIRenderTargetView rtv,
                                      const RHIColor* pClearColor)
{
	RTOutput out;
	out.texture = texture;
	out.rtv     = rtv;
	if (pClearColor)
	{
		out.clearColor = *pClearColor;
		out.bClear     = true;
	}
	m_vRenderTargets.push_back(out);
}

void RGPassBuilder::ReadTexture(RGHandle texture)
{
	m_vTextureReads.push_back(texture);
}

void RGPassBuilder::WriteDepthStencil(RGHandle texture, RHIDepthStencilView dsv,
                                      float fClearDepth, uint8 uClearStencil)
{
	m_DepthOutput.texture       = texture;
	m_DepthOutput.dsv           = dsv;
	m_DepthOutput.fClearDepth   = fClearDepth;
	m_DepthOutput.uClearStencil = uClearStencil;
	m_DepthOutput.bClear        = true;
	m_DepthOutput.bReadOnly     = false;
}

void RGPassBuilder::ReadDepthStencil(RGHandle texture, RHIDepthStencilView dsv)
{
	m_DepthOutput.texture       = texture;
	m_DepthOutput.dsv           = dsv;
	m_DepthOutput.bClear        = false;
	m_DepthOutput.bReadOnly     = true;
}

// ============================================================================
// RenderGraph
// ============================================================================

RGHandle RenderGraph::ImportTexture(const char* name, RHITextureHandle texture,
                                    RHITextureLayout currentLayout,
                                    RHITextureLayout finalLayout)
{
	RGHandle handle;
	handle.index = static_cast<uint32>(m_vTextures.size());

	RGTexture entry;
	entry.texture       = texture;
	entry.currentLayout = currentLayout;
	entry.finalLayout   = finalLayout;
	entry.sName         = name;
	m_vTextures.push_back(std::move(entry));

	return handle;
}

void RenderGraph::AddPass(const char* name,
                          std::function<void(RGPassBuilder&)> setupFn,
                          std::function<void(RHICommandList*)> executeFn)
{
	RGPassBuilder builder;
	setupFn(builder);

	RGPass pass;
	pass.sName          = name;
	pass.vRenderTargets = std::move(builder.m_vRenderTargets);
	pass.vTextureReads  = std::move(builder.m_vTextureReads);
	pass.depthOutput    = builder.m_DepthOutput;
	pass.executeFn      = std::move(executeFn);
	m_vPasses.push_back(std::move(pass));
}

void RenderGraph::Compile()
{
	m_vCompiledPasses.clear();
	m_vFinalBarriers.clear();

	// Per-texture tracking: current layout as we walk passes
	std::vector<RHITextureLayout> trackingLayout(m_vTextures.size());
	for (size_t i = 0; i < m_vTextures.size(); ++i)
		trackingLayout[i] = m_vTextures[i].currentLayout;

	for (uint32 passIdx = 0; passIdx < static_cast<uint32>(m_vPasses.size()); ++passIdx)
	{
		auto& pass = m_vPasses[passIdx];
		CompiledPass compiled;
		compiled.uPassIndex = passIdx;

		// RT outputs: need RenderTarget layout
		for (auto& rt : pass.vRenderTargets)
		{
			if (!rt.texture.IsValid() || rt.texture.index >= m_vTextures.size())
				continue;

			uint32 texIdx = rt.texture.index;
			if (trackingLayout[texIdx] != RHITextureLayout::RenderTarget)
			{
				RHITextureBarrier barrier = {};
				barrier.texture      = m_vTextures[texIdx].texture;
				barrier.syncBefore   = RHIBarrierSync::All;
				barrier.syncAfter    = RHIBarrierSync::RenderTarget;
				barrier.accessBefore = AccessFromLayout(trackingLayout[texIdx]);
				barrier.accessAfter  = RHIBarrierAccess::RenderTarget;
				barrier.layoutBefore = trackingLayout[texIdx];
				barrier.layoutAfter  = RHITextureLayout::RenderTarget;
				compiled.vBarriersBefore.push_back(barrier);
				trackingLayout[texIdx] = RHITextureLayout::RenderTarget;
			}

			compiled.vRTVs.push_back(rt.rtv);

			if (rt.bClear)
				compiled.vClears.push_back({ rt.rtv, rt.clearColor });
		}

		// Texture reads: need ShaderResource layout
		for (auto& texRef : pass.vTextureReads)
		{
			if (!texRef.IsValid() || texRef.index >= m_vTextures.size())
				continue;

			uint32 texIdx = texRef.index;
			if (trackingLayout[texIdx] != RHITextureLayout::ShaderResource)
			{
				RHITextureBarrier barrier = {};
				barrier.texture      = m_vTextures[texIdx].texture;
				barrier.syncBefore   = RHIBarrierSync::All;
				barrier.syncAfter    = RHIBarrierSync::AllShading;
				barrier.accessBefore = AccessFromLayout(trackingLayout[texIdx]);
				barrier.accessAfter  = RHIBarrierAccess::ShaderResource;
				barrier.layoutBefore = trackingLayout[texIdx];
				barrier.layoutAfter  = RHITextureLayout::ShaderResource;
				compiled.vBarriersBefore.push_back(barrier);
				trackingLayout[texIdx] = RHITextureLayout::ShaderResource;
			}
		}

		// Depth output: need DepthStencilWrite or DepthStencilRead layout
		if (pass.depthOutput.texture.IsValid() && pass.depthOutput.texture.index < m_vTextures.size())
		{
			uint32 texIdx = pass.depthOutput.texture.index;
			RHITextureLayout targetLayout = pass.depthOutput.bReadOnly
				? RHITextureLayout::DepthStencilRead
				: RHITextureLayout::DepthStencilWrite;

			if (trackingLayout[texIdx] != targetLayout)
			{
				RHITextureBarrier barrier = {};
				barrier.texture      = m_vTextures[texIdx].texture;
				barrier.syncBefore   = RHIBarrierSync::All;
				barrier.syncAfter    = RHIBarrierSync::DepthStencil;
				barrier.accessBefore = AccessFromLayout(trackingLayout[texIdx]);
				barrier.accessAfter  = pass.depthOutput.bReadOnly
					? RHIBarrierAccess::DepthStencilRead
					: RHIBarrierAccess::DepthStencilWrite;
				barrier.layoutBefore = trackingLayout[texIdx];
				barrier.layoutAfter  = targetLayout;
				compiled.vBarriersBefore.push_back(barrier);
				trackingLayout[texIdx] = targetLayout;
			}

			compiled.dsv           = pass.depthOutput.dsv;
			compiled.bClearDepth   = pass.depthOutput.bClear;
			compiled.fClearDepth   = pass.depthOutput.fClearDepth;
			compiled.uClearStencil = pass.depthOutput.uClearStencil;
		}

		m_vCompiledPasses.push_back(std::move(compiled));
	}

	// Final barriers: transition each texture back to finalLayout
	for (size_t i = 0; i < m_vTextures.size(); ++i)
	{
		if (trackingLayout[i] != m_vTextures[i].finalLayout)
		{
			RHITextureBarrier barrier = {};
			barrier.texture      = m_vTextures[i].texture;
			barrier.syncBefore   = RHIBarrierSync::All;
			barrier.syncAfter    = RHIBarrierSync::All;
			barrier.accessBefore = AccessFromLayout(trackingLayout[i]);
			barrier.accessAfter  = RHIBarrierAccess::Common;
			barrier.layoutBefore = trackingLayout[i];
			barrier.layoutAfter  = m_vTextures[i].finalLayout;
			m_vFinalBarriers.push_back(barrier);
		}
	}
}

void RenderGraph::Execute(RHIDevice* pDevice, std::vector<RHICommandList*>& outCmdLists)
{
	for (size_t i = 0; i < m_vCompiledPasses.size(); ++i)
	{
		auto& compiled = m_vCompiledPasses[i];
		auto& pass     = m_vPasses[compiled.uPassIndex];

		auto* pCmdList = pDevice->AcquireCommandList();

		// Bind GPU-visible descriptor heap for SRV/UAV/CBV descriptor tables
		pDevice->BindGpuDescriptorHeap(pCmdList);

		// Insert barriers before this pass
		if (!compiled.vBarriersBefore.empty())
			pCmdList->TextureBarrier(compiled.vBarriersBefore.data(),
			                         static_cast<uint32>(compiled.vBarriersBefore.size()));

		// Bind render targets (with optional DSV)
		if (!compiled.vRTVs.empty())
		{
			const RHIDepthStencilView* pDsv = compiled.dsv.IsValid() ? &compiled.dsv : nullptr;
			pCmdList->SetRenderTargets(compiled.vRTVs.data(),
			                           static_cast<uint32>(compiled.vRTVs.size()), pDsv);
		}
		else if (compiled.dsv.IsValid())
		{
			pCmdList->SetRenderTargets(nullptr, 0, &compiled.dsv);
		}

		// Clear render targets
		for (auto& [rtv, color] : compiled.vClears)
			pCmdList->ClearRenderTarget(rtv, color);

		// Clear depth/stencil
		if (compiled.bClearDepth && compiled.dsv.IsValid())
			pCmdList->ClearDepthStencilView(compiled.dsv, compiled.fClearDepth, compiled.uClearStencil);

		// Execute pass callback
		pass.executeFn(pCmdList);

		pCmdList->End();
		outCmdLists.push_back(pCmdList);
	}

	// Final barriers (e.g. RenderTarget → Common for present)
	if (!m_vFinalBarriers.empty())
	{
		auto* pCmdList = pDevice->AcquireCommandList();
		pCmdList->TextureBarrier(m_vFinalBarriers.data(),
		                         static_cast<uint32>(m_vFinalBarriers.size()));
		pCmdList->End();
		outCmdLists.push_back(pCmdList);
	}
}

void RenderGraph::Reset()
{
	m_vTextures.clear();
	m_vPasses.clear();
	m_vCompiledPasses.clear();
	m_vFinalBarriers.clear();
}

} // namespace Evo
