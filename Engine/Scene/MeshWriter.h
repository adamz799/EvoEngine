#pragma once

#include "Scene/MeshAsset.h"
#include "Math/Vec2.h"
#include "Math/Vec3.h"
#include "Math/Bounds.h"
#include "Mesh_generated.h"
#include <string>

namespace Evo {

/// Description of a single vertex attribute for mesh writing.
struct MeshAttributeDesc {
	Schema::AttributeFormat format = Schema::AttributeFormat_Float32x3;
	const uint8*            pData  = nullptr;
	uint32                  uDataSize = 0;
};

/// Description of a mesh to write as FlatBuffers .emesh file.
struct MeshWriteDesc {
	// Required vertex attributes
	MeshAttributeDesc positions;
	MeshAttributeDesc normals;
	MeshAttributeDesc uvs;

	// Optional vertex attributes
	MeshAttributeDesc tangents;
	MeshAttributeDesc uvs2;
	MeshAttributeDesc colors;
	MeshAttributeDesc boneIndices;
	MeshAttributeDesc boneWeights;

	// Index buffer
	Schema::IndexFormat indexFormat = Schema::IndexFormat_UInt32;
	const uint8*        pIndexData     = nullptr;
	uint32              uIndexDataSize = 0;

	// Metadata (optional)
	const AABB*    pBounds       = nullptr;
	const SubMesh* pSubMeshes    = nullptr;
	uint32         uSubMeshCount = 0;
};

/// Build a FlatBuffer from the given mesh data and write to disk.
bool WriteMesh(const std::string& sPath, const MeshWriteDesc& desc);

/// Convenience: write a basic mesh from Vec3/Vec2 arrays (Float32x3/Float32x2).
bool WriteMeshFromArrays(
	const std::string& sPath,
	const Vec3* pPositions, const Vec3* pNormals, const Vec2* pUVs, uint32 uVertexCount,
	const void* pIndices, uint32 uIndexCount, RHIIndexFormat indexFormat,
	const AABB* pBounds = nullptr);

} // namespace Evo

