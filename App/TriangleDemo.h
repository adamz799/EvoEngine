#pragma once

#include "RHI/RHI.h"
#include "Renderer/RenderGraph.h"

namespace Evo {

class Renderer;

class TriangleDemo {
public:
	bool Initialize(Renderer& renderer);
	void Shutdown(Renderer& renderer);

	/// Add triangle rendering pass to the frame's render graph.
	void AddPasses(Renderer& renderer);

private:
	RHIBufferHandle   m_TriangleVB;
	RHIShaderHandle   m_TriangleVS;
	RHIShaderHandle   m_TrianglePS;
	RHIPipelineHandle m_TrianglePipeline;
};

} // namespace Evo

