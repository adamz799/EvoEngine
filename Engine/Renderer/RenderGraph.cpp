#include "Renderer/RenderGraph.h"
#include "Core/Log.h"

namespace Evo {

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
				barrier.accessBefore = RHIBarrierAccess::Common;
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
				barrier.accessBefore = RHIBarrierAccess::Common;
				barrier.accessAfter  = RHIBarrierAccess::ShaderResource;
				barrier.layoutBefore = trackingLayout[texIdx];
				barrier.layoutAfter  = RHITextureLayout::ShaderResource;
				compiled.vBarriersBefore.push_back(barrier);
				trackingLayout[texIdx] = RHITextureLayout::ShaderResource;
			}
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
			barrier.syncBefore   = RHIBarrierSync::RenderTarget;
			barrier.syncAfter    = RHIBarrierSync::All;
			barrier.accessBefore = RHIBarrierAccess::RenderTarget;
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

		// Insert barriers before this pass
		if (!compiled.vBarriersBefore.empty())
			pCmdList->TextureBarrier(compiled.vBarriersBefore.data(),
			                         static_cast<uint32>(compiled.vBarriersBefore.size()));

		// Bind render targets
		if (!compiled.vRTVs.empty())
			pCmdList->SetRenderTargets(compiled.vRTVs.data(),
			                           static_cast<uint32>(compiled.vRTVs.size()));

		// Clear render targets
		for (auto& [rtv, color] : compiled.vClears)
			pCmdList->ClearRenderTarget(rtv, color);

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
