#pragma once

#include "RHI/RHI.h"
#include "Renderer/RenderGraph.h"
#include "Platform/Window.h"
#include <memory>

namespace Evo {

struct RendererDesc {
	RHIBackendType backend                   = RHIBackendType::DX12;
	bool           bEnableDebugLayer          = true;
	bool           bEnableGPUBasedValidation  = false;
};

// ============================================================================
// WindowTarget — secondary output window (e.g. editor runtime preview).
// Owned by caller, created by Renderer. Hides swap chain lifecycle.
// ============================================================================

class WindowTarget {
public:
	uint32 GetWidth()  const { return m_uWidth; }
	uint32 GetHeight() const { return m_uHeight; }

	RHITextureHandle   GetCurrentBackBuffer()    const { return m_pSwapChain->GetCurrentBackBuffer(); }
	RHIRenderTargetView GetCurrentBackBufferRTV() const { return m_pSwapChain->GetCurrentBackBufferRTV(); }

public:
	WindowTarget() = default;

private:
	friend class Renderer;
	std::unique_ptr<RHISwapChain> m_pSwapChain;
	uint32 m_uWidth  = 0;
	uint32 m_uHeight = 0;
};

// ============================================================================
// Renderer
// ============================================================================

class Renderer {
public:
	Renderer() = default;
	~Renderer();

	bool Initialize(const RendererDesc& desc, Window& window);
	void Shutdown();

	void BeginFrame();
	void EndFrame();

	/// Resize swap chain to match current window size. Call when window resizes.
	void HandleResize(uint32 uWidth, uint32 uHeight);

	/// Create a secondary window target (its own swap chain).
	WindowTarget CreateWindowTarget(void* nativeWindow, uint32 uWidth, uint32 uHeight);

	/// Resize a secondary window target.
	void ResizeWindowTarget(WindowTarget& t, uint32 uWidth, uint32 uHeight);

	/// Present a secondary window target.
	void PresentWindowTarget(WindowTarget& t);

	RHIDevice* GetDevice() const { return m_pRHIDevice.get(); }
	RenderGraph& GetRenderGraph()        { return m_RenderGraph; }
	RGHandle     GetBackBufferRG() const { return m_BackBufferRG; }
	RHISwapChain* GetSwapChain() const   { return m_pSwapChain.get(); }

private:
	std::unique_ptr<RHIDevice>		m_pRHIDevice;
	std::unique_ptr<RHISwapChain>	m_pSwapChain;
	std::unique_ptr<RHIFence>		m_pFrameFence;
	uint64                          m_uCurrentFrame = 0;
	uint64							m_uFrameIndex = 0;
	uint64                          m_vFenceValues[NUM_BACK_FRAMES] = {0, 0, 0};

	RenderGraph m_RenderGraph;
	RGHandle    m_BackBufferRG;
};

} // namespace Evo
