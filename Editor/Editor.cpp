#include "Editor.h"
#include "Core/Log.h"
#include "Platform/Window.h"
#include "RHI/RHICommandList.h"
#include "Renderer/RenderPipeline.h"
#include "Scene/Components.h"
#include "Scene/MaterialWriter.h"
#include "Math/Math.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <cfloat>
#include <cmath>

#if EVO_RHI_DX12
#include <imgui_impl_sdl3.h>
#include <imgui_impl_dx12.h>
#include "RHI/DX12/DX12Device.h"
#include "RHI/DX12/DX12TypeMap.h"
#endif

namespace Evo {

// ---- ImGui DX12 descriptor callbacks ----
#if EVO_RHI_DX12

static void ImGuiSrvAllocCallback(ImGui_ImplDX12_InitInfo* info,
                                   D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu,
                                   D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu)
{
	auto* pAlloc = static_cast<DX12GpuDescriptorAllocator*>(info->UserData);
	auto alloc = pAlloc->Allocate();
	*out_cpu = alloc.cpuHandle;
	*out_gpu = alloc.gpuHandle;
}

static void ImGuiSrvFreeCallback(ImGui_ImplDX12_InitInfo* info,
                                  D3D12_CPU_DESCRIPTOR_HANDLE cpu,
                                  D3D12_GPU_DESCRIPTOR_HANDLE gpu)
{
	auto* pAlloc = static_cast<DX12GpuDescriptorAllocator*>(info->UserData);
	DX12GpuDescriptorAllocator::Allocation alloc;
	alloc.cpuHandle = cpu;
	alloc.gpuHandle = gpu;
	pAlloc->Free(alloc);
}

// Wrapper to match EventCallbackFn signature
static bool ImGuiProcessEvent(const SDL_Event& event)
{
	return ImGui_ImplSDL3_ProcessEvent(&event);
}

#endif // EVO_RHI_DX12

// ============================================================================
// Ray-AABB intersection (slab method)
// ============================================================================

static bool RayAABB(const Vec3& origin, const Vec3& dir,
                    const Vec3& aabbMin, const Vec3& aabbMax, float& tOut)
{
	float tMin = -FLT_MAX;
	float tMax =  FLT_MAX;

	for (int i = 0; i < 3; ++i)
	{
		if (std::abs(dir[i]) < 1e-8f)
		{
			if (origin[i] < aabbMin[i] || origin[i] > aabbMax[i])
				return false;
		}
		else
		{
			float invD = 1.0f / dir[i];
			float t1 = (aabbMin[i] - origin[i]) * invD;
			float t2 = (aabbMax[i] - origin[i]) * invD;
			if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
			if (t1 > tMin) tMin = t1;
			if (t2 < tMax) tMax = t2;
			if (tMin > tMax) return false;
		}
	}

	tOut = tMin >= 0.0f ? tMin : tMax;
	return tMax >= 0.0f;
}

// ============================================================================
// Viewport texture management
// ============================================================================

void Editor::CreateViewportTexture(uint32 uWidth, uint32 uHeight)
{
	if (uWidth == 0 || uHeight == 0)
		return;

#if EVO_RHI_DX12
	RHITextureDesc texDesc = {};
	texDesc.uWidth     = uWidth;
	texDesc.uHeight    = uHeight;
	texDesc.format     = m_RTFormat;
	texDesc.usage      = RHITextureUsage::RenderTarget | RHITextureUsage::ShaderResource;
	texDesc.clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	texDesc.sDebugName = "EditorViewport";

	m_ViewportTexture = m_pDevice->CreateTexture(texDesc);
	m_ViewportRTV     = m_pDevice->CreateRenderTargetView(m_ViewportTexture);

	auto* pDX12 = static_cast<DX12Device*>(m_pDevice);
	m_ViewportSRV = pDX12->CreateShaderResourceView(m_ViewportTexture);

	m_uViewportWidth  = uWidth;
	m_uViewportHeight = uHeight;
#endif
}

void Editor::DestroyViewportTexture()
{
#if EVO_RHI_DX12
	if (m_ViewportTexture.IsValid())
	{
		auto* pDX12 = static_cast<DX12Device*>(m_pDevice);
		pDX12->DestroyShaderResourceView(m_ViewportSRV);
		m_ViewportSRV = {};

		m_pDevice->DestroyRenderTargetView(m_ViewportRTV);
		m_ViewportRTV = {};

		m_pDevice->DestroyTexture(m_ViewportTexture);
		m_ViewportTexture = {};
	}
	m_uViewportWidth  = 0;
	m_uViewportHeight = 0;
#endif
}

// ============================================================================
// Editor style — UE5-inspired dark theme
// ============================================================================

static void SetEditorStyle()
{
	ImGuiStyle& style = ImGui::GetStyle();

	// Rounding & spacing
	style.WindowRounding    = 4.0f;
	style.ChildRounding     = 4.0f;
	style.FrameRounding     = 3.0f;
	style.PopupRounding     = 4.0f;
	style.ScrollbarRounding = 4.0f;
	style.GrabRounding      = 3.0f;
	style.TabRounding       = 4.0f;

	style.WindowPadding     = ImVec2(8, 8);
	style.FramePadding      = ImVec2(6, 4);
	style.ItemSpacing       = ImVec2(8, 4);
	style.ItemInnerSpacing  = ImVec2(4, 4);
	style.IndentSpacing     = 20.0f;

	style.ScrollbarSize     = 14.0f;
	style.GrabMinSize       = 12.0f;
	style.WindowBorderSize  = 1.0f;
	style.ChildBorderSize   = 1.0f;
	style.FrameBorderSize   = 0.0f;
	style.TabBorderSize     = 0.0f;

	ImVec4* c = style.Colors;

	// Background
	c[ImGuiCol_WindowBg]             = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
	c[ImGuiCol_ChildBg]              = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
	c[ImGuiCol_PopupBg]              = ImVec4(0.10f, 0.10f, 0.10f, 0.96f);

	// Border
	c[ImGuiCol_Border]               = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	c[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

	// Frame (input fields, checkboxes, etc.)
	c[ImGuiCol_FrameBg]              = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
	c[ImGuiCol_FrameBgHovered]       = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
	c[ImGuiCol_FrameBgActive]        = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);

	// Title bar
	c[ImGuiCol_TitleBg]              = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
	c[ImGuiCol_TitleBgActive]        = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	c[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.08f, 0.08f, 0.08f, 0.75f);

	// Menu bar
	c[ImGuiCol_MenuBarBg]            = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);

