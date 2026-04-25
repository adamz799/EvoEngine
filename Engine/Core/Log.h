#pragma once

#include <spdlog/spdlog.h>
#include <memory>

namespace Evo {

class Log {
public:
    static void Initialize();
    static void Shutdown();

    static std::shared_ptr<spdlog::logger>& GetEngineLogger()  { return s_EngineLogger; }
    static std::shared_ptr<spdlog::logger>& GetClientLogger()  { return s_ClientLogger; }

private:
    static std::shared_ptr<spdlog::logger> s_EngineLogger;
    static std::shared_ptr<spdlog::logger> s_ClientLogger;
};

} // namespace Evo

// Engine log macros
#define EVO_LOG_TRACE(...)    ::Evo::Log::GetEngineLogger()->trace(__VA_ARGS__)
#define EVO_LOG_DEBUG(...)    ::Evo::Log::GetEngineLogger()->debug(__VA_ARGS__)
#define EVO_LOG_INFO(...)     ::Evo::Log::GetEngineLogger()->info(__VA_ARGS__)
#define EVO_LOG_WARN(...)     ::Evo::Log::GetEngineLogger()->warn(__VA_ARGS__)
#define EVO_LOG_ERROR(...)    ::Evo::Log::GetEngineLogger()->error(__VA_ARGS__)
#define EVO_LOG_CRITICAL(...) ::Evo::Log::GetEngineLogger()->critical(__VA_ARGS__)

// Client log macros
#define EVO_APP_TRACE(...)    ::Evo::Log::GetClientLogger()->trace(__VA_ARGS__)
#define EVO_APP_DEBUG(...)    ::Evo::Log::GetClientLogger()->debug(__VA_ARGS__)
#define EVO_APP_INFO(...)     ::Evo::Log::GetClientLogger()->info(__VA_ARGS__)
#define EVO_APP_WARN(...)     ::Evo::Log::GetClientLogger()->warn(__VA_ARGS__)
#define EVO_APP_ERROR(...)    ::Evo::Log::GetClientLogger()->error(__VA_ARGS__)
