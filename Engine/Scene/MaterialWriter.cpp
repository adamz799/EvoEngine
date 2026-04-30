#include "Scene/MaterialWriter.h"
#include "Platform/FileSystem.h"
#include "Core/Log.h"
#include "Material_generated.h"
#include <filesystem>

namespace Evo {

bool WriteMaterial(const std::string& sPath, const Vec3& vAlbedo,
				   float fRoughness, float fMetallic, float fAlpha)
{
	flatbuffers::FlatBufferBuilder builder(256);

	Schema::Vec3 albedo(vAlbedo.x, vAlbedo.y, vAlbedo.z);

	Schema::MaterialBuilder matBuilder(builder);
	matBuilder.add_albedo(&albedo);
	matBuilder.add_roughness(fRoughness);
	matBuilder.add_metallic(fMetallic);
	matBuilder.add_alpha(fAlpha);
	auto root = matBuilder.Finish();
	Schema::FinishMaterialBuffer(builder, root);

	std::filesystem::create_directories(std::filesystem::path(sPath).parent_path());
	auto data = std::span<const uint8>(builder.GetBufferPointer(), builder.GetSize());
	if (!FileSystem::WriteBinary(sPath, data))
	{
		EVO_LOG_ERROR("WriteMaterial: failed to write '{}'", sPath);
		return false;
	}

	EVO_LOG_INFO("WriteMaterial: wrote '{}' ({} bytes)", sPath, builder.GetSize());
	return true;
}

} // namespace Evo

