#pragma once

#include "RHI/RHI.h"
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

private:
	bool CreateTriangleResources();

	std::unique_ptr<RHIDevice>		m_pRHIDevice;
	std::unique_ptr<RHISwapChain>	m_pSwapChain;
	std::unique_ptr<RHIFence>		m_pFrameFence;
	uint64                          m_uCurrentFrame = 0;
	uint64							m_uFrameIndex = 0;// current back buffer index
	std::unique_ptr<RHICommandList> m_vCmdLists[NUM_BACK_FRAMES];
	uint64                          m_vFenceValues[NUM_BACK_FRAMES] = {0, 0, 0};

	// Hello triangle resources
	RHIBufferHandle   m_TriangleVB;
	RHIShaderHandle   m_TriangleVS;
	RHIShaderHandle   m_TrianglePS;
	RHIPipelineHandle m_TrianglePipeline;
};

} // namespace Evo
