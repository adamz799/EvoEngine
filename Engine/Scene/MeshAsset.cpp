#include "Scene/MeshAsset.h"
#include "RHI/RHIDevice.h"
#include "Core/Log.h"
#include "Mesh_generated.h"
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
// FlatBuffers format helpers
// ============================================================================

static float HalfToFloat(uint16 h)
{
	uint32 sign     = (h & 0x8000u) << 16;
	uint32 exponent = (h >> 10) & 0x1F;
	uint32 mantissa = h & 0x3FF;

	uint32 result;
	if (exponent == 0)
	{
		if (mantissa == 0)
			result = sign;
		else
		{
			exponent = 1;
			while (!(mantissa & 0x400)) { mantissa <<= 1; exponent--; }
			mantissa &= 0x3FF;
			result = sign | ((exponent + 127 - 15) << 23) | (mantissa << 13);
		}
	}
	else if (exponent == 31)
		result = sign | 0x7F800000u | (mantissa << 13);
	else
		result = sign | ((exponent + 127 - 15) << 23) | (mantissa << 13);

	float f;
	std::memcpy(&f, &result, 4);
	return f;
}

static uint32 GetFormatStride(Evo::Schema::AttributeFormat format)
{
	using enum Evo::Schema::AttributeFormat;
	switch (format)
	{
	case AttributeFormat_Float16x2: return 4;
	case AttributeFormat_Float16x3: return 6;
	case AttributeFormat_Float16x4: return 8;
	case AttributeFormat_Float32x2: return 8;
	case AttributeFormat_Float32x3: return 12;
	case AttributeFormat_Float32x4: return 16;
	case AttributeFormat_UNorm8x4:  return 4;
	case AttributeFormat_SNorm8x4:  return 4;
	case AttributeFormat_UNorm16x2: return 4;
	case AttributeFormat_UNorm16x4: return 8;
	default: return 0;
	}
}

/// Decode one vertex element into float32 components.
/// Writes exactly uOutComponents floats to pOut.
static void DecodeElement(const uint8* pElement, Evo::Schema::AttributeFormat format,
                          float* pOut, uint32 uOutComponents)
{
	using enum Evo::Schema::AttributeFormat;
	switch (format)
	{
	case AttributeFormat_Float32x2:
	case AttributeFormat_Float32x3:
	case AttributeFormat_Float32x4:
		std::memcpy(pOut, pElement, uOutComponents * sizeof(float));
		break;

	case AttributeFormat_Float16x2:
	case AttributeFormat_Float16x3:
	case AttributeFormat_Float16x4:
	{
		auto* pHalf = reinterpret_cast<const uint16*>(pElement);
		for (uint32 c = 0; c < uOutComponents; ++c)
			pOut[c] = HalfToFloat(pHalf[c]);
		break;
	}

	default:
		for (uint32 c = 0; c < uOutComponents; ++c)
			pOut[c] = 0.0f;
		break;
	}
}

static bool IsFloat3Format(Evo::Schema::AttributeFormat format)
{
	return format == Evo::Schema::AttributeFormat_Float32x3
	    || format == Evo::Schema::AttributeFormat_Float16x3;
}

static bool IsFloat2Format(Evo::Schema::AttributeFormat format)
{
	return format == Evo::Schema::AttributeFormat_Float32x2
	    || format == Evo::Schema::AttributeFormat_Float16x2;
}

// ============================================================================
// Asset lifecycle — FlatBuffers-based loading (.emesh)
// ============================================================================

