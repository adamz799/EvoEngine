#include "Editor.h"
#include "Core/Log.h"
#include "Platform/Window.h"
#include "RHI/RHICommandList.h"
#include "Scene/Components.h"
#include "Math/Math.h"

#include <imgui.h>
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

void Editor::CreateViewportResources(uint32 uWidth, uint32 uHeight)
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

	// Depth buffer for viewport
	RHITextureDesc depthDesc = {};
	depthDesc.uWidth     = uWidth;
	depthDesc.uHeight    = uHeight;
	depthDesc.format     = RHIFormat::D32_FLOAT;
	depthDesc.usage      = RHITextureUsage::DepthStencil | RHITextureUsage::ShaderResource;
	depthDesc.sDebugName = "EditorViewportDepth";

	m_DepthTexture = m_pDevice->CreateTexture(depthDesc);
	m_DepthDSV     = m_pDevice->CreateDepthStencilView(m_DepthTexture);

	// G-Buffer textures (same size as viewport)
	RHITextureDesc gbAlbedoDesc = {};
	gbAlbedoDesc.uWidth     = uWidth;
	gbAlbedoDesc.uHeight    = uHeight;
	gbAlbedoDesc.format     = RHIFormat::R8G8B8A8_UNORM;
	gbAlbedoDesc.usage      = RHITextureUsage::RenderTarget | RHITextureUsage::ShaderResource;
	gbAlbedoDesc.sDebugName = "EditorGBuffer_Albedo";
	gbAlbedoDesc.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_GBAlbedoTexture = m_pDevice->CreateTexture(gbAlbedoDesc);
	m_GBAlbedoRTV     = m_pDevice->CreateRenderTargetView(m_GBAlbedoTexture);

	RHITextureDesc gbNormalDesc = {};
	gbNormalDesc.uWidth     = uWidth;
	gbNormalDesc.uHeight    = uHeight;
	gbNormalDesc.format     = RHIFormat::R16G16B16A16_FLOAT;
	gbNormalDesc.usage      = RHITextureUsage::RenderTarget | RHITextureUsage::ShaderResource;
	gbNormalDesc.sDebugName = "EditorGBuffer_Normal";
	gbNormalDesc.clearColor = { 0.5f, 0.5f, 1.0f, 0.0f };
	m_GBNormalTexture = m_pDevice->CreateTexture(gbNormalDesc);
	m_GBNormalRTV     = m_pDevice->CreateRenderTargetView(m_GBNormalTexture);

	RHITextureDesc gbRoughMetDesc = {};
	gbRoughMetDesc.uWidth     = uWidth;
	gbRoughMetDesc.uHeight    = uHeight;
	gbRoughMetDesc.format     = RHIFormat::R8G8B8A8_UNORM;
	gbRoughMetDesc.usage      = RHITextureUsage::RenderTarget | RHITextureUsage::ShaderResource;
	gbRoughMetDesc.sDebugName = "EditorGBuffer_RoughMet";
	gbRoughMetDesc.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_GBRoughMetTexture = m_pDevice->CreateTexture(gbRoughMetDesc);
	m_GBRoughMetRTV     = m_pDevice->CreateRenderTargetView(m_GBRoughMetTexture);

	// HDR intermediate (same size as viewport)
	RHITextureDesc hdrDesc = {};
	hdrDesc.uWidth     = uWidth;
	hdrDesc.uHeight    = uHeight;
	hdrDesc.format     = RHIFormat::R16G16B16A16_FLOAT;
	hdrDesc.usage      = RHITextureUsage::RenderTarget | RHITextureUsage::ShaderResource;
	hdrDesc.sDebugName = "EditorHDRIntermediate";
	hdrDesc.clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_HDRTexture = m_pDevice->CreateTexture(hdrDesc);
	m_HDRRTV     = m_pDevice->CreateRenderTargetView(m_HDRTexture);

	m_uViewportWidth  = uWidth;
	m_uViewportHeight = uHeight;
#endif
}

void Editor::DestroyViewportResources()
{
#if EVO_RHI_DX12
	// Destroy HDR intermediate
	if (m_HDRTexture.IsValid())
	{
		m_pDevice->DestroyRenderTargetView(m_HDRRTV);
		m_HDRRTV = {};
		m_pDevice->DestroyTexture(m_HDRTexture);
		m_HDRTexture = {};
	}
	// Destroy G-Buffer textures
	if (m_GBRoughMetTexture.IsValid())
	{
		m_pDevice->DestroyRenderTargetView(m_GBRoughMetRTV);
		m_GBRoughMetRTV = {};
		m_pDevice->DestroyTexture(m_GBRoughMetTexture);
		m_GBRoughMetTexture = {};
	}
	if (m_GBNormalTexture.IsValid())
	{
		m_pDevice->DestroyRenderTargetView(m_GBNormalRTV);
		m_GBNormalRTV = {};
		m_pDevice->DestroyTexture(m_GBNormalTexture);
		m_GBNormalTexture = {};
	}
	if (m_GBAlbedoTexture.IsValid())
	{
		m_pDevice->DestroyRenderTargetView(m_GBAlbedoRTV);
		m_GBAlbedoRTV = {};
		m_pDevice->DestroyTexture(m_GBAlbedoTexture);
		m_GBAlbedoTexture = {};
	}

	if (m_DepthTexture.IsValid())
	{
		m_pDevice->DestroyDepthStencilView(m_DepthDSV);
		m_DepthDSV = {};

		m_pDevice->DestroyTexture(m_DepthTexture);
		m_DepthTexture = {};
	}

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

bool Editor::Initialize(RHIDevice* pDevice, Window& window, RHIFormat rtFormat)
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
	CreateViewportResources(window.GetWidth(), window.GetHeight());

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
	DestroyViewportResources();
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
	ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

	// Main menu bar
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("View"))
		{
			ImGui::MenuItem("Hierarchy", nullptr, &m_bShowHierarchy);
			ImGui::MenuItem("Inspector", nullptr, &m_bShowInspector);
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
			DestroyViewportResources();
			CreateViewportResources(uNewW, uNewH);
		}

#if EVO_RHI_DX12
		if (m_ViewportSRV.IsValid())
		{
			ImGui::Image(static_cast<ImTextureID>(m_ViewportSRV.gpuHandle.ptr), vAvail);

			// Viewport picking — left-click on the image to select an entity
			if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				ImVec2 rectMin  = ImGui::GetItemRectMin();
				ImVec2 rectSize = ImGui::GetItemRectSize();
				ImVec2 mousePos = ImGui::GetMousePos();
				float u = (mousePos.x - rectMin.x) / rectSize.x;
				float v = (mousePos.y - rectMin.y) / rectSize.y;
				DoViewportPicking(scene, camera, u, v);
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
				ImGui::ColorEdit3("Albedo", &pMaterial->vAlbedoColor.x);
				ImGui::SliderFloat("Roughness", &pMaterial->fRoughness, 0.0f, 1.0f);
				ImGui::SliderFloat("Metallic", &pMaterial->fMetallic, 0.0f, 1.0f);
			}
		}
	}
	else
	{
		ImGui::TextDisabled("No entity selected");
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

} // namespace Evo
