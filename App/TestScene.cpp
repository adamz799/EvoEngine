#include "TestScene.h"
#include "Renderer/Renderer.h"
#include "Asset/ShaderAsset.h"
#include "Asset/TextureAsset.h"
#include "Scene/MeshWriter.h"
#include "Scene/MaterialAsset.h"
#include "Scene/MaterialWriter.h"
#include "Scene/PrefabAsset.h"
#include "Scene/PrefabWriter.h"
#include "Scene/SceneWriter.h"
#include "Scene/SceneLoader.h"
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

bool TestScene::Initialize(Render* pRender)
{
	auto* pDevice = pRender->GetDevice();
#if EVO_RHI_DX12
	// ---- Initialize asset manager (scene assets only) ----
	m_AssetManager.Initialize(pDevice);
	m_AssetManager.RegisterFactory(".emesh",     [] { return std::make_unique<MeshAsset>(); });
	m_AssetManager.RegisterFactory(".eprefab",   [] { return std::make_unique<PrefabAsset>(); });
	m_AssetManager.RegisterFactory(".ematerial", [] { return std::make_unique<MaterialAsset>(); });
	m_AssetManager.RegisterFactory(".png",       [] { return std::make_unique<TextureAsset>(); });
	m_AssetManager.RegisterFactory(".jpg",       [] { return std::make_unique<TextureAsset>(); });
	m_AssetManager.RegisterFactory(".tga",       [] { return std::make_unique<TextureAsset>(); });

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
	if (!LoadScene("Assets/Scenes/CubeScene.escene", m_Scene, m_AssetManager))
	{
		EVO_LOG_ERROR("TestScene: failed to load CubeScene.escene");
		return false;
	}

	// ---- Add transparent cubes ----
	{
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

	// Create game camera entity
	{
		m_GameCameraEntity = m_Scene.CreateEntity("GameCamera");

		TransformComponent camTransform;
		camTransform.vPosition = Vec3(0.0f, 3.0f, -8.0f);

		// Compute orientation: LookAt target (0,0,0) from position
		Vec3 dir = (Vec3::Zero - camTransform.vPosition).Normalized();
		float yaw   = std::atan2(dir.x, dir.z);
		float pitch = std::asin(Clamp(dir.y, -1.0f, 1.0f));
		camTransform.qRotation = Quat::FromEuler(pitch, yaw, 0.0f);

		m_Scene.Transforms().Add(m_GameCameraEntity, camTransform);

		CameraComponent camComp;
		camComp.fFovY  = DegToRad(60.0f);
		camComp.fNearZ = 0.1f;
		camComp.fFarZ  = 100.0f;
		m_Scene.Cameras().Add(m_GameCameraEntity, camComp);
	}

	EVO_LOG_INFO("TestScene initialized: {} entities (loaded from .escene)", m_Scene.GetEntityCount());
	return true;
#else
	return false;
#endif
}

void TestScene::Shutdown(Render* /*pRender*/)
{
	m_AssetManager.Shutdown();
}

void TestScene::Update(float fDeltaTime)
{
	m_fTime += fDeltaTime;
	m_AssetManager.Update();

	// Rotate each cube differently (skip camera entities)
	int index = 0;
	m_Scene.Transforms().ForEach([&](EntityHandle entity, TransformComponent& transform) {
		if (m_Scene.Cameras().Has(entity)) return;
		float speed = 0.5f + index * 0.3f;
		transform.qRotation = Quat::FromEuler(
			m_fTime * speed * 0.7f,
			m_fTime * speed,
			m_fTime * speed * 0.3f);
		index++;
	});
}

} // namespace Evo

