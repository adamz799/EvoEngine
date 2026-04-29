#pragma once

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

/// MeshAsset — GPU-resident mesh data with bounds and LOD support.
class MeshAsset {
public:
	/// Create a single-LOD mesh from raw vertex + index data. Uploads to GPU.
	static std::unique_ptr<MeshAsset> CreateFromMemory(
		RHIDevice* pDevice,
		const void* pVertices, uint32 uVertexCount, uint32 uVertexStride,
		const void* pIndices,  uint32 uIndexCount,  RHIIndexFormat indexFormat,
		const Vec3* pPositions, uint32 uPositionCount,
		const char* pName = nullptr);

	/// Release GPU resources.
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
};

} // namespace Evo
