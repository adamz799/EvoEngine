add_executable(EvoApp
	Main.cpp
	TestScene.cpp
)

evo_set_compiler_flags(EvoApp)

target_link_libraries(EvoApp PRIVATE EvoEngine)

# Pass Agility SDK version number to the application
if(EVO_RHI_DX12 AND DEFINED EVO_D3D12_AGILITY_SDK_VERSION)
	target_compile_definitions(EvoApp PRIVATE
		EVO_D3D12_AGILITY_SDK_VERSION=${EVO_D3D12_AGILITY_SDK_VERSION}
	)
endif()

# Set VS debugger working directory to project root (Assets/ lives there)
set_target_properties(EvoApp PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# Copy Assets to output directory for non-IDE launches
add_custom_command(TARGET EvoApp POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_SOURCE_DIR}/Assets"
		"$<TARGET_FILE_DIR:EvoApp>/Assets"
	COMMENT "Copying Assets to output directory"
)

# Copy SDL3 DLL next to executable on Windows (if shared build)
if(WIN32 AND TARGET SDL3::SDL3-shared)
	add_custom_command(TARGET EvoApp POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			$<TARGET_FILE:SDL3::SDL3-shared>
			$<TARGET_FILE_DIR:EvoApp>
	)
endif()

# Copy Agility SDK DLLs next to executable (handles multi-config generators like MSVC)
if(EVO_RHI_DX12 AND TARGET CopyAgilitySDK)
	set(AGILITY_BIN_SRC ${CMAKE_SOURCE_DIR}/ThirdParty/AgilitySDK/build/native/bin/x64)
	add_custom_command(TARGET EvoApp POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:EvoApp>/D3D12"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${AGILITY_BIN_SRC}/D3D12Core.dll"
			"${AGILITY_BIN_SRC}/d3d12SDKLayers.dll"
			"$<TARGET_FILE_DIR:EvoApp>/D3D12"
		COMMENT "Copying Agility SDK DLLs next to EvoApp"
	)
endif()


