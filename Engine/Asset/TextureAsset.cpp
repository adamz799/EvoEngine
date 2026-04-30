#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_TGA
#define STBI_ONLY_BMP
#include "stb_image.h"

#include "Asset/TextureAsset.h"
#include "RHI/RHIDevice.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIQueue.h"
#include "Core/Log.h"

#include <cstring>

namespace Evo {

bool TextureAsset::OnLoad(const std::vector<uint8>& rawData)
{
	int w, h, channels;
	uint8* pPixels = stbi_load_from_memory(
		rawData.data(), static_cast<int>(rawData.size()),
		&w, &h, &channels, 4);   // Force RGBA

	if (!pPixels)
	{
		EVO_LOG_ERROR("TextureAsset: failed to decode image '{}'", m_sPath);
		return false;
	}

	m_uWidth    = static_cast<uint32>(w);
	m_uHeight   = static_cast<uint32>(h);
	m_iChannels = 4;

	size_t dataSize = static_cast<size_t>(w) * h * 4;
	m_vPixelData.resize(dataSize);
	std::memcpy(m_vPixelData.data(), pPixels, dataSize);

	stbi_image_free(pPixels);
	return true;
}

bool TextureAsset::OnFinalize(RHIDevice* pDevice)
{
	if (m_vPixelData.empty()) return false;

	uint64 rowPitch  = static_cast<uint64>(m_uWidth) * 4;
	// D3D12 requires row pitch aligned to D3D12_TEXTURE_DATA_PITCH_ALIGNMENT (256)
	uint64 alignedRowPitch = (rowPitch + 255) & ~255ULL;
	uint64 stagingSize     = alignedRowPitch * m_uHeight;

	// Create staging buffer
	RHIBufferDesc stagingDesc = {};
	stagingDesc.uSize      = stagingSize;
	stagingDesc.usage      = RHIBufferUsage::CopySrc;
	stagingDesc.memory     = RHIMemoryUsage::CpuToGpu;
	stagingDesc.sDebugName = "TextureStaging";
	auto staging = pDevice->CreateBuffer(stagingDesc);
	if (!staging.IsValid()) return false;

	void* pMapped = pDevice->MapBuffer(staging);
	if (!pMapped)
	{
		pDevice->DestroyBuffer(staging);
		return false;
	}

	// Copy pixel data row by row (with alignment padding)
	for (uint32 row = 0; row < m_uHeight; ++row)
	{
		uint8* dst = static_cast<uint8*>(pMapped) + row * alignedRowPitch;
		uint8* src = m_vPixelData.data() + row * rowPitch;
		std::memcpy(dst, src, rowPitch);
	}

	// Create GPU texture
	RHITextureDesc texDesc = {};
	texDesc.uWidth     = m_uWidth;
	texDesc.uHeight    = m_uHeight;
	texDesc.format     = RHIFormat::R8G8B8A8_UNORM;
	texDesc.usage      = RHITextureUsage::ShaderResource | RHITextureUsage::CopyDst;
	texDesc.sDebugName = m_sPath;
	m_Texture = pDevice->CreateTexture(texDesc);
	if (!m_Texture.IsValid())
	{
		pDevice->DestroyBuffer(staging);
		return false;
	}

	// Record copy command
	auto* pCmdList = pDevice->AcquireCommandList();

	// Barrier: texture Common -> CopyDest
	RHITextureBarrier barrier = {};
	barrier.texture      = m_Texture;
	barrier.syncBefore   = RHIBarrierSync::None;
	barrier.syncAfter    = RHIBarrierSync::Copy;
	barrier.accessBefore = RHIBarrierAccess::Common;
	barrier.accessAfter  = RHIBarrierAccess::CopyDest;
	barrier.layoutBefore = RHITextureLayout::Common;
	barrier.layoutAfter  = RHITextureLayout::CopyDest;
	pCmdList->TextureBarrier(barrier);

	pCmdList->CopyBufferToTexture(staging, m_Texture);

	// Barrier: CopyDest -> ShaderResource
	barrier.syncBefore   = RHIBarrierSync::Copy;
	barrier.syncAfter    = RHIBarrierSync::AllShading;
	barrier.accessBefore = RHIBarrierAccess::CopyDest;
	barrier.accessAfter  = RHIBarrierAccess::ShaderResource;
	barrier.layoutBefore = RHITextureLayout::CopyDest;
	barrier.layoutAfter  = RHITextureLayout::ShaderResource;
	pCmdList->TextureBarrier(barrier);

	pCmdList->End();

	// Submit and wait
	auto* pQueue = pDevice->GetGraphicsQueue();
	RHICommandList* pCmdListPtr = pCmdList;
	pQueue->Submit(&pCmdListPtr, 1);
	pQueue->WaitIdle();

	// Cleanup staging
	pDevice->DestroyBuffer(staging);
	m_vPixelData.clear();
	m_vPixelData.shrink_to_fit();

	EVO_LOG_INFO("TextureAsset: loaded '{}' ({}x{})", m_sPath, m_uWidth, m_uHeight);
	return true;
}

void TextureAsset::OnUnload(RHIDevice* pDevice)
{
	if (m_Texture.IsValid())
	{
		pDevice->DestroyTexture(m_Texture);
		m_Texture = {};
	}
	m_vPixelData.clear();
}

} // namespace Evo

