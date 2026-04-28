#pragma once

#include "RHI/RHI.h"
#include "Renderer/RenderGraph.h"
#include "Platform/Window.h"
#include <memory>

namespace Evo {

struct RendererDesc {
	RHIBackendType backend = RHIBackendType::DX12;
	bool enableDebug       = true;
};

class Renderer {
public:
	Renderer() = default;
	~Renderer();

	bool Initialize(const RendererDesc& desc, Window& window);
	void Shutdown();

	void BeginFrame();
	void EndFrame();

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
	std::unique_ptr<RHICommandList> m_vCmdLists[NUM_BACK_FRAMES];
	uint64                          m_vFenceValues[NUM_BACK_FRAMES] = {0, 0, 0};

	RenderGraph m_RenderGraph;
	RGHandle    m_BackBufferRG;
};

} // namespace Evo