bool MeshAsset::OnLoad(const std::vector<uint8>& rawData)
{
	// ---- Verify FlatBuffer ----
	flatbuffers::Verifier verifier(rawData.data(), rawData.size());
	if (!Evo::Schema::VerifyMeshBuffer(verifier))
	{
		EVO_LOG_ERROR("MeshAsset::OnLoad — FlatBuffer verification failed for '{}'", m_sPath);
		return false;
	}

	auto* pMesh = Evo::Schema::GetMesh(rawData.data());
	if (!pMesh)
	{
		EVO_LOG_ERROR("MeshAsset::OnLoad — null root in '{}'", m_sPath);
		return false;
	}

	// ---- Validate attribute formats ----
	auto* pPositions = pMesh->positions();
	auto* pNormals   = pMesh->normals();
	auto* pUVs       = pMesh->uvs();

	if (!IsFloat3Format(pPositions->format()))
	{
		EVO_LOG_ERROR("MeshAsset::OnLoad — positions must be Float32x3 or Float16x3 in '{}'", m_sPath);
		return false;
	}
	if (!IsFloat3Format(pNormals->format()))
	{
		EVO_LOG_ERROR("MeshAsset::OnLoad — normals must be Float32x3 or Float16x3 in '{}'", m_sPath);
		return false;
	}
	if (!IsFloat2Format(pUVs->format()))
	{
		EVO_LOG_ERROR("MeshAsset::OnLoad — uvs must be Float32x2 or Float16x2 in '{}'", m_sPath);
		return false;
	}

	uint32 uPosStride = GetFormatStride(pPositions->format());
	uint32 uNrmStride = GetFormatStride(pNormals->format());
	uint32 uUvStride  = GetFormatStride(pUVs->format());

	uint32 uVertexCount = static_cast<uint32>(pPositions->data()->size()) / uPosStride;

	// Validate array sizes match
	if (pNormals->data()->size() / uNrmStride != uVertexCount ||
	    pUVs->data()->size() / uUvStride != uVertexCount)
	{
		EVO_LOG_ERROR("MeshAsset::OnLoad — vertex attribute count mismatch in '{}'", m_sPath);
		return false;
	}

	// ---- Interleave SoA → AoS (always output float32) ----
	// Layout: float32[3] position + float32[3] normal + float32[2] uv = 32 bytes
	constexpr uint32 kVertexStride = 32;

	m_pPending = std::make_unique<PendingData>();
	m_pPending->uVertexCount  = uVertexCount;
	m_pPending->uVertexStride = kVertexStride;
	m_pPending->vVertexData.resize(static_cast<size_t>(uVertexCount) * kVertexStride);

	const uint8* pPosData = pPositions->data()->data();
	const uint8* pNrmData = pNormals->data()->data();
	const uint8* pUvData  = pUVs->data()->data();
	uint8* pDst = m_pPending->vVertexData.data();

	for (uint32 i = 0; i < uVertexCount; ++i)
	{
		float* pVertex = reinterpret_cast<float*>(pDst + i * kVertexStride);
		DecodeElement(pPosData + i * uPosStride, pPositions->format(), pVertex,     3);
		DecodeElement(pNrmData + i * uNrmStride, pNormals->format(),   pVertex + 3, 3);
		DecodeElement(pUvData  + i * uUvStride,  pUVs->format(),       pVertex + 6, 2);
	}

	// ---- Index data ----
	auto* pIB = pMesh->indices();
	uint32 uIndexStride = (pIB->format() == Evo::Schema::IndexFormat_UInt16) ? 2 : 4;
	m_pPending->indexFormat = (pIB->format() == Evo::Schema::IndexFormat_UInt16)
	                        ? RHIIndexFormat::U16 : RHIIndexFormat::U32;
	m_pPending->uIndexCount = static_cast<uint32>(pIB->data()->size()) / uIndexStride;
	m_pPending->vIndexData.assign(pIB->data()->data(),
	                              pIB->data()->data() + pIB->data()->size());

	// ---- Bounds ----
	auto* pBounds = pMesh->bounds();
	if (pBounds)
	{
		auto& bMin = pBounds->min();
		auto& bMax = pBounds->max();
		m_pPending->vBoundsMin = Vec3(bMin.x(), bMin.y(), bMin.z());
		m_pPending->vBoundsMax = Vec3(bMax.x(), bMax.y(), bMax.z());
	}
	else
	{
		// Compute from decoded positions
		Vec3 vMin(Infinity);
		Vec3 vMax(-Infinity);
		for (uint32 i = 0; i < uVertexCount; ++i)
		{
			float* pVertex = reinterpret_cast<float*>(pDst + i * kVertexStride);
			Vec3 p(pVertex[0], pVertex[1], pVertex[2]);
			vMin = Vec3::Min(vMin, p);
			vMax = Vec3::Max(vMax, p);
		}
		m_pPending->vBoundsMin = vMin;
		m_pPending->vBoundsMax = vMax;
	}

	// ---- Sub-meshes ----
	auto* pSubMeshes = pMesh->sub_meshes();
	if (pSubMeshes && pSubMeshes->size() > 0)
	{
		for (uint32 i = 0; i < pSubMeshes->size(); ++i)
		{
			auto* sm = pSubMeshes->Get(i);
			SubMesh subMesh;
			subMesh.uIndexOffset  = sm->index_offset();
			subMesh.uIndexCount   = sm->index_count();
			subMesh.uVertexOffset = sm->vertex_offset();
			m_pPending->vSubMeshes.push_back(subMesh);
		}
	}

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

	// Override auto-generated single sub-mesh if file specified explicit ranges
	if (bSuccess && !m_pPending->vSubMeshes.empty())
		m_vLODs.back().vSubMeshes = std::move(m_pPending->vSubMeshes);

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
