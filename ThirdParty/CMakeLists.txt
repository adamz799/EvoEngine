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
