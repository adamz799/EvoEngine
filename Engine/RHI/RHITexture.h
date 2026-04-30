#pragma once

#include "RHI/RHITypes.h"

namespace Evo {

class RHIDevice;

/// Thin wrapper around RHITextureHandle for Renderer-level ergonomics.
///
/// Same philosophy as RHIBuffer: holds Handle + descriptor.
/// Does NOT own the GPU resource — destroy via Device::DestroyTexture(GetHandle()).
class RHITexture {
public:
	RHITexture() = default;
	RHITexture(RHITextureHandle handle, const RHITextureDesc& desc)
		: m_Handle(handle), m_Desc(desc) {}

	RHITextureHandle      GetHandle()    const { return m_Handle; }
	bool                  IsValid()      const { return m_Handle.IsValid(); }

	uint32                   GetWidth()     const { return m_Desc.uWidth; }
	uint32                   GetHeight()    const { return m_Desc.uHeight; }
	uint32                   GetMipLevels() const { return m_Desc.uMipLevels; }
	RHIFormat             GetFormat()    const { return m_Desc.format; }
	RHITextureUsage       GetUsage()     const { return m_Desc.usage; }
	RHITextureDimension   GetDimension() const { return m_Desc.dimension; }
	const std::string&    GetName()      const { return m_Desc.sDebugName; }
	const RHITextureDesc& GetDesc()      const { return m_Desc; }

private:
	RHITextureHandle m_Handle;
	RHITextureDesc   m_Desc;
};

} // namespace Evo

