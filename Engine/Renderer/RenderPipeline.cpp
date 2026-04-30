#include "Renderer/RenderPipeline.h"
#include "Renderer/Renderer.h"
#include "Renderer/Camera.h"
#include "Asset/ShaderAsset.h"
#include "Core/Log.h"

namespace Evo {

RenderPipeline::~RenderPipeline() = default;

bool RenderPipeline::Initialize(Render* pRender)
{
	m_pRender = pRender;
	auto* pDevice = m_pRender->GetDevice();
	RHIFormat rtFormat = m_pRender->GetSwapChain()->GetFormat();

	EVO_LOG_INFO("RenderPipeline initializing...");

	// ---- Shader asset manager ----
	m_AssetManager.Initialize(pDevice);
	m_AssetManager.RegisterFactory(".hlsl", [] { return std::make_unique<ShaderAsset>(); });

	// Input layout for StaticVertex
	RHIInputElement inputElements[] = {
		{ "POSITION",  0, RHIFormat::R32G32B32_FLOAT,  0, 0 },
		{ "NORMAL",    0, RHIFormat::R32G32B32_FLOAT, 12, 0 },
		{ "TEXCOORD",  0, RHIFormat::R32G32_FLOAT,    24, 0 },
	};

	// ---- G-Buffer pipeline ----
	{
		auto shaderHandle = m_AssetManager.LoadSync("Assets/Shaders/GBuffer.hlsl");
		auto* pShader = m_AssetManager.Get<ShaderAsset>(shaderHandle);
		if (!pShader) { EVO_LOG_ERROR("RenderPipeline: failed to load GBuffer shader"); return false; }

		RHIGraphicsPipelineDesc desc = {};
		desc.vertexShader = pShader->GetVertexShader();
		desc.pixelShader  = pShader->GetPixelShader();
		desc.pInputElements = inputElements;
		desc.uInputElementCount = 3;
		desc.rasterizer.cullMode = RHICullMode::Back;
		desc.rasterizer.bFrontCounterClockwise = true;
		desc.depthStencil.bDepthTestEnable  = true;
		desc.depthStencil.bDepthWriteEnable = true;
		desc.depthStencil.depthCompareOp    = RHICompareOp::Less;
		desc.depthStencilFormat = RHIFormat::D32_FLOAT;
		desc.uRenderTargetCount = 3;
		desc.renderTargetFormats[0] = RHIFormat::R8G8B8A8_UNORM;
		desc.renderTargetFormats[1] = RHIFormat::R16G16B16A16_FLOAT;
		desc.renderTargetFormats[2] = RHIFormat::R8G8B8A8_UNORM;
		desc.topology = RHIPrimitiveTopology::TriangleList;
		desc.uPushConstantSize = sizeof(float) * 24;
		desc.sDebugName = "GBufferPSO";
		m_GBufferPipeline = pDevice->CreateGraphicsPipeline(desc);
		if (!m_GBufferPipeline.IsValid()) return false;
	}

	// ---- Shadow depth pipeline ----
	{
		auto shaderHandle = m_AssetManager.LoadSync("Assets/Shaders/ShadowDepth.hlsl");
		auto* pShader = m_AssetManager.Get<ShaderAsset>(shaderHandle);
		if (!pShader) { EVO_LOG_ERROR("RenderPipeline: failed to load ShadowDepth shader"); return false; }

		RHIInputElement shadowInput[] = {
			{ "POSITION", 0, RHIFormat::R32G32B32_FLOAT, 0, 0 },
		};

		RHIGraphicsPipelineDesc desc = {};
		desc.vertexShader = pShader->GetVertexShader();
		desc.pInputElements = shadowInput;
		desc.uInputElementCount = 1;
		desc.rasterizer.cullMode = RHICullMode::Front;
		desc.rasterizer.bFrontCounterClockwise = true;
		desc.rasterizer.depthBias = 1000;
		desc.rasterizer.fSlopeScaledDepthBias = 1.5f;
		desc.depthStencil.bDepthTestEnable  = true;
		desc.depthStencil.bDepthWriteEnable = true;
		desc.depthStencil.depthCompareOp = RHICompareOp::Less;
		desc.depthStencilFormat = RHIFormat::D32_FLOAT;
		desc.uRenderTargetCount = 0;
		desc.topology = RHIPrimitiveTopology::TriangleList;
		desc.uPushConstantSize = sizeof(float) * 16;
		desc.sDebugName = "ShadowDepthPSO";
		m_ShadowPipeline = pDevice->CreateGraphicsPipeline(desc);
		if (!m_ShadowPipeline.IsValid()) return false;
	}

	// ---- Compute light data ----
	m_vLightDir   = Vec3(0.5f, 1.0f, -0.3f).Normalized();
	m_vLightColor = Vec3(1.0f, 0.98f, 0.92f);
	Vec3 lightPos  = m_vLightDir * 15.0f;
	Mat4 lightView = Mat4::LookAtLH(lightPos, Vec3::Zero, Vec3(0, 1, 0));
	Mat4 lightProj = Mat4::OrthographicLH(20.0f, 20.0f, 0.1f, 40.0f);
	m_LightViewProj = lightView * lightProj;

	// ---- Descriptor set layouts ----
	{
		RHIDescriptorBinding bindings[] = {
			{ 0, RHIDescriptorType::ShaderResource, 1, RHIShaderStage::Pixel },
			{ 1, RHIDescriptorType::ShaderResource, 1, RHIShaderStage::Pixel },
			{ 2, RHIDescriptorType::ShaderResource, 1, RHIShaderStage::Pixel },
			{ 3, RHIDescriptorType::ShaderResource, 1, RHIShaderStage::Pixel },
			{ 4, RHIDescriptorType::ShaderResource, 1, RHIShaderStage::Pixel },
		};
		RHIDescriptorSetLayoutDesc layoutDesc;
		layoutDesc.pBindings = bindings;
		layoutDesc.uBindingCount = 5;
		m_LightingSetLayout = pDevice->CreateDescriptorSetLayout(layoutDesc);
	}
	{
		RHIDescriptorBinding bindings[] = {
			{ 0, RHIDescriptorType::ShaderResource, 1, RHIShaderStage::Pixel },
		};
		RHIDescriptorSetLayoutDesc layoutDesc;
		layoutDesc.pBindings = bindings;
		layoutDesc.uBindingCount = 1;
		m_PostProcessSetLayout = pDevice->CreateDescriptorSetLayout(layoutDesc);
	}
	{
		RHIDescriptorBinding bindings[] = {
			{ 0, RHIDescriptorType::ShaderResource, 1, RHIShaderStage::Pixel },
		};
		RHIDescriptorSetLayoutDesc layoutDesc;
		layoutDesc.pBindings = bindings;
		layoutDesc.uBindingCount = 1;
		m_TransShadowSetLayout = pDevice->CreateDescriptorSetLayout(layoutDesc);
	}

	// ---- Deferred lighting pipeline ----
	{
		auto shaderHandle = m_AssetManager.LoadSync("Assets/Shaders/DeferredLighting.hlsl");
		auto* pShader = m_AssetManager.Get<ShaderAsset>(shaderHandle);
		if (!pShader) { EVO_LOG_ERROR("RenderPipeline: failed to load DeferredLighting shader"); return false; }

		RHIGraphicsPipelineDesc desc = {};
		desc.vertexShader = pShader->GetVertexShader();
		desc.pixelShader  = pShader->GetPixelShader();
		desc.rasterizer.cullMode = RHICullMode::None;
		desc.depthStencil.bDepthTestEnable  = false;
		desc.depthStencil.bDepthWriteEnable = false;
		desc.uRenderTargetCount = 1;
		desc.renderTargetFormats[0] = RHIFormat::R16G16B16A16_FLOAT;
		desc.topology = RHIPrimitiveTopology::TriangleList;
		desc.uPushConstantSize = sizeof(LightingPushConstants);
		desc.descriptorSetLayouts[0] = m_LightingSetLayout;
		desc.uDescriptorSetLayoutCount = 1;
		desc.sDebugName = "DeferredLightingPSO";
		m_LightingPipeline = pDevice->CreateGraphicsPipeline(desc);
		if (!m_LightingPipeline.IsValid()) return false;
	}

	// ---- Post-processing pipeline ----
	{
		auto shaderHandle = m_AssetManager.LoadSync("Assets/Shaders/PostProcess.hlsl");
		auto* pShader = m_AssetManager.Get<ShaderAsset>(shaderHandle);
		if (!pShader) { EVO_LOG_ERROR("RenderPipeline: failed to load PostProcess shader"); return false; }

		RHIGraphicsPipelineDesc desc = {};
		desc.vertexShader = pShader->GetVertexShader();
		desc.pixelShader  = pShader->GetPixelShader();
		desc.rasterizer.cullMode = RHICullMode::None;
		desc.depthStencil.bDepthTestEnable  = false;
		desc.depthStencil.bDepthWriteEnable = false;
		desc.uRenderTargetCount = 1;
		desc.renderTargetFormats[0] = rtFormat;
		desc.topology = RHIPrimitiveTopology::TriangleList;
		desc.uPushConstantSize = 0;
		desc.descriptorSetLayouts[0] = m_PostProcessSetLayout;
		desc.uDescriptorSetLayoutCount = 1;
		desc.sDebugName = "PostProcessPSO";
		m_PostProcessPipeline = pDevice->CreateGraphicsPipeline(desc);
		if (!m_PostProcessPipeline.IsValid()) return false;
	}

	// ---- Forward transparent pipeline ----
	{
		auto shaderHandle = m_AssetManager.LoadSync("Assets/Shaders/ForwardTransparent.hlsl");
		auto* pShader = m_AssetManager.Get<ShaderAsset>(shaderHandle);
		if (!pShader) { EVO_LOG_ERROR("RenderPipeline: failed to load ForwardTransparent shader"); return false; }

		RHIGraphicsPipelineDesc desc = {};
		desc.vertexShader = pShader->GetVertexShader();
		desc.pixelShader  = pShader->GetPixelShader();
		desc.pInputElements = inputElements;
		desc.uInputElementCount = 3;
		desc.rasterizer.cullMode = RHICullMode::Back;
		desc.rasterizer.bFrontCounterClockwise = true;
		desc.depthStencil.bDepthTestEnable  = true;
		desc.depthStencil.bDepthWriteEnable = false;
		desc.depthStencil.depthCompareOp = RHICompareOp::Less;
		desc.depthStencilFormat = RHIFormat::D32_FLOAT;
		desc.uRenderTargetCount = 1;
		desc.renderTargetFormats[0] = RHIFormat::R16G16B16A16_FLOAT;
		desc.blendTargets[0].bBlendEnable = true;
		desc.blendTargets[0].srcColor = RHIBlendFactor::SrcAlpha;
		desc.blendTargets[0].dstColor = RHIBlendFactor::InvSrcAlpha;
		desc.blendTargets[0].colorOp  = RHIBlendOp::Add;
		desc.blendTargets[0].srcAlpha = RHIBlendFactor::One;
		desc.blendTargets[0].dstAlpha = RHIBlendFactor::InvSrcAlpha;
		desc.blendTargets[0].alphaOp  = RHIBlendOp::Add;
		desc.topology = RHIPrimitiveTopology::TriangleList;
		desc.uPushConstantSize = sizeof(TransparentPushConstants);
		desc.descriptorSetLayouts[0] = m_TransShadowSetLayout;
		desc.uDescriptorSetLayoutCount = 1;
		desc.sDebugName = "ForwardTransparentPSO";
		m_TransparentPipeline = pDevice->CreateGraphicsPipeline(desc);
		if (!m_TransparentPipeline.IsValid()) return false;
	}

	// ---- Create shadow map ----
	{
		RHITextureDesc shadowDesc{};
		shadowDesc.uWidth  = kShadowMapSize;
		shadowDesc.uHeight = kShadowMapSize;
		shadowDesc.format  = RHIFormat::D32_FLOAT;
		shadowDesc.usage   = RHITextureUsage::DepthStencil | RHITextureUsage::ShaderResource;
		shadowDesc.sDebugName = "ShadowMap";
		m_ShadowTexture = pDevice->CreateTexture(shadowDesc);
		m_ShadowDSV     = pDevice->CreateDepthStencilView(m_ShadowTexture);
	}

	EVO_LOG_INFO("RenderPipeline initialized");
	return true;
}

void RenderPipeline::Shutdown()
{
	auto* pDevice = m_pRender->GetDevice();

	if (m_ShadowDSV.IsValid())     pDevice->DestroyDepthStencilView(m_ShadowDSV);
	if (m_ShadowTexture.IsValid()) pDevice->DestroyTexture(m_ShadowTexture);

	if (m_TransparentPipeline.IsValid())  pDevice->DestroyPipeline(m_TransparentPipeline);
	if (m_PostProcessPipeline.IsValid())  pDevice->DestroyPipeline(m_PostProcessPipeline);
	if (m_LightingPipeline.IsValid())     pDevice->DestroyPipeline(m_LightingPipeline);
	if (m_ShadowPipeline.IsValid())       pDevice->DestroyPipeline(m_ShadowPipeline);
	if (m_GBufferPipeline.IsValid())      pDevice->DestroyPipeline(m_GBufferPipeline);

	if (m_TransShadowSetLayout.IsValid()) pDevice->DestroyDescriptorSetLayout(m_TransShadowSetLayout);
	if (m_PostProcessSetLayout.IsValid()) pDevice->DestroyDescriptorSetLayout(m_PostProcessSetLayout);
	if (m_LightingSetLayout.IsValid())    pDevice->DestroyDescriptorSetLayout(m_LightingSetLayout);

	for (auto& vp : m_vViewports)
		vp.Shutdown();
	m_vViewports.clear();

	m_AssetManager.Shutdown();

	EVO_LOG_INFO("RenderPipeline shut down");
}

ViewportFrameDesc RenderPipeline::MakeViewportFrameDesc(
	uint32 uWidth, uint32 uHeight, const std::string& sName) const
{
	ViewportFrameDesc desc;
	desc.sDebugName = sName;
	desc.uWidth     = uWidth;
	desc.uHeight    = uHeight;
	desc.lightingSetLayout          = m_LightingSetLayout;
	desc.postProcessSetLayout       = m_PostProcessSetLayout;
	desc.transparentShadowSetLayout = m_TransShadowSetLayout;
	desc.shadowTexture              = m_ShadowTexture;
	return desc;
}

// ============================================================================
// Internal passes
// ============================================================================

void RenderPipeline::RenderShadow()
{
	auto& rg = m_pRender->GetRenderGraph();
	m_ShadowRG = rg.ImportTexture("ShadowMap", m_ShadowTexture,
		RHITextureLayout::Common, RHITextureLayout::Common);

	m_SceneRenderer.RenderShadowMap(*m_pScene, m_pRender, m_ShadowPipeline,
		m_LightViewProj, m_ShadowRG, m_ShadowDSV,
		static_cast<float>(kShadowMapSize));
}

void RenderPipeline::RenderViewport(ViewportFrame& viewport, const Mat4& viewProj)
{
	RenderViewport(viewport, viewProj,
		m_pRender->GetBackBufferRG(),
		m_pRender->GetSwapChain()->GetCurrentBackBufferRTV());
}

void RenderPipeline::RenderViewport(ViewportFrame& viewport, const Mat4& viewProj,
									RGHandle outputTarget, RHIRenderTargetView outputRTV)
{
	auto& rg = m_pRender->GetRenderGraph();
	float w = static_cast<float>(viewport.GetWidth());
	float h = static_cast<float>(viewport.GetHeight());

	auto imported = viewport.ImportToRenderGraph(rg);

	// G-Buffer
	m_SceneRenderer.RenderGBuffer(*m_pScene, m_pRender, m_GBufferPipeline,
		viewProj, imported.gbTargets, w, h);

	// Deferred lighting
	LightingPushConstants lightPC = {};
	lightPC.invViewProj    = viewProj.Inverse();
	lightPC.lightViewProj  = m_LightViewProj;
	lightPC.vLightDir[0]   = m_vLightDir.x;
	lightPC.vLightDir[1]   = m_vLightDir.y;
	lightPC.vLightDir[2]   = m_vLightDir.z;
	lightPC.fShadowMapSize = static_cast<float>(kShadowMapSize);
	lightPC.vLightColor[0] = m_vLightColor.x;
	lightPC.vLightColor[1] = m_vLightColor.y;
	lightPC.vLightColor[2] = m_vLightColor.z;

	m_SceneRenderer.AddLightingPass(m_pRender, m_LightingPipeline,
		viewport.GetLightingDescSet(), imported.gbTargets, m_ShadowRG,
		imported.hdrTexture, imported.hdrRTV, lightPC, w, h);

	// Forward transparent
	TransparentPushConstants transPC = {};
	transPC.lightViewProj  = m_LightViewProj;
	transPC.vLightDir[0]   = m_vLightDir.x;
	transPC.vLightDir[1]   = m_vLightDir.y;
	transPC.vLightDir[2]   = m_vLightDir.z;
	transPC.fShadowMapSize = static_cast<float>(kShadowMapSize);
	transPC.vLightColor[0] = m_vLightColor.x;
	transPC.vLightColor[1] = m_vLightColor.y;
	transPC.vLightColor[2] = m_vLightColor.z;

	m_SceneRenderer.RenderForwardTransparent(*m_pScene, m_pRender, m_TransparentPipeline,
		viewport.GetTransparentShadowDescSet(), viewProj, transPC,
		imported.hdrTexture, imported.hdrRTV,
		imported.gbTargets.depthTexture, imported.gbTargets.depthDSV,
		m_ShadowRG, w, h);

	// Post-processing
	m_SceneRenderer.AddPostProcessPass(m_pRender, m_PostProcessPipeline,
		viewport.GetPostProcessDescSet(), imported.hdrTexture,
		outputTarget, outputRTV, w, h);
}

// ============================================================================
// Frame entry + Viewport management
// ============================================================================

ViewportFrame* RenderPipeline::GetViewport(size_t i)
{
	return i < m_vViewports.size() ? &m_vViewports[i] : nullptr;
}

void RenderPipeline::SetupViewports()
{
	if (m_vViewports.empty() && m_pScene)
	{
		auto cameras = m_pScene->GetCameraEntities();

		// Create one viewport per camera, or one default if no cameras
		if (cameras.empty())
		{
			auto& vp = m_vViewports.emplace_back();
			auto desc = MakeViewportFrameDesc(
				m_pRender->GetSwapChain()->GetWidth(),
				m_pRender->GetSwapChain()->GetHeight(), "Main");
			vp.Initialize(m_pRender, desc);
		}
		else
		{
			for (auto cam : cameras)
			{
				auto& vp = m_vViewports.emplace_back();
				auto desc = MakeViewportFrameDesc(
					m_pRender->GetSwapChain()->GetWidth(),
					m_pRender->GetSwapChain()->GetHeight(), "Viewport");
				vp.Initialize(m_pRender, desc);
				vp.SetCameraEntity(cam);
			}
		}
	}
}

void RenderPipeline::RenderFrame()
{
	SetupViewports();
	RenderShadow();

	for (size_t i = 0; i < m_vViewports.size(); ++i)
	{
		auto& vp = m_vViewports[i];
		auto camEntity = vp.GetCameraEntity();
		Mat4 viewProj;

		if (camEntity.IsValid() && m_pScene)
		{
			auto* pTransform = m_pScene->Transforms().Get(camEntity);
			auto* pCamComp   = m_pScene->Cameras().Get(camEntity);
			if (pTransform && pCamComp)
			{
				float aspect = vp.GetHeight() > 0
					? static_cast<float>(vp.GetWidth()) / vp.GetHeight()
					: 1.0f;
				Camera cam = BuildCameraFromEntity(*pTransform, *pCamComp, aspect);
				viewProj = cam.GetViewProjectionMatrix();
			}
		}

		if (i == 0)
			RenderViewport(vp, viewProj);                         // → swap chain
		else
		{
			auto& rg = m_pRender->GetRenderGraph();
			auto outRG = rg.ImportTexture(
				("ViewportOut_" + std::to_string(i)).c_str(),
				vp.GetOutputTexture(),
				RHITextureLayout::Common, RHITextureLayout::Common);
			RenderViewport(vp, viewProj, outRG, vp.GetOutputRTV());  // → own RT
		}
	}
}

} // namespace Evo
