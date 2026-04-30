#include "Asset/ShaderAsset.h"
#include "RHI/RHIDevice.h"
#include "Core/Log.h"

#if EVO_RHI_DX12
#include "RHI/DX12/DX12ShaderCompiler.h"
#endif

namespace Evo {

bool ShaderAsset::OnLoad(const std::vector<uint8>& rawData)
{
	if (rawData.empty())
		return false;

	m_sSource.assign(rawData.begin(), rawData.end());

	// Detect entry points by simple string search
	m_bHasVS = m_sSource.find("VSMain") != std::string::npos;
	m_bHasPS = m_sSource.find("PSMain") != std::string::npos;
	m_bHasCS = m_sSource.find("CSMain") != std::string::npos;

	if (!m_bHasVS && !m_bHasPS && !m_bHasCS)
	{
		EVO_LOG_ERROR("ShaderAsset::OnLoad — no known entry points in '{}'", m_sPath);
		return false;
	}

	return true;
}

bool ShaderAsset::OnFinalize(RHIDevice* pDevice)
{
#if EVO_RHI_DX12
	const char* sourceName = m_sPath.c_str();

	if (m_bHasVS)
	{
		auto blob = CompileShaderFromSource(
			m_sSource.data(), m_sSource.size(),
			sourceName, "VSMain", "vs_5_1");
		if (!blob) return false;

		RHIShaderDesc desc = {};
		desc.pBytecode     = blob->GetBufferPointer();
		desc.uBytecodeSize = blob->GetBufferSize();
		desc.stage         = RHIShaderStage::Vertex;
		desc.sDebugName    = m_sPath + ":VS";
		m_VS = pDevice->CreateShader(desc);
		if (!m_VS.IsValid()) return false;
	}

	if (m_bHasPS)
	{
		auto blob = CompileShaderFromSource(
			m_sSource.data(), m_sSource.size(),
			sourceName, "PSMain", "ps_5_1");
		if (!blob) return false;

		RHIShaderDesc desc = {};
		desc.pBytecode     = blob->GetBufferPointer();
		desc.uBytecodeSize = blob->GetBufferSize();
		desc.stage         = RHIShaderStage::Pixel;
		desc.sDebugName    = m_sPath + ":PS";
		m_PS = pDevice->CreateShader(desc);
		if (!m_PS.IsValid()) return false;
	}

	if (m_bHasCS)
	{
		auto blob = CompileShaderFromSource(
			m_sSource.data(), m_sSource.size(),
			sourceName, "CSMain", "cs_5_1");
		if (!blob) return false;

		RHIShaderDesc desc = {};
		desc.pBytecode     = blob->GetBufferPointer();
		desc.uBytecodeSize = blob->GetBufferSize();
		desc.stage         = RHIShaderStage::Compute;
		desc.sDebugName    = m_sPath + ":CS";
		m_CS = pDevice->CreateShader(desc);
		if (!m_CS.IsValid()) return false;
	}

	// Source no longer needed
	m_sSource.clear();
	m_sSource.shrink_to_fit();

	EVO_LOG_INFO("ShaderAsset '{}' compiled (VS={} PS={} CS={})",
				 m_sPath, m_bHasVS, m_bHasPS, m_bHasCS);
	return true;
#else
	(void)pDevice;
	EVO_LOG_ERROR("ShaderAsset::OnFinalize — no RHI backend available");
	return false;
#endif
}

void ShaderAsset::OnUnload(RHIDevice* pDevice)
{
	if (m_VS.IsValid()) { pDevice->DestroyShader(m_VS); m_VS = {}; }
	if (m_PS.IsValid()) { pDevice->DestroyShader(m_PS); m_PS = {}; }
	if (m_CS.IsValid()) { pDevice->DestroyShader(m_CS); m_CS = {}; }
	m_sSource.clear();
}

} // namespace Evo

