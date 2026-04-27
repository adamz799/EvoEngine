#pragma once

#include "Core/Types.h"
#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include <span>

namespace Evo {

class FileSystem {
public:
    /// Read entire file as binary bytes. Returns empty on failure.
    static std::vector<uint8> ReadBinary(const std::filesystem::path& path);

    /// Read entire file as text string. Returns nullopt on failure.
    static std::optional<std::string> ReadText(const std::filesystem::path& path);

    /// Write binary data to file. Returns true on success.
    static bool WriteBinary(const std::filesystem::path& path, std::span<const uint8> data);

    /// Write text to file. Returns true on success.
    static bool WriteText(const std::filesystem::path& path, const std::string& text);

    /// Check if file exists.
    static bool Exists(const std::filesystem::path& path);

    /// Get the executable's directory.
    static std::filesystem::path GetExecutableDir();
};

} // namespace Evo
