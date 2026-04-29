#pragma once

#include "RHI/DX12/DX12Common.h"
#include <string>

namespace Evo {

/// Compile HLSL source from a memory buffer using D3DCompile.
/// sourceName is used for error messages only.
/// Returns a ComPtr to the compiled bytecode blob, or nullptr on failure.
ComPtr<ID3DBlob> CompileShaderFromSource(
	const void* pSource, size_t sourceSize,
	const char* sourceName,
	const char* entryPoint,
	const char* target);

/// Compile HLSL source from file using D3DCompile (d3dcompiler_47.dll).
/// Returns a ComPtr to the compiled bytecode blob, or nullptr on failure.
ComPtr<ID3DBlob> CompileShaderFromFile(
	const std::string& filePath,
	const char* entryPoint,
	const char* target);

} // namespace Evo
