#pragma once

#include "RHI/RHITypes.h"
#include "Renderer/Camera.h"
#include "Scene/Scene.h"
#include "ImGuiLogSink.h"

#include <memory>

#if EVO_RHI_DX12
#include "RHI/DX12/DX12GpuDescriptorAllocator.h"
#endif

namespace Evo {

class RHIDevice;
class RHICommandList;
class Window;

/// Editor — ImGui-based editor UI with scene hierarchy, inspector, and log panels.
class Editor {
public:
	bool Initialize(RHIDevice* pDevice, Window& window, RHIFormat rtFormat);
	void Shutdown();

	void BeginFrame();
	void Update(Scene& scene, const Camera& camera, float fDeltaTime);
	void Render(RHICommandList* pCmdList);

	EntityHandle GetSelectedEntity() const { return m_SelectedEntity; }

	// Viewport texture accessors
	RHITextureHandle    GetViewportTexture() const { return m_ViewportTexture; }
	RHIRenderTargetView GetViewportRTV()     const { return m_ViewportRTV; }
	uint32              GetViewportWidth()   const { return m_uViewportWidth; }
	uint32              GetViewportHeight()  const { return m_uViewportHeight; }

private:
	void DrawHierarchyPanel(Scene& scene);
	void DrawInspectorPanel(Scene& scene);
	void DrawLogPanel();
	void DoViewportPicking(Scene& scene, const Camera& camera, float u, float v);

	void CreateViewportResources(uint32 uWidth, uint32 uHeight);
	void DestroyViewportResources();

	RHIDevice*   m_pDevice   = nullptr;
	RHIFormat    m_RTFormat  = RHIFormat::R8G8B8A8_UNORM;

	// Panel visibility
	bool m_bShowHierarchy = true;
	bool m_bShowInspector = true;
	bool m_bShowLog       = true;

	// Selection
	EntityHandle m_SelectedEntity;

	// Log sink for ImGui panel
	std::shared_ptr<ImGuiLogSink> m_pLogSink;

	// Off-screen viewport texture
	RHITextureHandle    m_ViewportTexture;
	RHIRenderTargetView m_ViewportRTV;
	uint32              m_uViewportWidth  = 0;
	uint32              m_uViewportHeight = 0;

#if EVO_RHI_DX12
	DX12GpuDescriptorAllocator::Allocation m_ViewportSRV = {};
#endif
};

} // namespace Evo
