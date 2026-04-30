#pragma once

#include "Asset/Asset.h"
#include "RHI/RHITypes.h"
#include <vector>

namespace Evo {

/// Texture asset — loads image files (PNG, JPG, TGA, BMP) via stb_image,
/// uploads pixel data to a GPU texture via staging buffer.
class TextureAsset : public Asset {
public:
	const char* GetTypeName() const override { return "Texture"; }

	bool OnLoad(const std::vector<uint8>& rawData) override;
	bool OnFinalize(RHIDevice* pDevice) override;
	void OnUnload(RHIDevice* pDevice) override;

	RHITextureHandle GetTexture() const { return m_Texture; }
	uint32 GetWidth()  const { return m_uWidth; }
	uint32 GetHeight() const { return m_uHeight; }

private:
	// CPU-side pixel data (cleared after GPU upload)
	std::vector<uint8> m_vPixelData;
	uint32 m_uWidth    = 0;
	uint32 m_uHeight   = 0;
	int    m_iChannels = 0;

	// GPU resource
	RHITextureHandle m_Texture;
};

} // namespace Evo