	// Scrollbar
	c[ImGuiCol_ScrollbarBg]          = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	c[ImGuiCol_ScrollbarGrab]        = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
	c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

	// Button
	c[ImGuiCol_Button]               = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	c[ImGuiCol_ButtonHovered]        = ImVec4(0.28f, 0.56f, 0.90f, 1.00f);
	c[ImGuiCol_ButtonActive]         = ImVec4(0.20f, 0.46f, 0.78f, 1.00f);

	// Header (collapsing header, selectable, menu item)
	c[ImGuiCol_Header]               = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	c[ImGuiCol_HeaderHovered]        = ImVec4(0.28f, 0.56f, 0.90f, 0.80f);
	c[ImGuiCol_HeaderActive]         = ImVec4(0.28f, 0.56f, 0.90f, 1.00f);

	// Separator
	c[ImGuiCol_Separator]            = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	c[ImGuiCol_SeparatorHovered]     = ImVec4(0.28f, 0.56f, 0.90f, 0.78f);
	c[ImGuiCol_SeparatorActive]      = ImVec4(0.28f, 0.56f, 0.90f, 1.00f);

	// Resize grip
	c[ImGuiCol_ResizeGrip]           = ImVec4(0.28f, 0.56f, 0.90f, 0.25f);
	c[ImGuiCol_ResizeGripHovered]    = ImVec4(0.28f, 0.56f, 0.90f, 0.67f);
	c[ImGuiCol_ResizeGripActive]     = ImVec4(0.28f, 0.56f, 0.90f, 0.95f);

