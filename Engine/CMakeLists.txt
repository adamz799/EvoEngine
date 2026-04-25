# ---- Generate RHIConfig.h from compile options ----
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/RHIConfig.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/Generated/RHIConfig.h
)

# ---- Collect source files ----
set(CORE_SOURCES
    Core/Log.cpp
)

set(PLATFORM_SOURCES
    Platform/Window.cpp
    Platform/FileSystem.cpp
)

set(RHI_SOURCES
    RHI/RHI.cpp
)

set(RENDERER_SOURCES
    Renderer/Renderer.cpp
)

# Conditionally add backend sources
if(EVO_RHI_DX12)
    list(APPEND RHI_SOURCES
        RHI/DX12/DX12Device.cpp
    )
endif()

if(EVO_RHI_VULKAN)
    list(APPEND RHI_SOURCES
        RHI/Vulkan/VulkanDevice.cpp
    )
endif()

# ---- Create static library ----
add_library(EvoEngine STATIC
    ${CORE_SOURCES}
    ${PLATFORM_SOURCES}
    ${RHI_SOURCES}
    ${RENDERER_SOURCES}
)

evo_set_compiler_flags(EvoEngine)

# ---- Include directories ----
target_include_directories(EvoEngine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}        # Engine/ root, so includes are "Core/Log.h"
        ${CMAKE_CURRENT_BINARY_DIR}/Generated  # For RHIConfig.h
)

# ---- Link third-party ----
target_link_libraries(EvoEngine
    PUBLIC
        SDL3::SDL3-static
        spdlog::spdlog
        DirectXMath
)

# ---- Platform / backend specific linking ----
if(EVO_RHI_DX12)
    target_compile_definitions(EvoEngine PUBLIC EVO_RHI_DX12=1)
    target_link_libraries(EvoEngine
        PUBLIC
            d3d12
            dxgi
            dxguid
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
