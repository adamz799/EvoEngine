#include "Scene/MeshAsset.h"
#include "RHI/RHIDevice.h"
#include "Core/Log.h"
#include <cstring>

namespace Evo {

std::unique_ptr<MeshAsset> MeshAsset::CreateFromMemory(
	RHIDevice* pDevice,
	const void* pVertices, uint32 uVertexCount, uint32 uVertexStride,
	const void* pIndices,  uint32 uIndexCount,  RHIIndexFormat indexFormat,
	const Vec3* pPositions, uint32 uPositionCount,
	const char* pName)
{
	auto mesh = std::make_unique<MeshAsset>();
	mesh->m_sName = pName ? pName : "Unnamed";

	// Compute bounds from positions
	if (pPositions && uPositionCount > 0)
	{
		mesh->m_AABB = AABB::FromPoints(pPositions, uPositionCount);
		mesh->m_BoundingSphere = BoundingSphere::FromAABB(mesh->m_AABB);
	}

	// Create vertex buffer
	uint64 vbSize = static_cast<uint64>(uVertexCount) * uVertexStride;
	RHIBufferDesc vbDesc = {};
	vbDesc.uSize      = vbSize;
	vbDesc.usage      = RHIBufferUsage::Vertex;
	vbDesc.memory     = RHIMemoryUsage::CpuToGpu;
	vbDesc.sDebugName = mesh->m_sName + "_VB";

	RHIBufferHandle vb = pDevice->CreateBuffer(vbDesc);
	if (!vb.IsValid())
	{
		EVO_LOG_ERROR("MeshAsset: failed to create vertex buffer for '{}'", mesh->m_sName);
		return nullptr;
	}

	void* mapped = pDevice->MapBuffer(vb);
	std::memcpy(mapped, pVertices, vbSize);

	// Create index buffer
	uint32 indexStride = (indexFormat == RHIIndexFormat::U16) ? 2 : 4;
	uint64 ibSize = static_cast<uint64>(uIndexCount) * indexStride;
	RHIBufferDesc ibDesc = {};
	ibDesc.uSize      = ibSize;
	ibDesc.usage      = RHIBufferUsage::Index;
	ibDesc.memory     = RHIMemoryUsage::CpuToGpu;
	ibDesc.sDebugName = mesh->m_sName + "_IB";

	RHIBufferHandle ib = pDevice->CreateBuffer(ibDesc);
	if (!ib.IsValid())
	{
		EVO_LOG_ERROR("MeshAsset: failed to create index buffer for '{}'", mesh->m_sName);
		pDevice->DestroyBuffer(vb);
		return nullptr;
	}

	mapped = pDevice->MapBuffer(ib);
	std::memcpy(mapped, pIndices, ibSize);

	// Build LOD 0
	MeshLOD lod;
	lod.vertexBuffer = vb;
	lod.indexBuffer  = ib;
	lod.uVertexCount = uVertexCount;
	lod.uIndexCount  = uIndexCount;
	lod.uVertexStride = uVertexStride;

	SubMesh subMesh;
	subMesh.uIndexOffset  = 0;
	subMesh.uIndexCount   = uIndexCount;
	subMesh.uVertexOffset = 0;
	lod.vSubMeshes.push_back(subMesh);

	mesh->m_vLODs.push_back(std::move(lod));

	EVO_LOG_INFO("MeshAsset '{}' created: {} vertices, {} indices",
	             mesh->m_sName, uVertexCount, uIndexCount);
	return mesh;
}

void MeshAsset::Destroy(RHIDevice* pDevice)
{
	for (auto& lod : m_vLODs)
	{
		if (lod.vertexBuffer.IsValid()) pDevice->DestroyBuffer(lod.vertexBuffer);
		if (lod.indexBuffer.IsValid())  pDevice->DestroyBuffer(lod.indexBuffer);
	}
	m_vLODs.clear();
}

} // namespace Evo