	// Tab
	c[ImGuiCol_Tab]                  = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	c[ImGuiCol_TabHovered]           = ImVec4(0.28f, 0.56f, 0.90f, 0.80f);
	c[ImGuiCol_TabSelected]          = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
	c[ImGuiCol_TabDimmed]            = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	c[ImGuiCol_TabDimmedSelected]    = ImVec4(0.16f, 0.30f, 0.48f, 1.00f);

	// Docking
	c[ImGuiCol_DockingPreview]       = ImVec4(0.28f, 0.56f, 0.90f, 0.70f);
	c[ImGuiCol_DockingEmptyBg]       = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);

	// Text
	c[ImGuiCol_Text]                 = ImVec4(0.88f, 0.88f, 0.88f, 1.00f);
	c[ImGuiCol_TextDisabled]         = ImVec4(0.46f, 0.46f, 0.46f, 1.00f);
	c[ImGuiCol_TextSelectedBg]       = ImVec4(0.28f, 0.56f, 0.90f, 0.35f);

	// Misc
	c[ImGuiCol_CheckMark]            = ImVec4(0.28f, 0.56f, 0.90f, 1.00f);
	c[ImGuiCol_SliderGrab]           = ImVec4(0.28f, 0.56f, 0.90f, 1.00f);
	c[ImGuiCol_SliderGrabActive]     = ImVec4(0.37f, 0.62f, 0.95f, 1.00f);
	c[ImGuiCol_DragDropTarget]       = ImVec4(0.28f, 0.56f, 0.90f, 0.90f);
	c[ImGuiCol_NavHighlight]         = ImVec4(0.28f, 0.56f, 0.90f, 1.00f);
}

// ============================================================================
// Initialize / Shutdown
// ============================================================================

bool Editor::Initialize(RHIDevice* pDevice, Window& window, RHIFormat rtFormat,
                        const RenderPipeline& pipeline)
{
#if EVO_RHI_DX12
	m_pDevice  = pDevice;
	m_RTFormat = rtFormat;
	auto* pDX12 = static_cast<DX12Device*>(pDevice);

	// Register log sink for the ImGui log panel
	m_pLogSink = std::make_shared<ImGuiLogSink>();
	Log::AddSink(m_pLogSink);

	// Create ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	SetEditorStyle();

	// Load Microsoft YaHei UI font (Chinese support)
	ImFontConfig fontConfig;
	fontConfig.OversampleH = 2;
	fontConfig.PixelSnapH  = true;
	io.Fonts->AddFontFromFileTTF(
		"C:\\Windows\\Fonts\\msyh.ttc", 16.0f,
		&fontConfig, io.Fonts->GetGlyphRangesChineseFull());

	// Initialize SDL3 backend
	ImGui_ImplSDL3_InitForD3D(window.GetSDLWindow());

	// Initialize DX12 backend
	ImGui_ImplDX12_InitInfo initInfo = {};
	initInfo.Device             = pDX12->GetD3D12Device();
	initInfo.CommandQueue        = pDX12->GetGraphicsCommandQueue();
	initInfo.NumFramesInFlight   = NUM_BACK_FRAMES;
	initInfo.RTVFormat           = MapFormat(rtFormat);
	initInfo.SrvDescriptorHeap   = pDX12->GetSRVHeap();
	initInfo.SrvDescriptorAllocFn = ImGuiSrvAllocCallback;
	initInfo.SrvDescriptorFreeFn  = ImGuiSrvFreeCallback;
	initInfo.UserData             = &pDX12->GetSRVAllocator();
	ImGui_ImplDX12_Init(&initInfo);

	// Register event callback for ImGui input
	window.SetEventCallback(ImGuiProcessEvent);

	// Create initial viewport texture
	CreateViewportTexture(window.GetWidth(), window.GetHeight());

	// Create ViewportFrame for intermediate rendering resources
	m_ViewportFrame.Initialize(pDevice,
		pipeline.MakeViewportFrameDesc(window.GetWidth(), window.GetHeight(), "Editor"));

	EVO_LOG_INFO("Editor initialized");
	return true;
#else
	return false;
#endif
}

