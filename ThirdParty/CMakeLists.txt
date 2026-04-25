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
