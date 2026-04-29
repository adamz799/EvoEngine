#include "Renderer/SceneRenderer.h"
#include "Renderer/Renderer.h"
#include "Scene/Scene.h"
#include "Scene/MeshAsset.h"
#include "Core/Log.h"

namespace Evo {

void SceneRenderer::RenderScene(Scene& scene, Renderer& renderer,
                                RHIPipelineHandle defaultPipeline,
                                const Mat4& viewProj,
                                RGHandle targetTexture,
                                RHIRenderTargetView targetRTV,
                                float fViewportWidth, float fViewportHeight)
{
	m_vDrawItems.clear();
	CollectDrawItems(scene, defaultPipeline);
	AddOpaquePass(renderer, viewProj, targetTexture, targetRTV, fViewportWidth, fViewportHeight);
}

void SceneRenderer::CollectDrawItems(Scene& scene, RHIPipelineHandle defaultPipeline)
{
	// Iterate entities that have both Transform and Mesh components
	scene.Meshes().ForEach([&](EntityHandle entity, MeshComponent& mesh) {
		if (!mesh.bVisible) return;
		auto* pTransform = scene.Transforms().Get(entity);
		if (!pTransform || !mesh.pMesh)
			return;

		const MeshLOD& lod = mesh.pMesh->GetLOD(mesh.uLODIndex);
		Mat4 worldMatrix = pTransform->GetWorldMatrix();

		for (const auto& subMesh : lod.vSubMeshes)
		{
			DrawItem item;
			item.pipeline      = defaultPipeline;
			item.vertexBuffer  = lod.vertexBuffer;
			item.indexBuffer   = lod.indexBuffer;
			item.uVertexStride = lod.uVertexStride;
			item.uIndexOffset  = subMesh.uIndexOffset;
			item.uIndexCount   = subMesh.uIndexCount;
			item.uVertexOffset = subMesh.uVertexOffset;
			item.worldMatrix   = worldMatrix;
			m_vDrawItems.push_back(item);
		}
	});
}

void SceneRenderer::AddOpaquePass(Renderer& renderer, const Mat4& viewProj,
                                  RGHandle targetTexture, RHIRenderTargetView targetRTV,
                                  float fViewportWidth, float fViewportHeight)
{
	auto& graph = renderer.GetRenderGraph();

	float w = fViewportWidth;
	float h = fViewportHeight;
	RHIColor clearColor = { 0.2f, 0.3f, 0.4f, 1.0f };

	// Capture draw items + viewProj by value for the execute lambda
	auto drawItems = m_vDrawItems;
	Mat4 vp = viewProj;

	graph.AddPass("OpaquePass",
		[targetTexture, targetRTV, clearColor](RGPassBuilder& builder) {
			builder.WriteRenderTarget(targetTexture, targetRTV, &clearColor);
		},
		[drawItems, vp, w, h](RHICommandList* pCmdList) {
			RHIViewport viewport = { 0, 0, w, h, 0, 1 };
			pCmdList->SetViewport(viewport);
			RHIScissorRect scissor = { 0, 0, static_cast<int32>(w), static_cast<int32>(h) };
			pCmdList->SetScissorRect(scissor);

			for (const auto& item : drawItems)
			{
				pCmdList->SetPipeline(item.pipeline);

				// Per-object MVP via push constants
				Mat4 mvp = item.worldMatrix * vp;
				pCmdList->SetPushConstants(&mvp, sizeof(Mat4));

				RHIVertexBufferView vbView = {};
				vbView.buffer  = item.vertexBuffer;
				vbView.uStride = item.uVertexStride;

				RHIIndexBufferView ibView = {};
				ibView.buffer = item.indexBuffer;
				ibView.format = RHIIndexFormat::U32;

				pCmdList->SetVertexBuffer(0, vbView);
				pCmdList->SetIndexBuffer(ibView);

				pCmdList->DrawIndexed(item.uIndexCount, 1,
				                      item.uIndexOffset, item.uVertexOffset, 0);
			}
		});
}

} // namespace Evo