void Editor::Shutdown()
{
#if EVO_RHI_DX12
	m_pDevice->WaitIdle();
	m_ViewportFrame.Shutdown(m_pDevice);
	DestroyViewportTexture();
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
	EVO_LOG_INFO("Editor shut down");
#endif
}

// ============================================================================
// Per-frame
// ============================================================================

void Editor::BeginFrame()
{
#if EVO_RHI_DX12
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
#endif
}

void Editor::Update(Scene& scene, const Camera& camera, float /*fDeltaTime*/)
{
	// Create a DockSpace over the entire window
	ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

	// Build initial dock layout on first frame (only when no saved layout exists)
	if (m_bFirstFrame)
	{
		m_bFirstFrame = false;

		// Only build layout if dockspace has no existing saved nodes
		if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr ||
		    ImGui::DockBuilderGetNode(dockspaceId)->ChildNodes[0] == nullptr)
		{
			ImGui::DockBuilderRemoveNode(dockspaceId);
			ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->Size);

			// Split: left 18% = Hierarchy, rest = main area
			ImGuiID leftId, mainId;
			ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.18f, &leftId, &mainId);

			// Split main: right 22% = right panel, rest = center area
			ImGuiID rightId, centerId;
			ImGui::DockBuilderSplitNode(mainId, ImGuiDir_Right, 0.22f, &rightId, &centerId);

			// Split center: bottom 25% = Log, rest = viewport
			ImGuiID bottomId, viewportId;
			ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Down, 0.25f, &bottomId, &viewportId);

			// Dock windows into nodes
			ImGui::DockBuilderDockWindow("Hierarchy", leftId);
			ImGui::DockBuilderDockWindow("Scene Viewport", viewportId);
			ImGui::DockBuilderDockWindow("Inspector", rightId);
			ImGui::DockBuilderDockWindow("Material Editor", rightId);  // tabs with Inspector
			ImGui::DockBuilderDockWindow("Log", bottomId);

			ImGui::DockBuilderFinish(dockspaceId);
		}
	}

	// Main menu bar
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("View"))
		{
			ImGui::MenuItem("Hierarchy", nullptr, &m_bShowHierarchy);
			ImGui::MenuItem("Inspector", nullptr, &m_bShowInspector);
			ImGui::MenuItem("Material Editor", nullptr, &m_bShowMaterialEditor);
			ImGui::MenuItem("Log",       nullptr, &m_bShowLog);
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	// Hierarchy panel
	if (m_bShowHierarchy)
	{
		ImGui::Begin("Hierarchy", &m_bShowHierarchy);
		DrawHierarchyPanel(scene);
		ImGui::End();
	}

	// Scene Viewport panel
	{
		ImGui::Begin("Scene Viewport");

		ImVec2 vAvail = ImGui::GetContentRegionAvail();
		uint32 uNewW = static_cast<uint32>(vAvail.x > 1.0f ? vAvail.x : 1.0f);
		uint32 uNewH = static_cast<uint32>(vAvail.y > 1.0f ? vAvail.y : 1.0f);

		// Resize viewport texture if needed
		if (uNewW != m_uViewportWidth || uNewH != m_uViewportHeight)
		{
			m_pDevice->WaitIdle();
			DestroyViewportTexture();
			CreateViewportTexture(uNewW, uNewH);
			m_ViewportFrame.Resize(m_pDevice, uNewW, uNewH);
		}

#if EVO_RHI_DX12
		if (m_ViewportSRV.IsValid())
		{
			ImGui::Image(static_cast<ImTextureID>(m_ViewportSRV.gpuHandle.ptr), vAvail);

			bool bHovered = ImGui::IsItemHovered();
			ImVec2 rectMin  = ImGui::GetItemRectMin();
			ImVec2 rectSize = ImGui::GetItemRectSize();
			ImVec2 mousePos = ImGui::GetMousePos();
			float u = (rectSize.x > 0) ? (mousePos.x - rectMin.x) / rectSize.x : 0;
			float v = (rectSize.y > 0) ? (mousePos.y - rectMin.y) / rectSize.y : 0;

			// Gizmo hover detection (every frame when entity selected)
			m_iHoveredAxis = -1;
			if (bHovered && m_SelectedEntity.IsValid() && scene.IsAlive(m_SelectedEntity) && !m_bDraggingGizmo)
			{
				auto* pTransform = scene.Transforms().Get(m_SelectedEntity);
				if (pTransform)
				{
					Vec3 rayOrigin, rayDir;
					ComputeViewportRay(camera, u, v, rayOrigin, rayDir);
					m_iHoveredAxis = TestGizmoAxisHit(rayOrigin, rayDir,
						pTransform->vPosition, 2.0f, 0.15f);
				}
			}

			// Left click: start gizmo drag or do picking
			if (bHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				if (m_iHoveredAxis >= 0)
				{
					// Start gizmo drag
					auto* pTransform = scene.Transforms().Get(m_SelectedEntity);
					if (pTransform)
					{
						m_bDraggingGizmo = true;
						m_iDragAxis = m_iHoveredAxis;
						m_vDragOrigin = pTransform->vPosition;

						Vec3 rayOrigin, rayDir;
						ComputeViewportRay(camera, u, v, rayOrigin, rayDir);

						// Compute initial closest point on axis
						const Vec3 axes[3] = { Vec3(1,0,0), Vec3(0,1,0), Vec3(0,0,1) };
						Vec3 axisDir = axes[m_iDragAxis];
						Vec3 w0 = rayOrigin - m_vDragOrigin;
						float a = Vec3::Dot(axisDir, axisDir);
						float b = Vec3::Dot(axisDir, rayDir);
						float c = Vec3::Dot(rayDir, rayDir);
						float d = Vec3::Dot(axisDir, w0);
						float e = Vec3::Dot(rayDir, w0);
						float denom = a * c - b * b;
						float t = (denom > 1e-6f) ? (b * e - c * d) / denom : 0.0f;
						m_vDragRayHit = m_vDragOrigin + axisDir * t;
					}
				}
				else
				{
					DoViewportPicking(scene, camera, u, v);
				}
			}

			// Gizmo drag update
			if (m_bDraggingGizmo && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
			{
				auto* pTransform = scene.Transforms().Get(m_SelectedEntity);
				if (pTransform)
				{
					Vec3 rayOrigin, rayDir;
					ComputeViewportRay(camera, u, v, rayOrigin, rayDir);

					const Vec3 axes[3] = { Vec3(1,0,0), Vec3(0,1,0), Vec3(0,0,1) };
					Vec3 axisDir = axes[m_iDragAxis];
					Vec3 w0 = rayOrigin - m_vDragOrigin;
					float a = Vec3::Dot(axisDir, axisDir);
					float b = Vec3::Dot(axisDir, rayDir);
					float c = Vec3::Dot(rayDir, rayDir);
					float d = Vec3::Dot(axisDir, w0);
					float e = Vec3::Dot(rayDir, w0);
					float denom = a * c - b * b;
					float t = (denom > 1e-6f) ? (b * e - c * d) / denom : 0.0f;
					Vec3 currentHit = m_vDragOrigin + axisDir * t;
					Vec3 delta = currentHit - m_vDragRayHit;
					pTransform->vPosition = m_vDragOrigin + delta;
				}
			}

			// End drag
			if (m_bDraggingGizmo && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			{
				m_bDraggingGizmo = false;
				m_iDragAxis = -1;
			}
		}
#endif

		ImGui::End();
	}

	// Inspector panel
	if (m_bShowInspector)
	{
		ImGui::Begin("Inspector", &m_bShowInspector);
		DrawInspectorPanel(scene);
		ImGui::End();
	}

	// Material Editor panel
	if (m_bShowMaterialEditor)
	{
		ImGui::Begin("Material Editor", &m_bShowMaterialEditor);
		DrawMaterialEditorPanel(scene);
		ImGui::End();
	}

	// Log panel
	if (m_bShowLog)
	{
		ImGui::Begin("Log", &m_bShowLog);
		DrawLogPanel();
		ImGui::End();
	}
}

void Editor::Render(RHICommandList* pCmdList)
{
#if EVO_RHI_DX12
	ImGui::Render();

	auto* pDX12Device = static_cast<DX12Device*>(m_pDevice);
	ID3D12DescriptorHeap* pSrvHeap = pDX12Device->GetSRVHeap();
	void* heapPtr = pSrvHeap;
	pCmdList->SetDescriptorHeaps(1, &heapPtr);

	auto* pDX12CmdList = static_cast<DX12CommandList*>(pCmdList);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pDX12CmdList->GetD3D12CommandList());
#endif
}

// ============================================================================
// Panels
// ============================================================================

void Editor::DrawHierarchyPanel(Scene& scene)
{
	ImGuiTreeNodeFlags sceneFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow;
	if (ImGui::TreeNodeEx("Scene", sceneFlags))
	{
		scene.ForEachEntity([&](EntityHandle entity) {
			const auto& sName = scene.GetEntityName(entity);
			const char* pLabel = sName.empty() ? "<unnamed>" : sName.c_str();

			ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Leaf
				| ImGuiTreeNodeFlags_NoTreePushOnOpen
				| ImGuiTreeNodeFlags_SpanAvailWidth;
			if (entity == m_SelectedEntity)
				nodeFlags |= ImGuiTreeNodeFlags_Selected;

			ImGui::TreeNodeEx(pLabel, nodeFlags);
			if (ImGui::IsItemClicked())
				m_SelectedEntity = entity;
		});
		ImGui::TreePop();
	}
}

void Editor::DrawInspectorPanel(Scene& scene)
{
	if (scene.IsAlive(m_SelectedEntity))
	{
		// Entity name
		const auto& sName = scene.GetEntityName(m_SelectedEntity);
		ImGui::Text("Entity: %s", sName.empty() ? "<unnamed>" : sName.c_str());
		ImGui::Separator();

		// Transform
		auto* pTransform = scene.Transforms().Get(m_SelectedEntity);
		if (pTransform)
		{
			if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::DragFloat3("Position", &pTransform->vPosition.x, 0.1f);

				Vec3 euler = pTransform->qRotation.ToEuler();
				Vec3 eulerDeg(RadToDeg(euler.x), RadToDeg(euler.y), RadToDeg(euler.z));
				if (ImGui::DragFloat3("Rotation", &eulerDeg.x, 0.5f))
					pTransform->qRotation = Quat::FromEuler(
						DegToRad(eulerDeg.x), DegToRad(eulerDeg.y), DegToRad(eulerDeg.z));

				ImGui::DragFloat3("Scale", &pTransform->vScale.x, 0.1f);
			}
		}

		// Prefab
		const auto& sPrefab = scene.GetEntityPrefab(m_SelectedEntity);
		if (!sPrefab.empty())
		{
			if (ImGui::CollapsingHeader("Prefab", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Text("Path: %s", sPrefab.c_str());
			}
		}

		// Mesh
		auto* pMesh = scene.Meshes().Get(m_SelectedEntity);
		if (pMesh)
		{
			if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Checkbox("Visible", &pMesh->bVisible);
				ImGui::Text("LOD: %u", pMesh->uLODIndex);
			}
		}

		// Material
		auto* pMaterial = scene.Materials().Get(m_SelectedEntity);
		if (pMaterial)
		{
			if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
			{
				const auto& sMatPath = scene.GetEntityMaterial(m_SelectedEntity);
				if (!sMatPath.empty())
					ImGui::Text("Path: %s", sMatPath.c_str());
				ImGui::ColorEdit3("Albedo", &pMaterial->vAlbedoColor.x);
				ImGui::SliderFloat("Roughness", &pMaterial->fRoughness, 0.0f, 1.0f);
				ImGui::SliderFloat("Metallic", &pMaterial->fMetallic, 0.0f, 1.0f);
				ImGui::SliderFloat("Alpha", &pMaterial->fAlpha, 0.0f, 1.0f);
			}
		}

		// Camera
		auto* pCamera = scene.Cameras().Get(m_SelectedEntity);
		if (pCamera)
		{
			if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
			{
				float fovDeg = RadToDeg(pCamera->fFovY);
				if (ImGui::DragFloat("FOV", &fovDeg, 0.5f, 1.0f, 179.0f))
					pCamera->fFovY = DegToRad(fovDeg);
				ImGui::DragFloat("Near", &pCamera->fNearZ, 0.01f, 0.001f, 100.0f);
				ImGui::DragFloat("Far", &pCamera->fFarZ, 1.0f, 1.0f, 10000.0f);
			}
		}
	}
	else
	{
		ImGui::TextDisabled("No entity selected");
	}
}

