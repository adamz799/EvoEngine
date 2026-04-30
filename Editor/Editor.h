#pragma once

#include "RHI/RHITypes.h"
#include "Renderer/Camera.h"
#include "Renderer/ViewportFrame.h"
#include "Scene/Scene.h"
#include "ImGuiLogSink.h"

#include <memory>

#if EVO_RHI_DX12
#include "RHI/DX12/DX12GpuDescriptorAllocator.h"
#endif

namespace Evo {

class RHIDevice;
class RHICommandList;
class RenderPipeline;
class Render;
class Window;

/// Editor — ImGui-based editor UI with scene hierarchy, inspector, and log panels.
class Editor {
public:
	bool Initialize(Render* pRender, Window& window,
					const RenderPipeline& pipeline);
	void Shutdown();

	void BeginFrame();
	void Update(Scene& scene, const Camera& camera, float fDeltaTime);
	void RenderUI(RHICommandList* pCmdList);

	/// Composite the editor viewport + ImGui onto the swap chain back buffer.
	void CompositeToBackBuffer(Render* pRender);

	EntityHandle GetSelectedEntity() const { return m_SelectedEntity; }
	int          GetHoveredAxis()   const { return m_iHoveredAxis; }

	// Viewport texture (final output for ImGui display)
	RHITextureHandle    GetViewportTexture() const { return m_ViewportTexture; }
	RHIRenderTargetView GetViewportRTV()     const { return m_ViewportRTV; }

	// Intermediate rendering resources
	ViewportFrame& GetViewportFrame() { return m_ViewportFrame; }

	uint32 GetViewportWidth()  const { return m_uViewportWidth; }
	uint32 GetViewportHeight() const { return m_uViewportHeight; }

private:
	void DrawHierarchyPanel(Scene& scene);
	void DrawInspectorPanel(Scene& scene);
	void DrawMaterialEditorPanel(Scene& scene);
	void DrawLogPanel();
	void DoViewportPicking(Scene& scene, const Camera& camera, float u, float v);

	// Gizmo helpers
	int  TestGizmoAxisHit(const Vec3& rayOrigin, const Vec3& rayDir,
						  const Vec3& gizmoPos, float fSize, float fThreshold) const;
	void ComputeViewportRay(const Camera& camera, float u, float v,
							Vec3& outOrigin, Vec3& outDir) const;

	void CreateViewportTexture(uint32 uWidth, uint32 uHeight);
	void DestroyViewportTexture();

	RHIDevice*   m_pDevice    = nullptr;
	Render*      m_pRender  = nullptr;
	RHIFormat    m_RTFormat   = RHIFormat::R8G8B8A8_UNORM;

	// Panel visibility
	bool m_bShowHierarchy = true;
	bool m_bShowInspector = true;
	bool m_bShowMaterialEditor = true;
	bool m_bShowLog       = true;

	// Selection
	EntityHandle m_SelectedEntity;
	bool         m_bFirstFrame = true;

	// Gizmo state
	bool  m_bDraggingGizmo  = false;
	int   m_iDragAxis       = -1;      // 0=X, 1=Y, 2=Z
	int   m_iHoveredAxis    = -1;
	Vec3  m_vDragOrigin;               // entity position at drag start
	Vec3  m_vDragRayHit;               // initial closest point on axis

	// Log sink for ImGui panel
	std::shared_ptr<ImGuiLogSink> m_pLogSink;

	// Off-screen viewport texture for ImGui display (NOT intermediate render resources)
	RHITextureHandle    m_ViewportTexture;
	RHIRenderTargetView m_ViewportRTV;

	// Intermediate rendering resources (GBuffer, Depth, HDR, descriptor sets)
	ViewportFrame m_ViewportFrame;

	uint32 m_uViewportWidth  = 0;
	uint32 m_uViewportHeight = 0;

#if EVO_RHI_DX12
	DX12GpuDescriptorAllocator::Allocation m_ViewportSRV = {};
#endif
};

} // namespace Evo

