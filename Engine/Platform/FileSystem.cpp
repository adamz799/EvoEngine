#include "Platform/FileSystem.h"
#include "Core/Log.h"

#include <fstream>
#include <SDL3/SDL.h>

namespace Evo {

std::vector<uint8> FileSystem::ReadBinary(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        EVO_LOG_ERROR("Failed to open file: {}", path.string());
        return {};
    }

    auto size = file.tellg();
    if (size <= 0) return {};

    std::vector<uint8> buffer(static_cast<size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

std::optional<std::string> FileSystem::ReadText(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        EVO_LOG_ERROR("Failed to open text file: {}", path.string());
        return std::nullopt;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    return content;
}

bool FileSystem::WriteBinary(const std::filesystem::path& path, std::span<const uint8> data)
{
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        EVO_LOG_ERROR("Failed to write file: {}", path.string());
        return false;
    }
    file.write(reinterpret_cast<const char*>(data.data()),
               static_cast<std::streamsize>(data.size()));
    return file.good();
}

bool FileSystem::WriteText(const std::filesystem::path& path, const std::string& text)
{
    std::ofstream file(path);
    if (!file.is_open()) {
        EVO_LOG_ERROR("Failed to write text file: {}", path.string());
        return false;
    }
    file << text;
    return file.good();
}

bool FileSystem::Exists(const std::filesystem::path& path)
{
    return std::filesystem::exists(path);
}

std::filesystem::path FileSystem::GetExecutableDir()
{
    const char* basePath = SDL_GetBasePath();
    if (basePath) {
        return std::filesystem::path(basePath);
    }
    return std::filesystem::current_path();
}

} // namespace Evo
