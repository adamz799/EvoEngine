# ---- Generate RHIConfig.h from compile options ----
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/RHIConfig.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/Generated/RHIConfig.h
)

# ---- Collect source files by module ----
# We glob headers too so they appear in the VS solution tree.

file(GLOB CORE_HEADERS    Core/*.h)
set(CORE_SOURCES          Core/Log.cpp)

file(GLOB MATH_HEADERS    Math/*.h)

file(GLOB PLATFORM_HEADERS Platform/*.h)
set(PLATFORM_SOURCES       Platform/Window.cpp
                           Platform/FileSystem.cpp
                           Platform/Input.cpp)

file(GLOB ASSET_HEADERS    Asset/*.h)
set(ASSET_SOURCES          Asset/AssetLoader.cpp
                           Asset/AssetManager.cpp
                           Asset/ShaderAsset.cpp)

file(GLOB RHI_HEADERS     RHI/*.h)
set(RHI_SOURCES           RHI/RHI.cpp)

file(GLOB RENDERER_HEADERS Renderer/*.h)
set(RENDERER_SOURCES       Renderer/Renderer.cpp
                           Renderer/RenderGraph.cpp
                           Renderer/SceneRenderer.cpp
                           Renderer/Camera.cpp)

file(GLOB SCENE_HEADERS    Scene/*.h)
set(SCENE_SOURCES          Scene/Scene.cpp
                           Scene/MeshAsset.cpp
                           Scene/MeshWriter.cpp)

# ---- Backend sources ----
if(EVO_RHI_DX12)
    file(GLOB DX12_HEADERS  RHI/DX12/*.h)
    set(DX12_SOURCES
        RHI/DX12/DX12Device.cpp
        RHI/DX12/DX12Queue.cpp
        RHI/DX12/DX12Fence.cpp
        RHI/DX12/DX12SwapChain.cpp
        RHI/DX12/DX12CommandList.cpp
        RHI/DX12/DX12ShaderCompiler.cpp
        RHI/DX12/DX12CommandListPool.cpp
    )
endif()

if(EVO_RHI_VULKAN)
    file(GLOB VULKAN_HEADERS  RHI/Vulkan/*.h)
    set(VULKAN_SOURCES
        RHI/Vulkan/VulkanDevice.cpp
    )
endif()

# ---- Create static library ----
add_library(EvoEngine STATIC
    ${CORE_HEADERS}     ${CORE_SOURCES}
    ${MATH_HEADERS}
    ${PLATFORM_HEADERS} ${PLATFORM_SOURCES}
    ${ASSET_HEADERS}    ${ASSET_SOURCES}
    ${RHI_HEADERS}      ${RHI_SOURCES}
    ${RENDERER_HEADERS} ${RENDERER_SOURCES}
    ${SCENE_HEADERS}    ${SCENE_SOURCES}
    ${DX12_HEADERS}     ${DX12_SOURCES}
    ${VULKAN_HEADERS}   ${VULKAN_SOURCES}
)

evo_set_compiler_flags(EvoEngine)

# ---- VS solution folder tree ----
# Maps physical directory structure into VS filters so files aren't flat.
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES
    ${CORE_HEADERS}     ${CORE_SOURCES}
    ${MATH_HEADERS}
    ${PLATFORM_HEADERS} ${PLATFORM_SOURCES}
    ${ASSET_HEADERS}    ${ASSET_SOURCES}
    ${RHI_HEADERS}      ${RHI_SOURCES}
    ${RENDERER_HEADERS} ${RENDERER_SOURCES}
    ${SCENE_HEADERS}    ${SCENE_SOURCES}
    ${DX12_HEADERS}     ${DX12_SOURCES}
    ${VULKAN_HEADERS}   ${VULKAN_SOURCES}
)

# ---- Include directories ----
target_include_directories(EvoEngine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}            # Engine/ root: #include "Core/Log.h"
        ${CMAKE_CURRENT_BINARY_DIR}/Generated  # For RHIConfig.h
)

# ---- Link third-party ----
target_link_libraries(EvoEngine
    PUBLIC
        SDL3::SDL3-static
        spdlog::spdlog
        DirectXMath
        ImGui
        FlatBuffers
        EvoSchemas
)

# Ensure schema headers are generated before Engine compilation
add_dependencies(EvoEngine EvoSchemasGen)

# ---- Platform / backend specific linking ----
if(EVO_RHI_DX12)
    target_compile_definitions(EvoEngine PUBLIC EVO_RHI_DX12=1)
    target_link_libraries(EvoEngine
        PUBLIC
            AgilitySDK
            D3D12MemoryAllocator
            dxgi
            dxguid
            d3dcompiler
    )
endif()

if(EVO_RHI_VULKAN)
    find_package(Vulkan REQUIRED)
    target_compile_definitions(EvoEngine PUBLIC EVO_RHI_VULKAN=1)
    target_link_libraries(EvoEngine
        PUBLIC
            Vulkan::Vulkan
    )
endif()

if(WIN32)
    target_compile_definitions(EvoEngine PUBLIC EVO_PLATFORM_WINDOWS=1)
elseif(UNIX AND NOT APPLE)
    target_compile_definitions(EvoEngine PUBLIC EVO_PLATFORM_LINUX=1)
elseif(APPLE)
    target_compile_definitions(EvoEngine PUBLIC EVO_PLATFORM_MACOS=1)
endif()
