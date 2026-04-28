#pragma once

#include "RHI/DX12/DX12Common.h"
#include <string>

namespace Evo {

/// Compile HLSL source from file using D3DCompile (d3dcompiler_47.dll).
/// Returns a ComPtr to the compiled bytecode blob, or nullptr on failure.
ComPtr<ID3DBlob> CompileShaderFromFile(
	const std::string& filePath,
	const char* entryPoint,
	const char* target);

} // namespace Evo
