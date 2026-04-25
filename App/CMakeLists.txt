add_executable(EvoApp
    Main.cpp
)

evo_set_compiler_flags(EvoApp)

target_link_libraries(EvoApp PRIVATE EvoEngine)

# Copy SDL3 DLL next to executable on Windows (if shared build)
if(WIN32 AND TARGET SDL3::SDL3-shared)
    add_custom_command(TARGET EvoApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3-shared>
            $<TARGET_FILE_DIR:EvoApp>
    )
endif()
