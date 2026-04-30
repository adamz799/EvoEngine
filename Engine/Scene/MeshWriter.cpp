#include "Scene/MeshWriter.h"
#include "Platform/FileSystem.h"
#include "Core/Log.h"
#include <cstring>

namespace Evo {

// ============================================================================
// Layout safety checks
// ============================================================================

static_assert(sizeof(Vec3) == 12, "Vec3 must be 12 bytes for reinterpret to Schema::Vec3");
static_assert(sizeof(Vec2) == 8,  "Vec2 must be 8 bytes for reinterpret to Schema::Vec2");

// ============================================================================
// Core writer — serialises MeshWriteDesc into FlatBuffers .emesh
// ============================================================================

static flatbuffers::Offset<Schema::VertexAttribute> BuildAttribute(
	flatbuffers::FlatBufferBuilder& builder,
	const MeshAttributeDesc& desc)
{
	auto dataVec = builder.CreateVector(desc.pData, desc.uDataSize);
	return Schema::CreateVertexAttribute(builder, desc.format, dataVec);
}

bool WriteMesh(const std::string& sPath, const MeshWriteDesc& desc)
{
	flatbuffers::FlatBufferBuilder builder(1024);

	// ---- Required attributes ----
	auto posOffset = BuildAttribute(builder, desc.positions);
	auto nrmOffset = BuildAttribute(builder, desc.normals);
	auto uvOffset  = BuildAttribute(builder, desc.uvs);

	// ---- Optional attributes ----
	flatbuffers::Offset<Schema::VertexAttribute> tanOffset, uv2Offset, colOffset;
	flatbuffers::Offset<Schema::VertexAttribute> boneIdxOffset, boneWgtOffset;

	if (desc.tangents.pData)    tanOffset    = BuildAttribute(builder, desc.tangents);
	if (desc.uvs2.pData)        uv2Offset    = BuildAttribute(builder, desc.uvs2);
	if (desc.colors.pData)      colOffset    = BuildAttribute(builder, desc.colors);
	if (desc.boneIndices.pData) boneIdxOffset = BuildAttribute(builder, desc.boneIndices);
	if (desc.boneWeights.pData) boneWgtOffset = BuildAttribute(builder, desc.boneWeights);

	// ---- Index buffer ----
	auto idxDataVec = builder.CreateVector(desc.pIndexData, desc.uIndexDataSize);
	auto ibOffset = Schema::CreateIndexBuffer(builder, desc.indexFormat, idxDataVec);

	// ---- Bounds ----
	Schema::AABB schemaBounds;
	const Schema::AABB* pSchemaBounds = nullptr;
	if (desc.pBounds)
	{
		schemaBounds = Schema::AABB(
			Schema::Vec3(desc.pBounds->vMin.x, desc.pBounds->vMin.y, desc.pBounds->vMin.z),
			Schema::Vec3(desc.pBounds->vMax.x, desc.pBounds->vMax.y, desc.pBounds->vMax.z));
		pSchemaBounds = &schemaBounds;
	}

	// ---- Sub-meshes ----
	flatbuffers::Offset<flatbuffers::Vector<const Schema::SubMeshRange*>> smOffset;
	if (desc.pSubMeshes && desc.uSubMeshCount > 0)
	{
		std::vector<Schema::SubMeshRange> ranges;
		ranges.reserve(desc.uSubMeshCount);
		for (uint32 i = 0; i < desc.uSubMeshCount; ++i)
		{
			ranges.emplace_back(
				desc.pSubMeshes[i].uIndexOffset,
				desc.pSubMeshes[i].uIndexCount,
				desc.pSubMeshes[i].uVertexOffset);
		}
		smOffset = builder.CreateVectorOfStructs(ranges);
	}

	// ---- Build root ----
	auto root = Schema::CreateMesh(
		builder,
		posOffset, nrmOffset, uvOffset,
		tanOffset, uv2Offset, colOffset,
		boneIdxOffset, boneWgtOffset,
		ibOffset,
		pSchemaBounds,
		smOffset);

	Schema::FinishMeshBuffer(builder, root);

	// ---- Write to disk ----
	auto data = std::span<const uint8>(builder.GetBufferPointer(), builder.GetSize());
	if (!FileSystem::WriteBinary(sPath, data))
	{
		EVO_LOG_ERROR("WriteMesh: failed to write '{}'", sPath);
		return false;
	}

	EVO_LOG_INFO("WriteMesh: wrote '{}' ({} bytes)", sPath, builder.GetSize());
	return true;
}

// ============================================================================
// Convenience: write from typed arrays (always Float32)
// ============================================================================

bool WriteMeshFromArrays(
	const std::string& sPath,
	const Vec3* pPositions, const Vec3* pNormals, const Vec2* pUVs, uint32 uVertexCount,
	const void* pIndices, uint32 uIndexCount, RHIIndexFormat indexFormat,
	const AABB* pBounds)
{
	MeshWriteDesc desc = {};

	desc.positions.format    = Schema::AttributeFormat_Float32x3;
	desc.positions.pData     = reinterpret_cast<const uint8*>(pPositions);
	desc.positions.uDataSize = uVertexCount * sizeof(Vec3);

	desc.normals.format    = Schema::AttributeFormat_Float32x3;
	desc.normals.pData     = reinterpret_cast<const uint8*>(pNormals);
	desc.normals.uDataSize = uVertexCount * sizeof(Vec3);

	desc.uvs.format    = Schema::AttributeFormat_Float32x2;
	desc.uvs.pData     = reinterpret_cast<const uint8*>(pUVs);
	desc.uvs.uDataSize = uVertexCount * sizeof(Vec2);

	uint32 uIndexStride = (indexFormat == RHIIndexFormat::U16) ? 2 : 4;
	desc.indexFormat    = (indexFormat == RHIIndexFormat::U16)
						? Schema::IndexFormat_UInt16 : Schema::IndexFormat_UInt32;
	desc.pIndexData     = reinterpret_cast<const uint8*>(pIndices);
	desc.uIndexDataSize = uIndexCount * uIndexStride;

	// Compute bounds if not provided
	AABB computed;
	if (pBounds)
	{
		desc.pBounds = pBounds;
	}
	else
	{
		computed = AABB::FromPoints(pPositions, uVertexCount);
		desc.pBounds = &computed;
	}

	return WriteMesh(sPath, desc);
}

} // namespace Evo

