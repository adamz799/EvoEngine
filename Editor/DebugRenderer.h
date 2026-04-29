#pragma once

#include "Asset/AssetManager.h"
#include "Renderer/Camera.h"
#include "Renderer/RenderGraph.h"
#include "Math/Math.h"
#include <vector>

namespace Evo {

class Renderer;

struct DebugLineVertex {
	float fPosX, fPosY, fPosZ;
	float fColorR, fColorG, fColorB, fColorA;
};

/// DebugRenderer -- draws colored lines (frustum, camera icon, etc.) into a render target.
class DebugRenderer {
public:
	bool Initialize(RHIDevice* pDevice, AssetManager& assetManager, RHIFormat rtFormat);
	void Shutdown(RHIDevice* pDevice);

	void DrawLine(const Vec3& from, const Vec3& to, const Vec4& color);
	void DrawFrustum(const Camera& camera, const Vec4& color);
	void DrawCameraIcon(const Camera& camera, const Vec4& color, float fSize);

	void Render(Renderer& renderer, RGHandle target, RHIRenderTargetView rtv,
	            const Mat4& viewProj, float fViewportWidth, float fViewportHeight);

private:
	RHIPipelineHandle m_LinePipeline;
	RHIBufferHandle   m_LineBuffer;
	void*             m_pMappedData = nullptr;
	AssetHandle       m_ShaderHandle;
	std::vector<DebugLineVertex> m_vVertices;
	static constexpr uint32 kMaxVertices = 2048;
};

} // namespace Evo
