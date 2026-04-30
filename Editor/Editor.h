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
class Window;

/// Editor — ImGui-based editor UI with scene hierarchy, inspector, and log panels.
class Editor {
public:
	bool Initialize(RHIDevice* pDevice, Window& window, RHIFormat rtFormat,
	                const RenderPipeline& pipeline);
	void Shutdown();

	void BeginFrame();
	void Update(Scene& scene, const Camera& camera, float fDeltaTime);
	void Render(RHICommandList* pCmdList);

	EntityHandle GetSelectedEntity() const { return m_SelectedEntity; }

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

	void CreateViewportTexture(uint32 uWidth, uint32 uHeight);
	void DestroyViewportTexture();

	RHIDevice*   m_pDevice   = nullptr;
	RHIFormat    m_RTFormat  = RHIFormat::R8G8B8A8_UNORM;

	// Panel visibility
	bool m_bShowHierarchy = true;
	bool m_bShowInspector = true;
	bool m_bShowMaterialEditor = true;
	bool m_bShowLog       = true;

	// Selection
	EntityHandle m_SelectedEntity;
	bool         m_bFirstFrame = true;

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
