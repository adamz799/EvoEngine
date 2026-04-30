#include "Scene/PrefabWriter.h"
#include "Platform/FileSystem.h"
#include "Core/Log.h"
#include "Prefab_generated.h"
#include <filesystem>

namespace Evo {

bool WritePrefab(const std::string& sPath, const std::string& sMeshPath)
{
	flatbuffers::FlatBufferBuilder builder(256);

	auto meshPathOffset = builder.CreateString(sMeshPath);
	auto root = Schema::CreatePrefab(builder, meshPathOffset);
	Schema::FinishPrefabBuffer(builder, root);

	std::filesystem::create_directories(std::filesystem::path(sPath).parent_path());
	auto data = std::span<const uint8>(builder.GetBufferPointer(), builder.GetSize());
	if (!FileSystem::WriteBinary(sPath, data))
	{
		EVO_LOG_ERROR("WritePrefab: failed to write '{}'", sPath);
		return false;
	}

	EVO_LOG_INFO("WritePrefab: wrote '{}' ({} bytes, mesh='{}')", sPath, builder.GetSize(), sMeshPath);
	return true;
}

} // namespace Evo