void Editor::DrawMaterialEditorPanel(Scene& scene)
{
	if (!m_SelectedEntity.IsValid() || !scene.IsAlive(m_SelectedEntity))
	{
		ImGui::TextDisabled("Select an entity to edit material");
		return;
	}

	auto* pMaterial = scene.Materials().Get(m_SelectedEntity);
	if (!pMaterial)
	{
		ImGui::TextDisabled("Selected entity has no material");
		if (ImGui::Button("Add Material"))
			scene.Materials().Add(m_SelectedEntity);
		return;
	}

	ImGui::Text("Entity: %s", scene.GetEntityName(m_SelectedEntity).c_str());
	const auto& sMatPath = scene.GetEntityMaterial(m_SelectedEntity);
	if (!sMatPath.empty())
		ImGui::Text("File: %s", sMatPath.c_str());
	else
		ImGui::TextDisabled("(no file assigned)");
	ImGui::Separator();

	ImGui::ColorEdit3("Albedo Color", &pMaterial->vAlbedoColor.x);
	ImGui::SliderFloat("Roughness", &pMaterial->fRoughness, 0.0f, 1.0f);
	ImGui::SliderFloat("Metallic",  &pMaterial->fMetallic,  0.0f, 1.0f);
	ImGui::SliderFloat("Alpha",     &pMaterial->fAlpha,     0.0f, 1.0f);

	ImGui::Separator();
	if (ImGui::Button("Reset to Defaults"))
		*pMaterial = MaterialComponent{};

	if (!sMatPath.empty())
	{
		ImGui::SameLine();
		if (ImGui::Button("Save Material"))
		{
			if (WriteMaterial(sMatPath, pMaterial->vAlbedoColor,
			                  pMaterial->fRoughness, pMaterial->fMetallic, pMaterial->fAlpha))
			{
				EVO_LOG_INFO("Material saved to '{}'", sMatPath);
			}
		}
	}
}

