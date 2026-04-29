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

	// Scene viewport texture accessors
	RHITextureHandle    GetViewportTexture() const { return m_ViewportTexture; }
	RHIRenderTargetView GetViewportRTV()     const { return m_ViewportRTV; }
	RHITextureHandle    GetDepthTexture()    const { return m_DepthTexture; }
	RHIDepthStencilView GetDepthDSV()        const { return m_DepthDSV; }

	// G-Buffer texture accessors
	RHITextureHandle    GetGBAlbedoTexture()  const { return m_GBAlbedoTexture; }
	RHIRenderTargetView GetGBAlbedoRTV()      const { return m_GBAlbedoRTV; }
	RHITextureHandle    GetGBNormalTexture()  const { return m_GBNormalTexture; }
	RHIRenderTargetView GetGBNormalRTV()      const { return m_GBNormalRTV; }
	RHITextureHandle    GetGBRoughMetTexture() const { return m_GBRoughMetTexture; }
	RHIRenderTargetView GetGBRoughMetRTV()     const { return m_GBRoughMetRTV; }

	// HDR intermediate texture accessors
	RHITextureHandle    GetHDRTexture() const { return m_HDRTexture; }
	RHIRenderTargetView GetHDRRTV()     const { return m_HDRRTV; }

	uint32              GetViewportWidth()   const { return m_uViewportWidth; }
	uint32              GetViewportHeight()  const { return m_uViewportHeight; }

private:
	void DrawHierarchyPanel(Scene& scene);
	void DrawInspectorPanel(Scene& scene);
	void DrawMaterialEditorPanel(Scene& scene);
	void DrawLogPanel();
	void DoViewportPicking(Scene& scene, const Camera& camera, float u, float v);

	void CreateViewportResources(uint32 uWidth, uint32 uHeight);
	void DestroyViewportResources();

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

	// Off-screen scene viewport texture + depth buffer
	RHITextureHandle    m_ViewportTexture;
	RHIRenderTargetView m_ViewportRTV;
	RHITextureHandle    m_DepthTexture;
	RHIDepthStencilView m_DepthDSV;

	// G-Buffer textures (same size as viewport)
	RHITextureHandle    m_GBAlbedoTexture;
	RHIRenderTargetView m_GBAlbedoRTV;
	RHITextureHandle    m_GBNormalTexture;
	RHIRenderTargetView m_GBNormalRTV;
	RHITextureHandle    m_GBRoughMetTexture;
	RHIRenderTargetView m_GBRoughMetRTV;

	// HDR intermediate (same size as viewport)
	RHITextureHandle    m_HDRTexture;
	RHIRenderTargetView m_HDRRTV;

	uint32              m_uViewportWidth  = 0;
	uint32              m_uViewportHeight = 0;

#if EVO_RHI_DX12
	DX12GpuDescriptorAllocator::Allocation m_ViewportSRV = {};
#endif
};

} // namespace Evo
