#include "Core/Log.h"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Evo {

std::shared_ptr<spdlog::logger> Log::s_EngineLogger;
std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

void Log::Initialize()
{
    spdlog::set_pattern("%^[%T] [%n] %v%$");

    s_EngineLogger = spdlog::stdout_color_mt("EVO");
    s_EngineLogger->set_level(spdlog::level::trace);

    s_ClientLogger = spdlog::stdout_color_mt("APP");
    s_ClientLogger->set_level(spdlog::level::trace);

    s_EngineLogger->info("Log system initialized");
}

void Log::Shutdown()
{
    s_EngineLogger->info("Log system shutting down");
    spdlog::shutdown();
}

} // namespace Evo