void Editor::DrawLogPanel()
{
	if (ImGui::Button("Clear"))
	{
		if (m_pLogSink) m_pLogSink->Clear();
	}

	ImGui::SameLine();
	ImGui::Separator();

	ImGui::BeginChild("LogScroll", ImVec2(0, 0), ImGuiChildFlags_None,
	                   ImGuiWindowFlags_HorizontalScrollbar);

	if (m_pLogSink)
	{
		for (const auto& entry : m_pLogSink->GetEntries())
		{
			ImVec4 color;
			switch (entry.level)
			{
				case spdlog::level::err:
				case spdlog::level::critical:
					color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
					break;
				case spdlog::level::warn:
					color = ImVec4(1.0f, 0.8f, 0.3f, 1.0f);
					break;
				case spdlog::level::debug:
				case spdlog::level::trace:
					color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
					break;
				default:
					color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
					break;
			}

			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::TextUnformatted(entry.sMessage.c_str());
			ImGui::PopStyleColor();
		}

		// Auto-scroll to bottom
		if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			ImGui::SetScrollHereY(1.0f);
	}

	ImGui::EndChild();
}

// ============================================================================
// Viewport picking
// ============================================================================

void Editor::DoViewportPicking(Scene& scene, const Camera& camera, float u, float v)
{
	// Convert UV [0,1] to NDC — DX convention: Z in [0,1], Y up
	float ndcX = u * 2.0f - 1.0f;
	float ndcY = (1.0f - v) * 2.0f - 1.0f;

	// Unproject near/far points to world space
	Mat4 invVP = camera.GetViewProjectionMatrix().Inverse();
	Vec3 nearWS = invVP.TransformPoint(Vec3(ndcX, ndcY, 0.0f));
	Vec3 farWS  = invVP.TransformPoint(Vec3(ndcX, ndcY, 1.0f));

	Vec3 rayOrigin = nearWS;
	Vec3 rayDir    = (farWS - nearWS).Normalized();

	// Test each entity's OBB (unit cube in object space)
	EntityHandle hitEntity;
	float fMinDist = FLT_MAX;

	scene.ForEachEntity([&](EntityHandle entity) {
		auto* pTransform = scene.Transforms().Get(entity);
		if (!pTransform) return;

		Mat4 invModel = pTransform->GetWorldMatrix().Inverse();
		Vec3 roOS = invModel.TransformPoint(rayOrigin);
		Vec3 rdOS = invModel.TransformDirection(rayDir);

		float tHit;
		if (RayAABB(roOS, rdOS, Vec3(-0.5f), Vec3(0.5f), tHit) && tHit >= 0.0f)
		{
			// Convert hit to world space for distance comparison
			Vec3 hitWS = pTransform->GetWorldMatrix().TransformPoint(roOS + rdOS * tHit);
			float fDist = Vec3::Distance(camera.GetPosition(), hitWS);
			if (fDist < fMinDist)
			{
				fMinDist  = fDist;
				hitEntity = entity;
			}
		}
	});

	m_SelectedEntity = hitEntity;
}

