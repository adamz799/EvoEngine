#pragma once

#include "Asset/AssetManager.h"
#include "Renderer/Camera.h"
#include "Renderer/RenderGraph.h"
#include "Math/Math.h"
#include <vector>

namespace Evo {

class Renderer;

struct DebugLineVertex {
	float fPosX, fPosY, fPosZ;                // this endpoint (world)
	float fOtherX, fOtherY, fOtherZ;          // other endpoint (world)
	float fColorR, fColorG, fColorB, fColorA;  // vertex color
	float fEdgeDist;                           // ±1.0 for smooth edges
};

/// DebugRenderer -- draws anti-aliased colored lines (frustum, camera icon, gizmos).
class DebugRenderer {
public:
	bool Initialize(RHIDevice* pDevice, AssetManager& assetManager, RHIFormat rtFormat);
	void Shutdown(RHIDevice* pDevice);

	void DrawLine(const Vec3& from, const Vec3& to, const Vec4& color);
	void DrawFrustum(const Camera& camera, const Vec4& color);
	void DrawCameraIcon(const Camera& camera, const Vec4& color, float fSize);
	void DrawTranslationGizmo(const Vec3& position, float fSize, int iHighlightAxis = -1);

	void Render(Renderer& renderer, RGHandle target, RHIRenderTargetView rtv,
				const Mat4& viewProj, float fViewportWidth, float fViewportHeight);

private:
	struct RawLine { Vec3 from, to; Vec4 color; };

	RHIPipelineHandle m_LinePipeline;
	RHIBufferHandle   m_LineBuffer;
	void*             m_pMappedData = nullptr;
	AssetHandle       m_ShaderHandle;
	std::vector<RawLine> m_vLines;
	static constexpr uint32 kMaxVertices = 8192;
};

} // namespace Evo

