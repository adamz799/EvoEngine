#pragma once

#include "RHI/RHITypes.h"
#include "Math/Mat4.h"
#include <vector>

namespace Evo {

class Scene;
class Renderer;

/// Per-draw-call data collected from the scene.
struct DrawItem {
	RHIPipelineHandle pipeline;
	RHIBufferHandle   vertexBuffer;
	RHIBufferHandle   indexBuffer;
	uint32            uVertexStride = 0;
	uint32            uIndexOffset  = 0;
	uint32            uIndexCount   = 0;
	uint32            uVertexOffset = 0;
	Mat4              worldMatrix;
};

/// SceneRenderer — bridges Scene data and RenderGraph.
/// Collects renderable entities, builds draw items, adds passes.
class SceneRenderer {
public:
	void RenderScene(Scene& scene, Renderer& renderer,
	                 RHIPipelineHandle defaultPipeline,
	                 const Mat4& viewProj);

private:
	std::vector<DrawItem> m_vDrawItems;

	void CollectDrawItems(Scene& scene, RHIPipelineHandle defaultPipeline);
	void AddOpaquePass(Renderer& renderer, const Mat4& viewProj);
};

} // namespace Evo
