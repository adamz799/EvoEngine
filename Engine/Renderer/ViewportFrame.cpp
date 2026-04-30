#include "Renderer/ViewportFrame.h"
#include "Renderer/Renderer.h"
#include "Core/Log.h"

namespace Evo {

void ViewportFrame::Initialize(Render* pRender, const ViewportFrameDesc& desc)
{
	m_pRender = pRender;
	auto* pDevice = pRender->GetDevice();

	m_sDebugName = desc.sDebugName;
	m_uWidth     = desc.uWidth;
	m_uHeight    = desc.uHeight;

	m_LightingSetLayout    = desc.lightingSetLayout;
	m_PostProcessSetLayout = desc.postProcessSetLayout;
	m_TransShadowSetLayout = desc.transparentShadowSetLayout;
	m_ShadowTexture        = desc.shadowTexture;

	// Allocate descriptor sets
	m_LightingDescSet    = pDevice->AllocateDescriptorSet(m_LightingSetLayout);
	m_PostProcessDescSet = pDevice->AllocateDescriptorSet(m_PostProcessSetLayout);
	m_TransShadowDescSet = pDevice->AllocateDescriptorSet(m_TransShadowSetLayout);

	CreateResources(pDevice);
	WriteDescriptorSets(pDevice);

	EVO_LOG_INFO("ViewportFrame '{}' initialized: {}x{}", m_sDebugName, m_uWidth, m_uHeight);
}

void ViewportFrame::Shutdown()
{
	auto* pDevice = m_pRender->GetDevice();
	DestroyResources(pDevice);

	if (m_TransShadowDescSet.IsValid()) pDevice->FreeDescriptorSet(m_TransShadowDescSet);
	if (m_PostProcessDescSet.IsValid()) pDevice->FreeDescriptorSet(m_PostProcessDescSet);
	if (m_LightingDescSet.IsValid())    pDevice->FreeDescriptorSet(m_LightingDescSet);

	m_TransShadowDescSet = {};
	m_PostProcessDescSet = {};
	m_LightingDescSet    = {};
}

void ViewportFrame::Resize(uint32 uWidth, uint32 uHeight)
{
	auto* pDevice = m_pRender->GetDevice();
	if (uWidth == m_uWidth && uHeight == m_uHeight)
		return;
	if (uWidth == 0 || uHeight == 0)
		return;

	m_uWidth  = uWidth;
	m_uHeight = uHeight;

	DestroyResources(pDevice);
	CreateResources(pDevice);
	WriteDescriptorSets(pDevice);

	EVO_LOG_DEBUG("ViewportFrame '{}' resized: {}x{}", m_sDebugName, m_uWidth, m_uHeight);
}

void ViewportFrame::CreateResources(RHIDevice* pDevice)
{
	// G-Buffer Albedo
	RHITextureDesc albedoDesc{};
	albedoDesc.uWidth     = m_uWidth;
	albedoDesc.uHeight    = m_uHeight;
	albedoDesc.format     = RHIFormat::R8G8B8A8_UNORM;
	albedoDesc.usage      = RHITextureUsage::RenderTarget | RHITextureUsage::ShaderResource;
	albedoDesc.sDebugName = m_sDebugName + "_GBuffer_Albedo";
	albedoDesc.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_GBAlbedoTexture = pDevice->CreateTexture(albedoDesc);
	m_GBAlbedoRTV     = pDevice->CreateRenderTargetView(m_GBAlbedoTexture);

	// G-Buffer Normal
	RHITextureDesc normalDesc{};
	normalDesc.uWidth     = m_uWidth;
	normalDesc.uHeight    = m_uHeight;
	normalDesc.format     = RHIFormat::R16G16B16A16_FLOAT;
	normalDesc.usage      = RHITextureUsage::RenderTarget | RHITextureUsage::ShaderResource;
	normalDesc.sDebugName = m_sDebugName + "_GBuffer_Normal";
	normalDesc.clearColor = { 0.5f, 0.5f, 1.0f, 0.0f };
	m_GBNormalTexture = pDevice->CreateTexture(normalDesc);
	m_GBNormalRTV     = pDevice->CreateRenderTargetView(m_GBNormalTexture);

	// G-Buffer RoughMet
	RHITextureDesc roughMetDesc{};
	roughMetDesc.uWidth     = m_uWidth;
	roughMetDesc.uHeight    = m_uHeight;
	roughMetDesc.format     = RHIFormat::R8G8B8A8_UNORM;
	roughMetDesc.usage      = RHITextureUsage::RenderTarget | RHITextureUsage::ShaderResource;
	roughMetDesc.sDebugName = m_sDebugName + "_GBuffer_RoughMet";
	roughMetDesc.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_GBRoughMetTexture = pDevice->CreateTexture(roughMetDesc);
	m_GBRoughMetRTV     = pDevice->CreateRenderTargetView(m_GBRoughMetTexture);

	// Depth
	RHITextureDesc depthDesc{};
	depthDesc.uWidth     = m_uWidth;
	depthDesc.uHeight    = m_uHeight;
	depthDesc.format     = RHIFormat::D32_FLOAT;
	depthDesc.usage      = RHITextureUsage::DepthStencil | RHITextureUsage::ShaderResource;
	depthDesc.sDebugName = m_sDebugName + "_Depth";
	m_DepthTexture = pDevice->CreateTexture(depthDesc);
	m_DepthDSV     = pDevice->CreateDepthStencilView(m_DepthTexture);

	// HDR intermediate
	RHITextureDesc hdrDesc{};
	hdrDesc.uWidth     = m_uWidth;
	hdrDesc.uHeight    = m_uHeight;
	hdrDesc.format     = RHIFormat::R16G16B16A16_FLOAT;
	hdrDesc.usage      = RHITextureUsage::RenderTarget | RHITextureUsage::ShaderResource;
	hdrDesc.sDebugName = m_sDebugName + "_HDR";
	hdrDesc.clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_HDRTexture = pDevice->CreateTexture(hdrDesc);
	m_HDRRTV     = pDevice->CreateRenderTargetView(m_HDRTexture);
}

void ViewportFrame::DestroyResources(RHIDevice* pDevice)
{
	if (m_HDRRTV.IsValid())        pDevice->DestroyRenderTargetView(m_HDRRTV);
	if (m_HDRTexture.IsValid())    pDevice->DestroyTexture(m_HDRTexture);

	if (m_DepthDSV.IsValid())      pDevice->DestroyDepthStencilView(m_DepthDSV);
	if (m_DepthTexture.IsValid())  pDevice->DestroyTexture(m_DepthTexture);

	if (m_GBRoughMetRTV.IsValid())     pDevice->DestroyRenderTargetView(m_GBRoughMetRTV);
	if (m_GBRoughMetTexture.IsValid()) pDevice->DestroyTexture(m_GBRoughMetTexture);

	if (m_GBNormalRTV.IsValid())       pDevice->DestroyRenderTargetView(m_GBNormalRTV);
	if (m_GBNormalTexture.IsValid())   pDevice->DestroyTexture(m_GBNormalTexture);

	if (m_GBAlbedoRTV.IsValid())       pDevice->DestroyRenderTargetView(m_GBAlbedoRTV);
	if (m_GBAlbedoTexture.IsValid())   pDevice->DestroyTexture(m_GBAlbedoTexture);

	m_HDRRTV = {};  m_HDRTexture = {};
	m_DepthDSV = {};  m_DepthTexture = {};
	m_GBRoughMetRTV = {};  m_GBRoughMetTexture = {};
	m_GBNormalRTV = {};  m_GBNormalTexture = {};
	m_GBAlbedoRTV = {};  m_GBAlbedoTexture = {};
}

void ViewportFrame::WriteDescriptorSets(RHIDevice* pDevice)
{
	// Lighting descriptor set: 5 SRVs
	{
		RHIDescriptorWrite writes[5] = {};
		writes[0].uBinding = 0;
		writes[0].type     = RHIDescriptorType::ShaderResource;
		writes[0].texture  = m_GBAlbedoTexture;
		writes[1].uBinding = 1;
		writes[1].type     = RHIDescriptorType::ShaderResource;
		writes[1].texture  = m_GBNormalTexture;
		writes[2].uBinding = 2;
		writes[2].type     = RHIDescriptorType::ShaderResource;
		writes[2].texture  = m_GBRoughMetTexture;
		writes[3].uBinding = 3;
		writes[3].type     = RHIDescriptorType::ShaderResource;
		writes[3].texture  = m_DepthTexture;
		writes[4].uBinding = 4;
		writes[4].type     = RHIDescriptorType::ShaderResource;
		writes[4].texture  = m_ShadowTexture;
		pDevice->WriteDescriptorSet(m_LightingDescSet, writes, 5);
	}

	// Post-process descriptor set: 1 SRV
	{
		RHIDescriptorWrite write = {};
		write.uBinding = 0;
		write.type     = RHIDescriptorType::ShaderResource;
		write.texture  = m_HDRTexture;
		pDevice->WriteDescriptorSet(m_PostProcessDescSet, &write, 1);
	}

	// Transparent shadow descriptor set: 1 SRV
	{
		RHIDescriptorWrite write = {};
		write.uBinding = 0;
		write.type     = RHIDescriptorType::ShaderResource;
		write.texture  = m_ShadowTexture;
		pDevice->WriteDescriptorSet(m_TransShadowDescSet, &write, 1);
	}
}

ViewportFrame::ImportedTargets ViewportFrame::ImportToRenderGraph(RenderGraph& rg) const
{
	ImportedTargets targets;

	// Import G-Buffer textures
	RGHandle albedoRG = rg.ImportTexture(
		(m_sDebugName + "_GBuffer_Albedo").c_str(), m_GBAlbedoTexture,
		RHITextureLayout::Common, RHITextureLayout::Common);
	RGHandle normalRG = rg.ImportTexture(
		(m_sDebugName + "_GBuffer_Normal").c_str(), m_GBNormalTexture,
		RHITextureLayout::Common, RHITextureLayout::Common);
	RGHandle roughMetRG = rg.ImportTexture(
		(m_sDebugName + "_GBuffer_RoughMet").c_str(), m_GBRoughMetTexture,
		RHITextureLayout::Common, RHITextureLayout::Common);

	// Import depth
	RGHandle depthRG = rg.ImportTexture(
		(m_sDebugName + "_Depth").c_str(), m_DepthTexture,
		RHITextureLayout::Common, RHITextureLayout::Common);

	// Import HDR intermediate
	RGHandle hdrRG = rg.ImportTexture(
		(m_sDebugName + "_HDR").c_str(), m_HDRTexture,
		RHITextureLayout::Common, RHITextureLayout::Common);

	// Build GBufferTargets
	targets.gbTargets.albedoTexture   = albedoRG;
	targets.gbTargets.normalTexture   = normalRG;
	targets.gbTargets.roughMetTexture = roughMetRG;
	targets.gbTargets.depthTexture    = depthRG;
	targets.gbTargets.albedoRTV       = m_GBAlbedoRTV;
	targets.gbTargets.normalRTV       = m_GBNormalRTV;
	targets.gbTargets.roughMetRTV     = m_GBRoughMetRTV;
	targets.gbTargets.depthDSV        = m_DepthDSV;

	targets.hdrTexture = hdrRG;
	targets.hdrRTV     = m_HDRRTV;

	return targets;
}

} // namespace Evo

