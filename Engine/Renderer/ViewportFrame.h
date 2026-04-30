#pragma once

#include "RHI/RHI.h"
#include "Renderer/RenderGraph.h"
#include "Renderer/SceneRenderer.h"
#include <string>

namespace Evo {

struct ViewportFrameDesc {
	std::string sDebugName;
	uint32 uWidth  = 0;
	uint32 uHeight = 0;
	RHIDescriptorSetLayoutHandle lightingSetLayout;
	RHIDescriptorSetLayoutHandle postProcessSetLayout;
	RHIDescriptorSetLayoutHandle transparentShadowSetLayout;
	RHITextureHandle             shadowTexture;   // shared, not owned
};

/// Per-viewport intermediate rendering resources.
/// Owns GBuffer (3 MRT), depth buffer, HDR intermediate, and descriptor sets.
class ViewportFrame {
public:
	ViewportFrame() = default;
	~ViewportFrame() = default;

	void Initialize(RHIDevice* pDevice, const ViewportFrameDesc& desc);
	void Shutdown(RHIDevice* pDevice);
	void Resize(RHIDevice* pDevice, uint32 uWidth, uint32 uHeight);

	uint32 GetWidth()  const { return m_uWidth; }
	uint32 GetHeight() const { return m_uHeight; }

	/// Imported render graph targets for one frame.
	struct ImportedTargets {
		GBufferTargets      gbTargets;
		RGHandle            hdrTexture;
		RHIRenderTargetView hdrRTV;
	};

	/// Import all textures into the render graph and return handles.
	ImportedTargets ImportToRenderGraph(RenderGraph& rg) const;

	// Descriptor set accessors
	RHIDescriptorSetHandle GetLightingDescSet()         const { return m_LightingDescSet; }
	RHIDescriptorSetHandle GetPostProcessDescSet()      const { return m_PostProcessDescSet; }
	RHIDescriptorSetHandle GetTransparentShadowDescSet() const { return m_TransShadowDescSet; }

	// Resource accessors
	RHITextureHandle    GetDepthTexture()  const { return m_DepthTexture; }
	RHIDepthStencilView GetDepthDSV()      const { return m_DepthDSV; }
	RHITextureHandle    GetHDRTexture()    const { return m_HDRTexture; }
	RHIRenderTargetView GetHDRRTV()        const { return m_HDRRTV; }

private:
	void CreateResources(RHIDevice* pDevice);
	void DestroyResources(RHIDevice* pDevice);
	void WriteDescriptorSets(RHIDevice* pDevice);

	std::string m_sDebugName;
	uint32 m_uWidth  = 0;
	uint32 m_uHeight = 0;

	// G-Buffer
	RHITextureHandle    m_GBAlbedoTexture;
	RHIRenderTargetView m_GBAlbedoRTV;
	RHITextureHandle    m_GBNormalTexture;
	RHIRenderTargetView m_GBNormalRTV;
	RHITextureHandle    m_GBRoughMetTexture;
	RHIRenderTargetView m_GBRoughMetRTV;

	// Depth
	RHITextureHandle    m_DepthTexture;
	RHIDepthStencilView m_DepthDSV;

	// HDR intermediate
	RHITextureHandle    m_HDRTexture;
	RHIRenderTargetView m_HDRRTV;

	// Descriptor sets
	RHIDescriptorSetHandle m_LightingDescSet;
	RHIDescriptorSetHandle m_PostProcessDescSet;
	RHIDescriptorSetHandle m_TransShadowDescSet;

	// Stored for resize / descriptor writes
	RHIDescriptorSetLayoutHandle m_LightingSetLayout;
	RHIDescriptorSetLayoutHandle m_PostProcessSetLayout;
	RHIDescriptorSetLayoutHandle m_TransShadowSetLayout;
	RHITextureHandle             m_ShadowTexture;   // not owned
};

} // namespace Evo

