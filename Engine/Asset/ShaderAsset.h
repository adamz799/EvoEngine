#pragma once

#include "Asset/Asset.h"
#include "RHI/RHITypes.h"

namespace Evo {

/// Shader asset — loads an HLSL file, compiles detected stages, creates RHI shaders.
///
/// Entry point convention:
///   VSMain → vertex shader (vs_5_1)
///   PSMain → pixel shader  (ps_5_1)
///   CSMain → compute shader (cs_5_1)
///
/// Only stages whose entry points appear in the source are compiled.
class ShaderAsset : public Asset {
public:
	const char* GetTypeName() const override { return "Shader"; }

	bool OnLoad(const std::vector<uint8>& rawData) override;
	bool OnFinalize(RHIDevice* pDevice) override;
	void OnUnload(RHIDevice* pDevice) override;

	RHIShaderHandle GetVertexShader()  const { return m_VS; }
	RHIShaderHandle GetPixelShader()   const { return m_PS; }
	RHIShaderHandle GetComputeShader() const { return m_CS; }

	bool HasVertexShader()  const { return m_VS.IsValid(); }
	bool HasPixelShader()   const { return m_PS.IsValid(); }
	bool HasComputeShader() const { return m_CS.IsValid(); }

private:
	std::string     m_sSource;
	bool            m_bHasVS = false;
	bool            m_bHasPS = false;
	bool            m_bHasCS = false;
	RHIShaderHandle m_VS;
	RHIShaderHandle m_PS;
	RHIShaderHandle m_CS;
};

} // namespace Evo

