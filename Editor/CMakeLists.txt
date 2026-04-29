add_executable(EvoEditor
    EditorMain.cpp
    Editor.cpp
    ${CMAKE_SOURCE_DIR}/App/CubeDemo.cpp
)

evo_set_compiler_flags(EvoEditor)

target_include_directories(EvoEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}    # Editor.h, ImGuiLogSink.h
    ${CMAKE_SOURCE_DIR}/App        # CubeDemo.h
)

target_link_libraries(EvoEditor PRIVATE EvoEngine)

# Pass Agility SDK version number
if(EVO_RHI_DX12 AND DEFINED EVO_D3D12_AGILITY_SDK_VERSION)
    target_compile_definitions(EvoEditor PRIVATE
        EVO_D3D12_AGILITY_SDK_VERSION=${EVO_D3D12_AGILITY_SDK_VERSION}
    )
endif()

# Set VS debugger working directory to project root (Assets/ lives there)
set_target_properties(EvoEditor PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# Copy Assets to output directory for non-IDE launches
add_custom_command(TARGET EvoEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Assets"
        "$<TARGET_FILE_DIR:EvoEditor>/Assets"
    COMMENT "Copying Assets to output directory"
)

# Copy SDL3 DLL next to executable on Windows (if shared build)
if(WIN32 AND TARGET SDL3::SDL3-shared)
    add_custom_command(TARGET EvoEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3-shared>
            $<TARGET_FILE_DIR:EvoEditor>
    )
endif()

# Copy Agility SDK DLLs next to executable
if(EVO_RHI_DX12 AND TARGET CopyAgilitySDK)
    set(AGILITY_BIN_SRC ${CMAKE_SOURCE_DIR}/ThirdParty/AgilitySDK/build/native/bin/x64)
    add_custom_command(TARGET EvoEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:EvoEditor>/D3D12"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${AGILITY_BIN_SRC}/D3D12Core.dll"
            "${AGILITY_BIN_SRC}/d3d12SDKLayers.dll"
            "$<TARGET_FILE_DIR:EvoEditor>/D3D12"
        COMMENT "Copying Agility SDK DLLs next to EvoEditor"
    )
endif()