// ============================================================================
// Gizmo helpers
// ============================================================================

void Editor::ComputeViewportRay(const Camera& camera, float u, float v,
                                Vec3& outOrigin, Vec3& outDir) const
{
	float ndcX = u * 2.0f - 1.0f;
	float ndcY = (1.0f - v) * 2.0f - 1.0f;

	Mat4 invVP = camera.GetViewProjectionMatrix().Inverse();
	Vec3 nearWS = invVP.TransformPoint(Vec3(ndcX, ndcY, 0.0f));
	Vec3 farWS  = invVP.TransformPoint(Vec3(ndcX, ndcY, 1.0f));

	outOrigin = nearWS;
	outDir    = (farWS - nearWS).Normalized();
}

int Editor::TestGizmoAxisHit(const Vec3& rayOrigin, const Vec3& rayDir,
                             const Vec3& gizmoPos, float fSize, float fThreshold) const
{
	const Vec3 axes[3] = { Vec3(1,0,0), Vec3(0,1,0), Vec3(0,0,1) };
	int bestAxis = -1;
	float bestDist = fThreshold;

	for (int i = 0; i < 3; ++i)
	{
		Vec3 axisDir = axes[i];

		// Closest point between ray and axis segment
		// Axis line: P = gizmoPos + t * axisDir, t in [0, fSize]
		// Ray line:  Q = rayOrigin + s * rayDir
		Vec3 w0 = rayOrigin - gizmoPos;
		float a = Vec3::Dot(axisDir, axisDir);   // always 1.0
		float b = Vec3::Dot(axisDir, rayDir);
		float c = Vec3::Dot(rayDir, rayDir);      // always 1.0 if normalized
		float d = Vec3::Dot(axisDir, w0);
		float e = Vec3::Dot(rayDir, w0);
		float denom = a * c - b * b;

		float t, s;
		if (denom < 1e-6f)
		{
			// Lines are parallel
			t = 0.0f;
			s = e / c;
		}
		else
		{
			t = (b * e - c * d) / denom;
			s = (a * e - b * d) / denom;
		}

		// Clamp t to axis segment [0, fSize]
		t = Clamp(t, 0.0f, fSize);
		// s must be positive (in front of camera)
		if (s < 0.0f) continue;

		Vec3 closestOnAxis = gizmoPos + axisDir * t;
		Vec3 closestOnRay  = rayOrigin + rayDir * s;
		float dist = Vec3::Distance(closestOnAxis, closestOnRay);

		if (dist < bestDist)
		{
			bestDist = dist;
			bestAxis = i;
		}
	}

	return bestAxis;
}

} // namespace Evo
