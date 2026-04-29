#include "RHI/DX12/DX12ShaderCompiler.h"
#include "Core/Log.h"
#include <d3dcompiler.h>
#include <fstream>
#include <vector>

namespace Evo {

ComPtr<ID3DBlob> CompileShaderFromSource(
	const void* pSource, size_t sourceSize,
	const char* sourceName,
	const char* entryPoint,
	const char* target)
{
	UINT compileFlags = 0;
#if defined(_DEBUG)
	compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ComPtr<ID3DBlob> bytecode;
	ComPtr<ID3DBlob> errors;
	HRESULT hr = D3DCompile(
		pSource, sourceSize,
		sourceName,
		nullptr, nullptr,
		entryPoint, target,
		compileFlags, 0,
		&bytecode, &errors);

	if (FAILED(hr))
	{
		EVO_LOG_ERROR("CompileShader '{}' entry '{}' target '{}':\n{}",
			sourceName ? sourceName : "<memory>",
			entryPoint, target,
			errors ? static_cast<const char*>(errors->GetBufferPointer()) : "unknown error");
		return nullptr;
	}

	return bytecode;
}

ComPtr<ID3DBlob> CompileShaderFromFile(
	const std::string& filePath,
	const char* entryPoint,
	const char* target)
{
	// Read file into memory
	std::ifstream file(filePath, std::ios::binary | std::ios::ate);
	if (!file.is_open())
	{
		EVO_LOG_ERROR("CompileShader: failed to open '{}'", filePath);
		return nullptr;
	}

	auto fileSize = file.tellg();
	file.seekg(0);
	std::vector<char> source(static_cast<size_t>(fileSize));
	file.read(source.data(), fileSize);

	return CompileShaderFromSource(
		source.data(), source.size(),
		filePath.c_str(),
		entryPoint, target);
}

} // namespace Evo
