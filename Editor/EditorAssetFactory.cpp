#include "EditorAssetFactory.h"
#include "Scene/MeshWriter.h"
#include "Scene/SceneWriter.h"
#include "Scene/MaterialWriter.h"
#include "Scene/PrefabWriter.h"
#include "Scene/Scene.h"
#include "Scene/Components.h"
#include "Math/Math.h"
#include "Core/Log.h"
#include "Core/Types.h"
#include "Math/Vec2.h"

#include <vector>
#include <filesystem>

namespace Evo {

// ---- Cube mesh vertex data ----

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

// ---- Factory methods ----

bool EditorAssetFactory::CreateCubeMesh(const std::string& sPath)
{
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

	std::filesystem::create_directories(std::filesystem::path(sPath).parent_path());
	return WriteMeshFromArrays(sPath,
		positions.data(), normals.data(), uvs.data(),
		static_cast<uint32>(vertices.size()),
		indices.data(), static_cast<uint32>(indices.size()),
		RHIIndexFormat::U32);
}

bool EditorAssetFactory::CreatePrefab(const std::string& sPath, const std::string& sMeshPath)
{
	std::filesystem::create_directories(std::filesystem::path(sPath).parent_path());
	return WritePrefab(sPath, sMeshPath);
}

bool EditorAssetFactory::CreateMaterial(const std::string& sPath,
										const Vec3& vAlbedo, float fRoughness,
										float fMetallic, float fAlpha)
{
	std::filesystem::create_directories(std::filesystem::path(sPath).parent_path());
	return WriteMaterial(sPath, vAlbedo, fRoughness, fMetallic, fAlpha);
}

bool EditorAssetFactory::CreateCubeScene(const std::string& sPath,
										 const Vec3* pPositions, const Vec3* pScales,
										 const std::string* pMaterialNames,
										 int nCount,
										 const std::string& sPrefabPath,
										 const std::string& sMaterialDir)
{
	Scene tempScene;
	for (int i = 0; i < nCount; ++i)
	{
		auto entity = tempScene.CreateEntity(pMaterialNames[i].c_str());
		TransformComponent transform;
		transform.vPosition = pPositions[i];
		transform.vScale    = pScales[i];
		tempScene.Transforms().Add(entity, transform);
		tempScene.SetEntityPrefab(entity, sPrefabPath);
		tempScene.SetEntityMaterial(entity, sMaterialDir + pMaterialNames[i] + ".ematerial");
	}

	std::filesystem::create_directories(std::filesystem::path(sPath).parent_path());
	return WriteScene(sPath, &tempScene);
}

void EditorAssetFactory::EnsureDemoAssets()
{
	std::filesystem::create_directories("Assets/Meshes");
	std::filesystem::create_directories("Assets/Prefabs");
	std::filesystem::create_directories("Assets/Materials");
	std::filesystem::create_directories("Assets/Scenes");

	// Mesh + Prefab
	CreateCubeMesh("Assets/Meshes/Cube.emesh");
	CreatePrefab("Assets/Prefabs/CubePrefab.eprefab", "Assets/Meshes/Cube.emesh");

	// Opaque materials
	CreateMaterial("Assets/Materials/CenterCube.ematerial", Vec3(0.9f, 0.2f, 0.2f), 0.3f, 0.0f);
	CreateMaterial("Assets/Materials/RightCube.ematerial",  Vec3(0.2f, 0.9f, 0.2f), 0.5f, 0.0f);
	CreateMaterial("Assets/Materials/LeftCube.ematerial",   Vec3(0.2f, 0.2f, 0.9f), 0.5f, 0.0f);
	CreateMaterial("Assets/Materials/TopCube.ematerial",    Vec3(0.9f, 0.9f, 0.2f), 0.1f, 0.9f);
	CreateMaterial("Assets/Materials/BottomCube.ematerial", Vec3(0.9f, 0.5f, 0.2f), 0.8f, 0.0f);

	// Transparent materials
	CreateMaterial("Assets/Materials/GlassCube.ematerial",  Vec3(0.4f, 0.8f, 0.9f), 0.1f, 0.0f, 0.4f);
	CreateMaterial("Assets/Materials/TintedCube.ematerial", Vec3(0.9f, 0.3f, 0.6f), 0.3f, 0.0f, 0.6f);

	// Scene — 5 opaque + 2 transparent cubes + camera
	{
		Scene tempScene;
		Vec3 positions[] = {
			Vec3( 0.0f,  0.0f,  0.0f), Vec3( 3.0f,  0.0f,  0.0f),
			Vec3(-3.0f,  0.0f,  0.0f), Vec3( 0.0f,  2.5f,  0.0f),
			Vec3( 0.0f, -2.5f,  0.0f), Vec3( 1.5f,  1.5f,  1.5f),
			Vec3(-1.5f,  1.5f, -1.5f),
		};
		Vec3 scales[] = {
			Vec3(1.0f), Vec3(0.7f), Vec3(0.7f), Vec3(0.5f), Vec3(0.5f),
			Vec3(1.2f), Vec3(0.9f),
		};
		const char* mats[] = {
			"CenterCube", "RightCube", "LeftCube", "TopCube", "BottomCube",
			"GlassCube", "TintedCube",
		};

		for (int i = 0; i < 7; ++i)
		{
			auto entity = tempScene.CreateEntity(mats[i]);
			TransformComponent t;
			t.vPosition = positions[i];
			t.vScale    = scales[i];
			tempScene.Transforms().Add(entity, t);
			tempScene.SetEntityPrefab(entity, "Assets/Prefabs/CubePrefab.eprefab");
			tempScene.SetEntityMaterial(entity, std::string("Assets/Materials/") + mats[i] + ".ematerial");
		}

		// Game camera
		{
			auto cam = tempScene.CreateEntity("GameCamera");
			TransformComponent camT;
			camT.vPosition = Vec3(0.0f, 3.0f, -8.0f);
			Vec3 dir = (Vec3::Zero - camT.vPosition).Normalized();
			camT.qRotation = Quat::FromEuler(std::asin(Clamp(dir.y, -1.0f, 1.0f)),
											 std::atan2(dir.x, dir.z), 0.0f);
			tempScene.Transforms().Add(cam, camT);

			CameraComponent camComp;
			camComp.fFovY  = DegToRad(60.0f);
			camComp.fNearZ = 0.1f;
			camComp.fFarZ  = 100.0f;
			tempScene.Cameras().Add(cam, camComp);
		}

		WriteScene("Assets/Scenes/CubeScene.escene", &tempScene);
	}
}

} // namespace Evo
