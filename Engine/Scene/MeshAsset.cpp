#include "Scene/MeshAsset.h"
#include "RHI/RHIDevice.h"
#include "Core/Log.h"
#include <cstring>

namespace Evo {

// ============================================================================
// Shared GPU buffer creation
// ============================================================================

bool MeshAsset::CreateGPUBuffers(RHIDevice* pDevice,
                                 const void* pVertices, uint32 uVertexCount, uint32 uVertexStride,
                                 const void* pIndices,  uint32 uIndexCount,  RHIIndexFormat indexFormat)
{
	// Vertex buffer
	uint64 vbSize = static_cast<uint64>(uVertexCount) * uVertexStride;
	RHIBufferDesc vbDesc = {};
	vbDesc.uSize      = vbSize;
	vbDesc.usage      = RHIBufferUsage::Vertex;
	vbDesc.memory     = RHIMemoryUsage::CpuToGpu;
	vbDesc.sDebugName = m_sName + "_VB";

	RHIBufferHandle vb = pDevice->CreateBuffer(vbDesc);
	if (!vb.IsValid())
	{
		EVO_LOG_ERROR("MeshAsset: failed to create vertex buffer for '{}'", m_sName);
		return false;
	}

	void* mapped = pDevice->MapBuffer(vb);
	std::memcpy(mapped, pVertices, vbSize);

	// Index buffer
	uint32 indexStride = (indexFormat == RHIIndexFormat::U16) ? 2 : 4;
	uint64 ibSize = static_cast<uint64>(uIndexCount) * indexStride;
	RHIBufferDesc ibDesc = {};
	ibDesc.uSize      = ibSize;
	ibDesc.usage      = RHIBufferUsage::Index;
	ibDesc.memory     = RHIMemoryUsage::CpuToGpu;
	ibDesc.sDebugName = m_sName + "_IB";

	RHIBufferHandle ib = pDevice->CreateBuffer(ibDesc);
	if (!ib.IsValid())
	{
		EVO_LOG_ERROR("MeshAsset: failed to create index buffer for '{}'", m_sName);
		pDevice->DestroyBuffer(vb);
		return false;
	}

	mapped = pDevice->MapBuffer(ib);
	std::memcpy(mapped, pIndices, ibSize);

	// Build LOD 0
	MeshLOD lod;
	lod.vertexBuffer  = vb;
	lod.indexBuffer   = ib;
	lod.uVertexCount  = uVertexCount;
	lod.uIndexCount   = uIndexCount;
	lod.uVertexStride = uVertexStride;

	SubMesh subMesh;
	subMesh.uIndexOffset  = 0;
	subMesh.uIndexCount   = uIndexCount;
	subMesh.uVertexOffset = 0;
	lod.vSubMeshes.push_back(subMesh);

	m_vLODs.push_back(std::move(lod));
	return true;
}

// ============================================================================
// Asset lifecycle — file-based loading (.emesh)
// ============================================================================

bool MeshAsset::OnLoad(const std::vector<uint8>& rawData)
{
	if (rawData.size() < sizeof(EvoMeshHeader))
	{
		EVO_LOG_ERROR("MeshAsset::OnLoad — file too small for header ({})", m_sPath);
		return false;
	}

	EvoMeshHeader header;
	std::memcpy(&header, rawData.data(), sizeof(header));

	if (std::memcmp(header.magic, "EMSH", 4) != 0)
	{
		EVO_LOG_ERROR("MeshAsset::OnLoad — invalid magic in '{}'", m_sPath);
		return false;
	}

	if (header.uVersion != 1)
	{
		EVO_LOG_ERROR("MeshAsset::OnLoad — unsupported version {} in '{}'", header.uVersion, m_sPath);
		return false;
	}

	RHIIndexFormat indexFormat = (header.uIndexFormat == 0) ? RHIIndexFormat::U16 : RHIIndexFormat::U32;
	uint32 indexStride = (indexFormat == RHIIndexFormat::U16) ? 2 : 4;

	uint64 vertexDataSize = static_cast<uint64>(header.uVertexCount) * header.uVertexStride;
	uint64 indexDataSize  = static_cast<uint64>(header.uIndexCount)  * indexStride;
	uint64 expectedSize   = sizeof(EvoMeshHeader) + vertexDataSize + indexDataSize;

	if (rawData.size() < expectedSize)
	{
		EVO_LOG_ERROR("MeshAsset::OnLoad — file truncated in '{}' (expected {} bytes, got {})",
		              m_sPath, expectedSize, rawData.size());
		return false;
	}

	m_pPending = std::make_unique<PendingData>();
	m_pPending->uVertexCount  = header.uVertexCount;
	m_pPending->uIndexCount   = header.uIndexCount;
	m_pPending->uVertexStride = header.uVertexStride;
	m_pPending->indexFormat   = indexFormat;
	m_pPending->vBoundsMin    = Vec3(header.boundsMin[0], header.boundsMin[1], header.boundsMin[2]);
	m_pPending->vBoundsMax    = Vec3(header.boundsMax[0], header.boundsMax[1], header.boundsMax[2]);

	const uint8* pData = rawData.data() + sizeof(EvoMeshHeader);
	m_pPending->vVertexData.assign(pData, pData + vertexDataSize);
	pData += vertexDataSize;
	m_pPending->vIndexData.assign(pData, pData + indexDataSize);

	return true;
}

bool MeshAsset::OnFinalize(RHIDevice* pDevice)
{
	if (!m_pPending)
		return false;

	m_sName = m_sPath;

	// Compute bounds from header values
	m_AABB.vMin = m_pPending->vBoundsMin;
	m_AABB.vMax = m_pPending->vBoundsMax;
	m_BoundingSphere = BoundingSphere::FromAABB(m_AABB);

	bool bSuccess = CreateGPUBuffers(pDevice,
		m_pPending->vVertexData.data(), m_pPending->uVertexCount, m_pPending->uVertexStride,
		m_pPending->vIndexData.data(),  m_pPending->uIndexCount,  m_pPending->indexFormat);

	// Release intermediate data
	m_pPending.reset();

	if (bSuccess)
		EVO_LOG_INFO("MeshAsset '{}' loaded from file: {} vertices, {} indices",
		             m_sName, m_vLODs[0].uVertexCount, m_vLODs[0].uIndexCount);

	return bSuccess;
}

void MeshAsset::OnUnload(RHIDevice* pDevice)
{
	Destroy(pDevice);
}

// ============================================================================
// Procedural creation (bypasses Asset lifecycle)
// ============================================================================

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

	if (!mesh->CreateGPUBuffers(pDevice, pVertices, uVertexCount, uVertexStride,
	                            pIndices, uIndexCount, indexFormat))
		return nullptr;

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
