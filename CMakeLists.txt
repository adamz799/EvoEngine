cmake_minimum_required(VERSION 3.24)
project(EvoEngine VERSION 0.1.0 LANGUAGES CXX C)

# --- Enable solution folders in VS ---
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- RHI backend options ---
option(EVO_RHI_DX12   "Enable Direct3D 12 backend" ON)
option(EVO_RHI_VULKAN "Enable Vulkan backend"       OFF)

if(NOT EVO_RHI_DX12 AND NOT EVO_RHI_VULKAN)
	message(FATAL_ERROR "At least one RHI backend must be enabled (EVO_RHI_DX12 or EVO_RHI_VULKAN)")
endif()

# --- Include helper ---
include(cmake/EvoEngine.cmake)

# --- Subdirectories ---
add_subdirectory(ThirdParty)
add_subdirectory(Schemas)
add_subdirectory(Engine)
add_subdirectory(App)
add_subdirectory(Editor)



