#include "TestScene.h"
#include "Asset/ShaderAsset.h"
#include "Asset/TextureAsset.h"
#include "Scene/MeshWriter.h"
#include "Scene/MaterialAsset.h"
#include "Scene/MaterialWriter.h"
#include "Scene/PrefabAsset.h"
#include "Scene/PrefabWriter.h"
#include "Scene/SceneWriter.h"
#include "Scene/SceneLoader.h"
#include "Renderer/Renderer.h"
#include "Core/Log.h"

#include <cstring>
#include <cmath>
#include <filesystem>

namespace Evo {

struct StaticVertex {
	float position[3];
	float normal[3];
	float uv[2];
};

static void GenerateCubeData(std::vector<StaticVertex>& outVertices, std::vector<uint32>& outIndices)
{
	struct FaceData {
		float nx, ny, nz;
		float verts[4][3];
		float uvs[4][2];
	};

	const float s = 0.5f;

	FaceData faces[] = {
		{ 0, 0, 1, {{ -s,-s, s }, {  s,-s, s }, {  s, s, s }, { -s, s, s }}, {{ 0,1 }, { 1,1 }, { 1,0 }, { 0,0 }} },
		{ 0, 0,-1, {{  s,-s,-s }, { -s,-s,-s }, { -s, s,-s }, {  s, s,-s }}, {{ 0,1 }, { 1,1 }, { 1,0 }, { 0,0 }} },
		{ 1, 0, 0, {{  s,-s, s }, {  s,-s,-s }, {  s, s,-s }, {  s, s, s }}, {{ 0,1 }, { 1,1 }, { 1,0 }, { 0,0 }} },
		{-1, 0, 0, {{ -s,-s,-s }, { -s,-s, s }, { -s, s, s }, { -s, s,-s }}, {{ 0,1 }, { 1,1 }, { 1,0 }, { 0,0 }} },
		{ 0, 1, 0, {{ -s, s, s }, {  s, s, s }, {  s, s,-s }, { -s, s,-s }}, {{ 0,1 }, { 1,1 }, { 1,0 }, { 0,0 }} },
		{ 0,-1, 0, {{ -s,-s,-s }, {  s,-s,-s }, {  s,-s, s }, { -s,-s, s }}, {{ 0,1 }, { 1,1 }, { 1,0 }, { 0,0 }} },
	};

	for (int f = 0; f < 6; ++f)
	{
		uint32 baseVertex = static_cast<uint32>(outVertices.size());
		for (int v = 0; v < 4; ++v)
		{
			StaticVertex vert;
			vert.position[0] = faces[f].verts[v][0];
			vert.position[1] = faces[f].verts[v][1];
			vert.position[2] = faces[f].verts[v][2];
			vert.normal[0]   = faces[f].nx;
			vert.normal[1]   = faces[f].ny;
			vert.normal[2]   = faces[f].nz;
			vert.uv[0]       = faces[f].uvs[v][0];
			vert.uv[1]       = faces[f].uvs[v][1];
			outVertices.push_back(vert);
		}
		outIndices.push_back(baseVertex + 0);
		outIndices.push_back(baseVertex + 1);
		outIndices.push_back(baseVertex + 2);
		outIndices.push_back(baseVertex + 0);
		outIndices.push_back(baseVertex + 2);
		outIndices.push_back(baseVertex + 3);
	}
}

bool TestScene::Initialize(RHIDevice* pDevice, RHIFormat rtFormat)
{
#if EVO_RHI_DX12
	// ---- Initialize asset manager ----
	m_AssetManager.Initialize(pDevice);
	m_AssetManager.RegisterFactory(".hlsl",    [] { return std::make_unique<ShaderAsset>(); });
	m_AssetManager.RegisterFactory(".emesh",   [] { return std::make_unique<MeshAsset>(); });
	m_AssetManager.RegisterFactory(".eprefab",   [] { return std::make_unique<PrefabAsset>(); });
	m_AssetManager.RegisterFactory(".ematerial", [] { return std::make_unique<MaterialAsset>(); });
	m_AssetManager.RegisterFactory(".png",     [] { return std::make_unique<TextureAsset>(); });
	m_AssetManager.RegisterFactory(".jpg",     [] { return std::make_unique<TextureAsset>(); });
	m_AssetManager.RegisterFactory(".tga",     [] { return std::make_unique<TextureAsset>(); });

	// ---- Load shader via asset system ----
	m_ShaderHandle = m_AssetManager.LoadSync("Assets/Shaders/StaticMesh.hlsl");
	auto* pShader = m_AssetManager.Get<ShaderAsset>(m_ShaderHandle);
	if (!pShader)
	{
		EVO_LOG_ERROR("TestScene: failed to load shader");
		return false;
	}

	// Input layout matching StaticVertex
	RHIInputElement inputElements[] = {
		{ "POSITION",  0, RHIFormat::R32G32B32_FLOAT, 0,  0 },
		{ "NORMAL",    0, RHIFormat::R32G32B32_FLOAT, 12, 0 },
		{ "TEXCOORD",  0, RHIFormat::R32G32_FLOAT,    24, 0 },
	};

	// Pipeline with push constants for MVP matrix
	RHIGraphicsPipelineDesc pipelineDesc = {};
	pipelineDesc.vertexShader         = pShader->GetVertexShader();
	pipelineDesc.pixelShader          = pShader->GetPixelShader();
	pipelineDesc.pInputElements       = inputElements;
	pipelineDesc.uInputElementCount   = 3;
	pipelineDesc.rasterizer.cullMode  = RHICullMode::Back;
	pipelineDesc.rasterizer.bFrontCounterClockwise = true;
	pipelineDesc.depthStencil.bDepthTestEnable  = true;
	pipelineDesc.depthStencil.bDepthWriteEnable = true;
	pipelineDesc.depthStencil.depthCompareOp    = RHICompareOp::Less;
	pipelineDesc.depthStencilFormat             = RHIFormat::D32_FLOAT;
	pipelineDesc.uRenderTargetCount   = 1;
	pipelineDesc.renderTargetFormats[0] = rtFormat;
	pipelineDesc.topology             = RHIPrimitiveTopology::TriangleList;
	pipelineDesc.uPushConstantSize    = sizeof(float) * 16;  // 4x4 matrix
	pipelineDesc.sDebugName           = "StaticMeshPSO";
	m_Pipeline = pDevice->CreateGraphicsPipeline(pipelineDesc);

	if (!m_Pipeline.IsValid())
		return false;

	// ---- Create G-Buffer pipeline ----
	m_GBufferShaderHandle = m_AssetManager.LoadSync("Assets/Shaders/GBuffer.hlsl");
	auto* pGBufShader = m_AssetManager.Get<ShaderAsset>(m_GBufferShaderHandle);
	if (!pGBufShader)
	{
		EVO_LOG_ERROR("TestScene: failed to load G-Buffer shader");
		return false;
	}

	RHIGraphicsPipelineDesc gbufDesc = {};
	gbufDesc.vertexShader         = pGBufShader->GetVertexShader();
	gbufDesc.pixelShader          = pGBufShader->GetPixelShader();
	gbufDesc.pInputElements       = inputElements;
	gbufDesc.uInputElementCount   = 3;
	gbufDesc.rasterizer.cullMode  = RHICullMode::Back;
	gbufDesc.rasterizer.bFrontCounterClockwise = true;
	gbufDesc.depthStencil.bDepthTestEnable  = true;
	gbufDesc.depthStencil.bDepthWriteEnable = true;
	gbufDesc.depthStencil.depthCompareOp    = RHICompareOp::Less;
	gbufDesc.depthStencilFormat             = RHIFormat::D32_FLOAT;
	gbufDesc.uRenderTargetCount   = 3;
	gbufDesc.renderTargetFormats[0] = RHIFormat::R8G8B8A8_UNORM;      // Albedo
	gbufDesc.renderTargetFormats[1] = RHIFormat::R16G16B16A16_FLOAT;  // Normal
	gbufDesc.renderTargetFormats[2] = RHIFormat::R8G8B8A8_UNORM;      // RoughMet
	gbufDesc.topology             = RHIPrimitiveTopology::TriangleList;
	gbufDesc.uPushConstantSize    = sizeof(float) * 24;  // MVP + material
	gbufDesc.sDebugName           = "GBufferPSO";
	m_GBufferPipeline = pDevice->CreateGraphicsPipeline(gbufDesc);

	if (!m_GBufferPipeline.IsValid())
	{
		EVO_LOG_ERROR("TestScene: failed to create G-Buffer pipeline");
		return false;
	}

	// ---- Create shadow depth pipeline ----
	m_ShadowShaderHandle = m_AssetManager.LoadSync("Assets/Shaders/ShadowDepth.hlsl");
	auto* pShadowShader = m_AssetManager.Get<ShaderAsset>(m_ShadowShaderHandle);
	if (!pShadowShader)
	{
		EVO_LOG_ERROR("TestScene: failed to load ShadowDepth shader");
		return false;
	}

	RHIInputElement shadowInputElements[] = {
		{ "POSITION", 0, RHIFormat::R32G32B32_FLOAT, 0, 0 },
	};

	RHIGraphicsPipelineDesc shadowDesc = {};
	shadowDesc.vertexShader         = pShadowShader->GetVertexShader();
	// No pixel shader for depth-only pass
	shadowDesc.pInputElements       = shadowInputElements;
	shadowDesc.uInputElementCount   = 1;
	shadowDesc.rasterizer.cullMode  = RHICullMode::Front;  // Reduce peter-panning
	shadowDesc.rasterizer.bFrontCounterClockwise = true;
	shadowDesc.rasterizer.depthBias             = 1000;
	shadowDesc.rasterizer.fSlopeScaledDepthBias = 1.5f;
	shadowDesc.depthStencil.bDepthTestEnable  = true;
	shadowDesc.depthStencil.bDepthWriteEnable = true;
	shadowDesc.depthStencil.depthCompareOp    = RHICompareOp::Less;
	shadowDesc.depthStencilFormat             = RHIFormat::D32_FLOAT;
	shadowDesc.uRenderTargetCount   = 0;
	shadowDesc.topology             = RHIPrimitiveTopology::TriangleList;
	shadowDesc.uPushConstantSize    = sizeof(float) * 16;  // 4x4 matrix
	shadowDesc.sDebugName           = "ShadowDepthPSO";
	m_ShadowPipeline = pDevice->CreateGraphicsPipeline(shadowDesc);

	if (!m_ShadowPipeline.IsValid())
	{
		EVO_LOG_ERROR("TestScene: failed to create shadow depth pipeline");
		return false;
	}

	// Compute light view-projection matrix
	Vec3 lightDir = Vec3(0.5f, 1.0f, -0.3f).Normalized();
	Vec3 lightPos = lightDir * 15.0f;
	Mat4 lightView = Mat4::LookAtLH(lightPos, Vec3::Zero, Vec3(0, 1, 0));
	Mat4 lightProj = Mat4::OrthographicLH(20.0f, 20.0f, 0.1f, 40.0f);
	m_LightViewProj = lightView * lightProj;

	// ---- Create deferred lighting pipeline ----
	m_LightingShaderHandle = m_AssetManager.LoadSync("Assets/Shaders/DeferredLighting.hlsl");
	auto* pLightShader = m_AssetManager.Get<ShaderAsset>(m_LightingShaderHandle);
	if (!pLightShader)
	{
		EVO_LOG_ERROR("TestScene: failed to load DeferredLighting shader");
		return false;
	}

	// Descriptor set layout: 5 SRVs (albedo, normal, roughMet, depth, shadowMap)
	RHIDescriptorBinding lightBindings[] = {
		{ 0, RHIDescriptorType::ShaderResource, 1, RHIShaderStage::Pixel },
		{ 1, RHIDescriptorType::ShaderResource, 1, RHIShaderStage::Pixel },
		{ 2, RHIDescriptorType::ShaderResource, 1, RHIShaderStage::Pixel },
		{ 3, RHIDescriptorType::ShaderResource, 1, RHIShaderStage::Pixel },
		{ 4, RHIDescriptorType::ShaderResource, 1, RHIShaderStage::Pixel },
	};
	RHIDescriptorSetLayoutDesc lightLayoutDesc;
	lightLayoutDesc.pBindings     = lightBindings;
	lightLayoutDesc.uBindingCount = 5;
	m_LightingSetLayout = pDevice->CreateDescriptorSetLayout(lightLayoutDesc);

	RHIGraphicsPipelineDesc lightDesc = {};
	lightDesc.vertexShader         = pLightShader->GetVertexShader();
	lightDesc.pixelShader          = pLightShader->GetPixelShader();
	lightDesc.pInputElements       = nullptr;
	lightDesc.uInputElementCount   = 0;
	lightDesc.rasterizer.cullMode  = RHICullMode::None;
	lightDesc.depthStencil.bDepthTestEnable  = false;
	lightDesc.depthStencil.bDepthWriteEnable = false;
	lightDesc.uRenderTargetCount   = 1;
	lightDesc.renderTargetFormats[0] = RHIFormat::R16G16B16A16_FLOAT;
	lightDesc.topology             = RHIPrimitiveTopology::TriangleList;
	lightDesc.uPushConstantSize    = sizeof(LightingPushConstants);
	lightDesc.descriptorSetLayouts[0]  = m_LightingSetLayout;
	lightDesc.uDescriptorSetLayoutCount = 1;
	lightDesc.sDebugName           = "DeferredLightingPSO";
	m_LightingPipeline = pDevice->CreateGraphicsPipeline(lightDesc);

	if (!m_LightingPipeline.IsValid())
	{
		EVO_LOG_ERROR("TestScene: failed to create deferred lighting pipeline");
		return false;
	}

	// ---- Create post-processing pipeline ----
	m_PostProcessShaderHandle = m_AssetManager.LoadSync("Assets/Shaders/PostProcess.hlsl");
	auto* pPostShader = m_AssetManager.Get<ShaderAsset>(m_PostProcessShaderHandle);
	if (!pPostShader)
	{
		EVO_LOG_ERROR("TestScene: failed to load PostProcess shader");
		return false;
	}

	RHIDescriptorBinding postBindings[] = {
		{ 0, RHIDescriptorType::ShaderResource, 1, RHIShaderStage::Pixel },
	};
	RHIDescriptorSetLayoutDesc postLayoutDesc;
	postLayoutDesc.pBindings     = postBindings;
	postLayoutDesc.uBindingCount = 1;
	m_PostProcessSetLayout = pDevice->CreateDescriptorSetLayout(postLayoutDesc);

	RHIGraphicsPipelineDesc postDesc = {};
	postDesc.vertexShader         = pPostShader->GetVertexShader();
	postDesc.pixelShader          = pPostShader->GetPixelShader();
	postDesc.pInputElements       = nullptr;
	postDesc.uInputElementCount   = 0;
	postDesc.rasterizer.cullMode  = RHICullMode::None;
	postDesc.depthStencil.bDepthTestEnable  = false;
	postDesc.depthStencil.bDepthWriteEnable = false;
	postDesc.uRenderTargetCount   = 1;
	postDesc.renderTargetFormats[0] = rtFormat;
	postDesc.topology             = RHIPrimitiveTopology::TriangleList;
	postDesc.uPushConstantSize    = 0;
	postDesc.descriptorSetLayouts[0]  = m_PostProcessSetLayout;
	postDesc.uDescriptorSetLayoutCount = 1;
	postDesc.sDebugName           = "PostProcessPSO";
	m_PostProcessPipeline = pDevice->CreateGraphicsPipeline(postDesc);

	if (!m_PostProcessPipeline.IsValid())
	{
		EVO_LOG_ERROR("TestScene: failed to create post-processing pipeline");
		return false;
	}

	// ---- Create forward transparent pipeline ----
	m_TransparentShaderHandle = m_AssetManager.LoadSync("Assets/Shaders/ForwardTransparent.hlsl");
	auto* pTransShader = m_AssetManager.Get<ShaderAsset>(m_TransparentShaderHandle);
	if (!pTransShader)
	{
		EVO_LOG_ERROR("TestScene: failed to load ForwardTransparent shader");
		return false;
	}

	// Descriptor set layout: 1 SRV (shadow map)
	RHIDescriptorBinding transBindings[] = {
		{ 0, RHIDescriptorType::ShaderResource, 1, RHIShaderStage::Pixel },
	};
	RHIDescriptorSetLayoutDesc transLayoutDesc;
	transLayoutDesc.pBindings     = transBindings;
	transLayoutDesc.uBindingCount = 1;
	m_TransparentShadowSetLayout = pDevice->CreateDescriptorSetLayout(transLayoutDesc);

	RHIGraphicsPipelineDesc transDesc = {};
	transDesc.vertexShader         = pTransShader->GetVertexShader();
	transDesc.pixelShader          = pTransShader->GetPixelShader();
	transDesc.pInputElements       = inputElements;
	transDesc.uInputElementCount   = 3;
	transDesc.rasterizer.cullMode  = RHICullMode::Back;
	transDesc.rasterizer.bFrontCounterClockwise = true;
	transDesc.depthStencil.bDepthTestEnable  = true;
	transDesc.depthStencil.bDepthWriteEnable = false;
	transDesc.depthStencil.depthCompareOp    = RHICompareOp::Less;
	transDesc.depthStencilFormat             = RHIFormat::D32_FLOAT;
	transDesc.uRenderTargetCount   = 1;
	transDesc.renderTargetFormats[0] = RHIFormat::R16G16B16A16_FLOAT;  // HDR target
	transDesc.blendTargets[0].bBlendEnable = true;
	transDesc.blendTargets[0].srcColor     = RHIBlendFactor::SrcAlpha;
	transDesc.blendTargets[0].dstColor     = RHIBlendFactor::InvSrcAlpha;
	transDesc.blendTargets[0].colorOp      = RHIBlendOp::Add;
	transDesc.blendTargets[0].srcAlpha     = RHIBlendFactor::One;
	transDesc.blendTargets[0].dstAlpha     = RHIBlendFactor::InvSrcAlpha;
	transDesc.blendTargets[0].alphaOp      = RHIBlendOp::Add;
	transDesc.topology             = RHIPrimitiveTopology::TriangleList;
	transDesc.uPushConstantSize    = sizeof(TransparentPushConstants);
	transDesc.descriptorSetLayouts[0]  = m_TransparentShadowSetLayout;
	transDesc.uDescriptorSetLayoutCount = 1;
	transDesc.sDebugName           = "ForwardTransparentPSO";
	m_TransparentPipeline = pDevice->CreateGraphicsPipeline(transDesc);

	if (!m_TransparentPipeline.IsValid())
	{
		EVO_LOG_ERROR("TestScene: failed to create forward transparent pipeline");
		return false;
	}

	// ---- Generate cube mesh data and write to .emesh file ----
	std::vector<StaticVertex> vertices;
	std::vector<uint32> indices;
	GenerateCubeData(vertices, indices);

	std::vector<Vec3> positions;
	std::vector<Vec3> normals;
	std::vector<Vec2> uvs;
	positions.reserve(vertices.size());
	normals.reserve(vertices.size());
	uvs.reserve(vertices.size());
	for (const auto& v : vertices)
	{
		positions.emplace_back(v.position[0], v.position[1], v.position[2]);
		normals.emplace_back(v.normal[0], v.normal[1], v.normal[2]);
		uvs.emplace_back(v.uv[0], v.uv[1]);
	}

	std::filesystem::create_directories("Assets/Meshes");
	if (!WriteMeshFromArrays(
		"Assets/Meshes/Cube.emesh",
		positions.data(), normals.data(), uvs.data(),
		static_cast<uint32>(vertices.size()),
		indices.data(), static_cast<uint32>(indices.size()),
		RHIIndexFormat::U32))
	{
		EVO_LOG_ERROR("TestScene: failed to write Cube.emesh");
		return false;
	}

	// ---- Write prefab file ----
	if (!WritePrefab("Assets/Prefabs/CubePrefab.eprefab", "Assets/Meshes/Cube.emesh"))
	{
		EVO_LOG_ERROR("TestScene: failed to write CubePrefab.eprefab");
		return false;
	}

	// ---- Write material files ----
	{
		struct MaterialSetup {
			const char* pName;
			Vec3 vAlbedo;
			float fRoughness;
			float fMetallic;
			float fAlpha;
		};

		MaterialSetup materials[] = {
			{ "CenterCube", Vec3(0.9f, 0.2f, 0.2f), 0.3f, 0.0f, 1.0f },
			{ "RightCube",  Vec3(0.2f, 0.9f, 0.2f), 0.5f, 0.0f, 1.0f },
			{ "LeftCube",   Vec3(0.2f, 0.2f, 0.9f), 0.5f, 0.0f, 1.0f },
			{ "TopCube",    Vec3(0.9f, 0.9f, 0.2f), 0.1f, 0.9f, 1.0f },
			{ "BottomCube", Vec3(0.9f, 0.5f, 0.2f), 0.8f, 0.0f, 1.0f },
		};

		std::filesystem::create_directories("Assets/Materials");
		for (const auto& m : materials)
		{
			std::string path = std::string("Assets/Materials/") + m.pName + ".ematerial";
			if (!WriteMaterial(path, m.vAlbedo, m.fRoughness, m.fMetallic, m.fAlpha))
			{
				EVO_LOG_ERROR("TestScene: failed to write '{}'", path);
				return false;
			}
		}
	}

	// ---- Build a temporary scene and write .escene file ----
	{
		Scene tempScene;

		struct CubeSetup {
			const char* pName;
			Vec3 vPosition;
			Vec3 vScale;
		};

		CubeSetup cubes[] = {
			{ "CenterCube", Vec3( 0.0f,  0.0f,  0.0f), Vec3(1.0f) },
			{ "RightCube",  Vec3( 3.0f,  0.0f,  0.0f), Vec3(0.7f) },
			{ "LeftCube",   Vec3(-3.0f,  0.0f,  0.0f), Vec3(0.7f) },
			{ "TopCube",    Vec3( 0.0f,  2.5f,  0.0f), Vec3(0.5f) },
			{ "BottomCube", Vec3( 0.0f, -2.5f,  0.0f), Vec3(0.5f) },
		};

		for (const auto& setup : cubes)
		{
			auto entity = tempScene.CreateEntity(setup.pName);

			TransformComponent transform;
			transform.vPosition = setup.vPosition;
			transform.vScale    = setup.vScale;
			tempScene.Transforms().Add(entity, transform);

			tempScene.SetEntityPrefab(entity, "Assets/Prefabs/CubePrefab.eprefab");
			tempScene.SetEntityMaterial(entity,
				std::string("Assets/Materials/") + setup.pName + ".ematerial");
		}

		std::filesystem::create_directories("Assets/Scenes");
		if (!WriteScene("Assets/Scenes/CubeScene.escene", tempScene))
		{
			EVO_LOG_ERROR("TestScene: failed to write CubeScene.escene");
			return false;
		}
	}

	// ---- Load scene from .escene file ----
	// Materials are now loaded automatically from .ematerial files via material_path
	if (!LoadScene("Assets/Scenes/CubeScene.escene", m_Scene, m_AssetManager))
	{
		EVO_LOG_ERROR("TestScene: failed to load CubeScene.escene");
		return false;
	}

	// ---- Add transparent cubes ----
	{
		// Write transparent material files
		WriteMaterial("Assets/Materials/GlassCube.ematerial",
		              Vec3(0.4f, 0.8f, 0.9f), 0.1f, 0.0f, 0.4f);
		WriteMaterial("Assets/Materials/TintedCube.ematerial",
		              Vec3(0.9f, 0.3f, 0.6f), 0.3f, 0.0f, 0.6f);

		auto prefabHandle = m_AssetManager.LoadSync("Assets/Prefabs/CubePrefab.eprefab");
		auto* pPrefab = m_AssetManager.Get<PrefabAsset>(prefabHandle);
		MeshAsset* pMeshAsset = nullptr;
		if (pPrefab && !pPrefab->GetMeshPath().empty())
		{
			auto meshHandle = m_AssetManager.LoadSync(pPrefab->GetMeshPath());
			pMeshAsset = m_AssetManager.Get<MeshAsset>(meshHandle);
		}

		// Load transparent materials from asset files
		auto glassMatHandle = m_AssetManager.LoadSync("Assets/Materials/GlassCube.ematerial");
		auto* pGlassMat = m_AssetManager.Get<MaterialAsset>(glassMatHandle);
		auto tintMatHandle = m_AssetManager.LoadSync("Assets/Materials/TintedCube.ematerial");
		auto* pTintMat = m_AssetManager.Get<MaterialAsset>(tintMatHandle);

		auto transCube1 = m_Scene.CreateEntity("GlassCube");
		TransformComponent t1;
		t1.vPosition = Vec3(1.5f, 1.5f, 1.5f);
		t1.vScale    = Vec3(1.2f);
		m_Scene.Transforms().Add(transCube1, t1);
		if (pMeshAsset)
		{
			MeshComponent mesh1;
			mesh1.pMesh = pMeshAsset;
			m_Scene.Meshes().Add(transCube1, mesh1);
		}
		m_Scene.SetEntityMaterial(transCube1, "Assets/Materials/GlassCube.ematerial");
		if (pGlassMat)
		{
			MaterialComponent glassMat;
			glassMat.vAlbedoColor = pGlassMat->GetAlbedo();
			glassMat.fRoughness   = pGlassMat->GetRoughness();
			glassMat.fMetallic    = pGlassMat->GetMetallic();
			glassMat.fAlpha       = pGlassMat->GetAlpha();
			m_Scene.Materials().Add(transCube1, glassMat);
		}

		auto transCube2 = m_Scene.CreateEntity("TintedCube");
		TransformComponent t2;
		t2.vPosition = Vec3(-1.5f, 1.5f, -1.5f);
		t2.vScale    = Vec3(0.9f);
		m_Scene.Transforms().Add(transCube2, t2);
		if (pMeshAsset)
		{
			MeshComponent mesh2;
			mesh2.pMesh = pMeshAsset;
			m_Scene.Meshes().Add(transCube2, mesh2);
		}
		m_Scene.SetEntityMaterial(transCube2, "Assets/Materials/TintedCube.ematerial");
		if (pTintMat)
		{
			MaterialComponent tintMat;
			tintMat.vAlbedoColor = pTintMat->GetAlbedo();
			tintMat.fRoughness   = pTintMat->GetRoughness();
			tintMat.fMetallic    = pTintMat->GetMetallic();
			tintMat.fAlpha       = pTintMat->GetAlpha();
			m_Scene.Materials().Add(transCube2, tintMat);
		}
	}

	// Initialize game camera (fixed position)
	m_GameCamera.SetPerspective(DegToRad(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
	m_GameCamera.SetPosition(Vec3(0.0f, 3.0f, -8.0f));
	m_GameCamera.LookAt(Vec3::Zero);

	EVO_LOG_INFO("TestScene initialized: {} entities (loaded from .escene)", m_Scene.GetEntityCount());
	return true;
#else
	return false;
#endif
}

void TestScene::Shutdown(RHIDevice* pDevice)
{
	if (m_TransparentShadowSetLayout.IsValid()) pDevice->DestroyDescriptorSetLayout(m_TransparentShadowSetLayout);
	if (m_TransparentPipeline.IsValid()) pDevice->DestroyPipeline(m_TransparentPipeline);
	if (m_PostProcessSetLayout.IsValid()) pDevice->DestroyDescriptorSetLayout(m_PostProcessSetLayout);
	if (m_PostProcessPipeline.IsValid()) pDevice->DestroyPipeline(m_PostProcessPipeline);
	if (m_LightingSetLayout.IsValid()) pDevice->DestroyDescriptorSetLayout(m_LightingSetLayout);
	if (m_LightingPipeline.IsValid()) pDevice->DestroyPipeline(m_LightingPipeline);
	if (m_ShadowPipeline.IsValid()) pDevice->DestroyPipeline(m_ShadowPipeline);
	if (m_GBufferPipeline.IsValid()) pDevice->DestroyPipeline(m_GBufferPipeline);
	if (m_Pipeline.IsValid()) pDevice->DestroyPipeline(m_Pipeline);
	m_AssetManager.Shutdown();
}

void TestScene::Update(float fDeltaTime)
{
	m_fTime += fDeltaTime;
	m_AssetManager.Update();

	// Rotate each cube differently
	int index = 0;
	m_Scene.Transforms().ForEach([&](EntityHandle /*entity*/, TransformComponent& transform) {
		float speed = 0.5f + index * 0.3f;
		transform.qRotation = Quat::FromEuler(
			m_fTime * speed * 0.7f,
			m_fTime * speed,
			m_fTime * speed * 0.3f);
		index++;
	});
}

void TestScene::Render(Renderer& renderer,
                       RGHandle targetTexture, RHIRenderTargetView targetRTV,
                       RGHandle depthTexture, RHIDepthStencilView depthDSV,
                       const Mat4& viewProj,
                       float fViewportWidth, float fViewportHeight)
{
	m_SceneRenderer.RenderScene(m_Scene, renderer, m_Pipeline, viewProj,
	                            targetTexture, targetRTV,
	                            depthTexture, depthDSV,
	                            fViewportWidth, fViewportHeight);
}

void TestScene::RenderShadowMap(Renderer& renderer,
                               RGHandle shadowTexture, RHIDepthStencilView shadowDSV,
                               float fShadowMapSize)
{
	m_SceneRenderer.RenderShadowMap(m_Scene, renderer, m_ShadowPipeline,
	                                m_LightViewProj, shadowTexture, shadowDSV,
	                                fShadowMapSize);
}

void TestScene::RenderGBuffer(Renderer& renderer,
                              const GBufferTargets& targets,
                              const Mat4& viewProj,
                              float fViewportWidth, float fViewportHeight)
{
	m_SceneRenderer.RenderGBuffer(m_Scene, renderer, m_GBufferPipeline, viewProj,
	                              targets, fViewportWidth, fViewportHeight);
}

void TestScene::RenderLighting(Renderer& renderer,
                               const GBufferTargets& gbTargets,
                               RHIDescriptorSetHandle lightingDescSet,
                               RGHandle shadowTexture,
                               RGHandle targetTexture, RHIRenderTargetView targetRTV,
                               const Mat4& viewProj,
                               float fViewportWidth, float fViewportHeight)
{
	LightingPushConstants pc = {};
	pc.invViewProj = viewProj.Inverse();
	pc.lightViewProj = m_LightViewProj;
	Vec3 lightDir = Vec3(0.5f, 1.0f, -0.3f).Normalized();
	pc.vLightDir[0] = lightDir.x;
	pc.vLightDir[1] = lightDir.y;
	pc.vLightDir[2] = lightDir.z;
	pc.fShadowMapSize = 2048.0f;
	pc.vLightColor[0] = 1.0f;
	pc.vLightColor[1] = 0.98f;
	pc.vLightColor[2] = 0.92f;

	m_SceneRenderer.AddLightingPass(renderer, m_LightingPipeline, lightingDescSet,
	                                gbTargets, shadowTexture,
	                                targetTexture, targetRTV, pc,
	                                fViewportWidth, fViewportHeight);
}

void TestScene::RenderPostProcess(Renderer& renderer,
                                  RHIDescriptorSetHandle postDescSet,
                                  RGHandle hdrTexture,
                                  RGHandle targetTexture, RHIRenderTargetView targetRTV,
                                  float fViewportWidth, float fViewportHeight)
{
	m_SceneRenderer.AddPostProcessPass(renderer, m_PostProcessPipeline, postDescSet,
	                                   hdrTexture, targetTexture, targetRTV,
	                                   fViewportWidth, fViewportHeight);
}

void TestScene::RenderForwardTransparent(Renderer& renderer,
                                         RHIDescriptorSetHandle shadowDescSet,
                                         const Mat4& viewProj,
                                         RGHandle targetTexture, RHIRenderTargetView targetRTV,
                                         RGHandle depthTexture, RHIDepthStencilView depthDSV,
                                         RGHandle shadowTexture,
                                         float fViewportWidth, float fViewportHeight)
{
	TransparentPushConstants basePc = {};
	basePc.lightViewProj = m_LightViewProj;
	Vec3 lightDir = Vec3(0.5f, 1.0f, -0.3f).Normalized();
	basePc.vLightDir[0] = lightDir.x;
	basePc.vLightDir[1] = lightDir.y;
	basePc.vLightDir[2] = lightDir.z;
	basePc.fShadowMapSize = 2048.0f;
	basePc.vLightColor[0] = 1.0f;
	basePc.vLightColor[1] = 0.98f;
	basePc.vLightColor[2] = 0.92f;

	m_SceneRenderer.RenderForwardTransparent(m_Scene, renderer, m_TransparentPipeline,
	                                         shadowDescSet, viewProj, basePc,
	                                         targetTexture, targetRTV,
	                                         depthTexture, depthDSV,
	                                         shadowTexture,
	                                         fViewportWidth, fViewportHeight);
}

} // namespace Evo
