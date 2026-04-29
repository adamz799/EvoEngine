# ---- SDL3 ----
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/SDL/CMakeLists.txt)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON  CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory(SDL)
else()
    message(WARNING "SDL3 submodule not found at ThirdParty/SDL. Run: git submodule update --init")
endif()

# ---- spdlog ----
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/spdlog/CMakeLists.txt)
    set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    add_subdirectory(spdlog)
else()
    message(WARNING "spdlog submodule not found at ThirdParty/spdlog. Run: git submodule update --init")
endif()

# ---- DirectXMath (header-only) ----
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/DirectXMath/Inc/DirectXMath.h)
    add_library(DirectXMath INTERFACE)
    target_include_directories(DirectXMath INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/DirectXMath/Inc)
else()
    message(WARNING "DirectXMath submodule not found at ThirdParty/DirectXMath. Run: git submodule update --init")
endif()

# ---- D3D12 Agility SDK ----
# Provides the latest D3D12 headers and runtime, independent of Windows SDK version.
# Consumed as an INTERFACE library; include path takes priority over Windows SDK.
if(EVO_RHI_DX12)
    set(AGILITY_SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/AgilitySDK/build/native)
    if(EXISTS ${AGILITY_SDK_DIR}/include/d3d12.h)
        add_library(AgilitySDK INTERFACE)

        # BEFORE ensures Agility headers override Windows SDK's d3d12.h
        target_include_directories(AgilitySDK SYSTEM INTERFACE BEFORE
            ${AGILITY_SDK_DIR}/include
        )

        # d3d12.lib still comes from the Windows SDK (Agility SDK only ships the runtime DLLs)
        target_link_libraries(AgilitySDK INTERFACE d3d12)

        # SDK version (read from nuspec: 1.619.2 → SDK version 619)
        set(EVO_D3D12_AGILITY_SDK_VERSION 619 CACHE STRING "D3D12 Agility SDK version number" FORCE)

        # Copy runtime DLLs to <exe_output>/D3D12/ at build time
        set(AGILITY_BIN_DIR ${AGILITY_SDK_DIR}/bin/x64)
        set(AGILITY_DLL_DEST ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/D3D12)
        add_custom_target(CopyAgilitySDK ALL
            COMMAND ${CMAKE_COMMAND} -E make_directory "${AGILITY_DLL_DEST}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${AGILITY_BIN_DIR}/D3D12Core.dll"
                "${AGILITY_BIN_DIR}/d3d12SDKLayers.dll"
                "${AGILITY_DLL_DEST}"
            COMMENT "Copying D3D12 Agility SDK DLLs to output directory"
        )
    else()
        message(WARNING "Agility SDK not found at ThirdParty/AgilitySDK. "
            "Download from https://www.nuget.org/packages/Microsoft.Direct3D.D3D12")
    endif()
endif()

# ---- D3D12 Memory Allocator ----
if(EVO_RHI_DX12)
    set(D3D12MA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/D3D12MA)
    if(EXISTS ${D3D12MA_DIR}/include/D3D12MemAlloc.h)
        add_library(D3D12MemoryAllocator STATIC
            ${D3D12MA_DIR}/src/D3D12MemAlloc.cpp
            ${D3D12MA_DIR}/include/D3D12MemAlloc.h
        )
        target_include_directories(D3D12MemoryAllocator PUBLIC ${D3D12MA_DIR}/include)
        target_link_libraries(D3D12MemoryAllocator PUBLIC AgilitySDK)
    else()
        message(WARNING "D3D12MemoryAllocator not found at ThirdParty/D3D12MA")
    endif()
endif()

# ---- FlatBuffers (runtime headers + flatc compiler) ----
# Uses add_subdirectory so flatc can be built on-demand for schema generation.
# The FlatBuffers CMakeLists.txt creates a "FlatBuffers" INTERFACE target (line 728)
# providing the same include path as the old manual library.
set(FLATBUFFERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/flatbuffers)
if(EXISTS ${FLATBUFFERS_DIR}/CMakeLists.txt)
    set(FLATBUFFERS_BUILD_FLATC      ON  CACHE BOOL "" FORCE)
    set(FLATBUFFERS_BUILD_TESTS      OFF CACHE BOOL "" FORCE)
    set(FLATBUFFERS_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
    set(FLATBUFFERS_BUILD_FLATHASH   OFF CACHE BOOL "" FORCE)
    set(FLATBUFFERS_INSTALL          OFF CACHE BOOL "" FORCE)
    set(FLATBUFFERS_BUILD_FLATLIB    OFF CACHE BOOL "" FORCE)
    add_subdirectory(flatbuffers EXCLUDE_FROM_ALL)
else()
    message(WARNING "FlatBuffers not found at ThirdParty/flatbuffers")
endif()

# ---- Dear ImGui ----
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.h)
    set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
    add_library(ImGui STATIC
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        # Platform backend (SDL3)
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    )
    target_include_directories(ImGui
        PUBLIC
            ${IMGUI_DIR}
            ${IMGUI_DIR}/backends
    )
    target_link_libraries(ImGui PUBLIC SDL3::SDL3-static)

    # DX12 renderer backend
    if(EVO_RHI_DX12)
        target_sources(ImGui PRIVATE ${IMGUI_DIR}/backends/imgui_impl_dx12.cpp)
        target_link_libraries(ImGui PUBLIC AgilitySDK dxgi)
    endif()

    # Vulkan renderer backend
    if(EVO_RHI_VULKAN)
        find_package(Vulkan REQUIRED)
        target_sources(ImGui PRIVATE ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp)
        target_link_libraries(ImGui PUBLIC Vulkan::Vulkan)
    endif()
else()
    message(WARNING "imgui submodule not found at ThirdParty/imgui. Run: git submodule update --init")
endif()

# ---- Put all third-party targets into "ThirdParty" folder in VS solution ----
# Recursively set FOLDER for all targets defined under this directory
function(evo_set_folder_recursive dir folder)
    get_property(targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target IN LISTS targets)
        set_property(TARGET ${target} PROPERTY FOLDER ${folder})
    endforeach()
    get_property(subdirs DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
    foreach(subdir IN LISTS subdirs)
        evo_set_folder_recursive(${subdir} ${folder})
    endforeach()
endfunction()

evo_set_folder_recursive(${CMAKE_CURRENT_SOURCE_DIR} "ThirdParty")
