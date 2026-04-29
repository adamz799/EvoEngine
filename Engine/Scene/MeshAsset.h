#pragma once

#include "Asset/Asset.h"
#include "RHI/RHITypes.h"
#include "Math/Bounds.h"
#include <vector>
#include <string>
#include <memory>

namespace Evo {

class RHIDevice;

/// A sub-range of indices within a MeshLOD (one draw call per SubMesh).
struct SubMesh {
	uint32 uIndexOffset  = 0;
	uint32 uIndexCount   = 0;
	uint32 uVertexOffset = 0;
};

/// One level-of-detail: vertex + index buffers and sub-mesh ranges.
struct MeshLOD {
	RHIBufferHandle          vertexBuffer;
	RHIBufferHandle          indexBuffer;
	std::vector<SubMesh>     vSubMeshes;
	uint32                   uVertexCount  = 0;
	uint32                   uIndexCount   = 0;
	uint32                   uVertexStride = 0;
};

/// Binary mesh format header (.emesh).
struct EvoMeshHeader {
	char   magic[4];       // "EMSH"
	uint32 uVersion;       // 1
	uint32 uVertexCount;
	uint32 uIndexCount;
	uint32 uVertexStride;
	uint32 uIndexFormat;   // 0 = U16, 1 = U32
	float  boundsMin[3];
	float  boundsMax[3];
};

/// MeshAsset — GPU-resident mesh data with bounds and LOD support.
///
/// Supports two creation paths:
///   1. File-based (via AssetManager): OnLoad parses .emesh binary → OnFinalize creates GPU buffers
///   2. Procedural (CreateFromMemory): direct GPU upload, bypasses Asset lifecycle
class MeshAsset : public Asset {
public:
	const char* GetTypeName() const override { return "Mesh"; }
	bool OnLoad(const std::vector<uint8>& rawData) override;
	bool OnFinalize(RHIDevice* pDevice) override;
	void OnUnload(RHIDevice* pDevice) override;

	/// Create a single-LOD mesh from raw vertex + index data. Uploads to GPU.
	/// Bypasses Asset lifecycle — not managed by AssetManager.
	static std::unique_ptr<MeshAsset> CreateFromMemory(
		RHIDevice* pDevice,
		const void* pVertices, uint32 uVertexCount, uint32 uVertexStride,
		const void* pIndices,  uint32 uIndexCount,  RHIIndexFormat indexFormat,
		const Vec3* pPositions, uint32 uPositionCount,
		const char* pName = nullptr);

	/// Release GPU resources (for procedural meshes not managed by AssetManager).
	void Destroy(RHIDevice* pDevice);

	const MeshLOD&        GetLOD(uint32 uIndex = 0) const { return m_vLODs[uIndex]; }
	uint32                GetLODCount()              const { return static_cast<uint32>(m_vLODs.size()); }
	const AABB&           GetAABB()                  const { return m_AABB; }
	const BoundingSphere& GetBoundingSphere()        const { return m_BoundingSphere; }
	const std::string&    GetName()                  const { return m_sName; }

private:
	std::vector<MeshLOD>  m_vLODs;
	AABB                  m_AABB;
	BoundingSphere        m_BoundingSphere;
	std::string           m_sName;

	/// Intermediate CPU data for file-based loading (cleared after OnFinalize).
	struct PendingData {
		std::vector<uint8> vVertexData;
		std::vector<uint8> vIndexData;
		uint32             uVertexCount  = 0;
		uint32             uIndexCount   = 0;
		uint32             uVertexStride = 0;
		RHIIndexFormat     indexFormat   = RHIIndexFormat::U32;
		Vec3               vBoundsMin;
		Vec3               vBoundsMax;
	};
	std::unique_ptr<PendingData> m_pPending;

	/// Shared helper: create GPU buffers from raw data and build LOD 0.
	bool CreateGPUBuffers(RHIDevice* pDevice,
	                      const void* pVertices, uint32 uVertexCount, uint32 uVertexStride,
	                      const void* pIndices,  uint32 uIndexCount,  RHIIndexFormat indexFormat);
};

} // namespace Evo
