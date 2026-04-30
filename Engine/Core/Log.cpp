#include "Core/Log.h"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Evo {

std::shared_ptr<spdlog::logger> Log::s_EngineLogger;
std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

void Log::Initialize()
{
	if (s_EngineLogger) return; // already initialized

	spdlog::set_pattern("%^[%T] [%n] %v%$");

	auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

	s_EngineLogger = std::make_shared<spdlog::logger>("EVO", consoleSink);
	s_EngineLogger->set_level(spdlog::level::trace);
	spdlog::register_logger(s_EngineLogger);

	s_ClientLogger = std::make_shared<spdlog::logger>("APP", consoleSink);
	s_ClientLogger->set_level(spdlog::level::trace);
	spdlog::register_logger(s_ClientLogger);

	s_EngineLogger->info("Log system initialized");
}

void Log::Shutdown()
{
	s_EngineLogger->info("Log system shutting down");
	spdlog::shutdown();
}

void Log::AddSink(spdlog::sink_ptr pSink)
{
	s_EngineLogger->sinks().push_back(pSink);
	s_ClientLogger->sinks().push_back(pSink);
}

} // namespace Evo

