#include "CubeDemo.h"
#include "Asset/ShaderAsset.h"
#include "Scene/MeshWriter.h"
#include "Renderer/Renderer.h"
#include "Platform/Input.h"
#include "Platform/Window.h"
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
	// 24 vertices (4 per face, each with unique normal)
	// 36 indices (6 per face)

	struct FaceData {
		float nx, ny, nz;        // Normal
		float verts[4][3];       // 4 corner positions
		float uvs[4][2];        // 4 corner UVs
	};

	const float s = 0.5f;

	FaceData faces[] = {
		// +Z front
		{ 0, 0, 1, {{ -s,-s, s }, {  s,-s, s }, {  s, s, s }, { -s, s, s }}, {{ 0,1 }, { 1,1 }, { 1,0 }, { 0,0 }} },
		// -Z back
		{ 0, 0,-1, {{  s,-s,-s }, { -s,-s,-s }, { -s, s,-s }, {  s, s,-s }}, {{ 0,1 }, { 1,1 }, { 1,0 }, { 0,0 }} },
		// +X right
		{ 1, 0, 0, {{  s,-s, s }, {  s,-s,-s }, {  s, s,-s }, {  s, s, s }}, {{ 0,1 }, { 1,1 }, { 1,0 }, { 0,0 }} },
		// -X left
		{-1, 0, 0, {{ -s,-s,-s }, { -s,-s, s }, { -s, s, s }, { -s, s,-s }}, {{ 0,1 }, { 1,1 }, { 1,0 }, { 0,0 }} },
		// +Y top
		{ 0, 1, 0, {{ -s, s, s }, {  s, s, s }, {  s, s,-s }, { -s, s,-s }}, {{ 0,1 }, { 1,1 }, { 1,0 }, { 0,0 }} },
		// -Y bottom
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
		// Two triangles per face
		outIndices.push_back(baseVertex + 0);
		outIndices.push_back(baseVertex + 1);
		outIndices.push_back(baseVertex + 2);
		outIndices.push_back(baseVertex + 0);
		outIndices.push_back(baseVertex + 2);
		outIndices.push_back(baseVertex + 3);
	}
}

bool CubeDemo::Initialize(RHIDevice* pDevice, RHIFormat rtFormat)
{
#if EVO_RHI_DX12
	// ---- Initialize asset manager ----
	m_AssetManager.Initialize(pDevice);
	m_AssetManager.RegisterFactory(".hlsl",  [] { return std::make_unique<ShaderAsset>(); });
	m_AssetManager.RegisterFactory(".emesh", [] { return std::make_unique<MeshAsset>(); });

	// ---- Load shader via asset system ----
	m_ShaderHandle = m_AssetManager.LoadSync("Assets/Shaders/StaticMesh.hlsl");
	auto* pShader = m_AssetManager.Get<ShaderAsset>(m_ShaderHandle);
	if (!pShader)
	{
		EVO_LOG_ERROR("CubeDemo: failed to load shader");
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
	pipelineDesc.depthStencil.bDepthTestEnable = false;
	pipelineDesc.uRenderTargetCount   = 1;
	pipelineDesc.renderTargetFormats[0] = rtFormat;
	pipelineDesc.topology             = RHIPrimitiveTopology::TriangleList;
	pipelineDesc.uPushConstantSize    = sizeof(float) * 16;  // 4x4 matrix
	pipelineDesc.sDebugName           = "StaticMeshPSO";
	m_Pipeline = pDevice->CreateGraphicsPipeline(pipelineDesc);

	if (!m_Pipeline.IsValid())
		return false;

	// ---- Generate cube mesh data and write to .emesh file ----
	std::vector<StaticVertex> vertices;
	std::vector<uint32> indices;
	GenerateCubeData(vertices, indices);

	// Extract SoA arrays for MeshWriter
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

	// Write .emesh file (FlatBuffers format)
	std::filesystem::create_directories("Assets/Meshes");
	if (!WriteMeshFromArrays(
		"Assets/Meshes/Cube.emesh",
		positions.data(), normals.data(), uvs.data(),
		static_cast<uint32>(vertices.size()),
		indices.data(), static_cast<uint32>(indices.size()),
		RHIIndexFormat::U32))
	{
		EVO_LOG_ERROR("CubeDemo: failed to write Cube.emesh");
		return false;
	}

	// ---- Load cube mesh from .emesh file via asset system ----
	m_MeshHandle = m_AssetManager.LoadSync("Assets/Meshes/Cube.emesh");
	auto* pCubeMesh = m_AssetManager.Get<MeshAsset>(m_MeshHandle);
	if (!pCubeMesh)
	{
		EVO_LOG_ERROR("CubeDemo: failed to load Cube.emesh");
		return false;
	}

	// Create scene with multiple cubes at different positions
	struct CubeSetup {
		Vec3 vPosition;
		Vec3 vScale;
	};

	CubeSetup cubes[] = {
		{ Vec3( 0.0f,  0.0f,  0.0f), Vec3(1.0f) },
		{ Vec3( 3.0f,  0.0f,  0.0f), Vec3(0.7f) },
		{ Vec3(-3.0f,  0.0f,  0.0f), Vec3(0.7f) },
		{ Vec3( 0.0f,  2.5f,  0.0f), Vec3(0.5f) },
		{ Vec3( 0.0f, -2.5f,  0.0f), Vec3(0.5f) },
	};

	for (const auto& setup : cubes)
	{
		auto entity = m_Scene.CreateEntity("Cube");

		TransformComponent transform;
		transform.vPosition = setup.vPosition;
		transform.vScale    = setup.vScale;
		m_Scene.Transforms().Add(entity, transform);

		MeshComponent mesh;
		mesh.pMesh = pCubeMesh;
		m_Scene.Meshes().Add(entity, mesh);
	}

	// Initialize camera
	m_Camera.SetPerspective(DegToRad(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
	m_Camera.SetPosition(Vec3(0.0f, 3.0f, -8.0f));
	m_Camera.LookAt(Vec3::Zero);

	EVO_LOG_INFO("CubeDemo initialized: {} entities (mesh loaded from .emesh)", 5);
	return true;
#else
	return false;
#endif
}

void CubeDemo::Shutdown(RHIDevice* pDevice)
{
	if (m_Pipeline.IsValid()) pDevice->DestroyPipeline(m_Pipeline);

	// AssetManager shuts down all loaded assets (shaders, meshes, etc.)
	m_AssetManager.Shutdown();
}

void CubeDemo::Update(float fDeltaTime, const Input& input, Window& window)
{
	m_fTime += fDeltaTime;

	// Process async asset completions (for future async loads)
	m_AssetManager.Update();

	// Update camera
	float w = static_cast<float>(window.GetWidth());
	float h = static_cast<float>(window.GetHeight());
	if (h > 0.0f)
		m_Camera.SetAspect(w / h);
	m_CameraController.Update(m_Camera, input, window, fDeltaTime);

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

void CubeDemo::Render(Renderer& renderer)
{
	Mat4 viewProj = m_Camera.GetViewProjectionMatrix();
	m_SceneRenderer.RenderScene(m_Scene, renderer, m_Pipeline, viewProj);
}

} // namespace Evo
