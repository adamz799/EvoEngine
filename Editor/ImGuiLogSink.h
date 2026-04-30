#pragma once

#include <spdlog/sinks/base_sink.h>
#include <deque>
#include <mutex>
#include <string>

namespace Evo {

struct LogEntry {
	spdlog::level::level_enum level;
	std::string sMessage;
};

/// Custom spdlog sink that stores log messages in a ring buffer.
/// Editor reads entries to display in the ImGui log panel.
class ImGuiLogSink : public spdlog::sinks::base_sink<std::mutex> {
public:
	const std::deque<LogEntry>& GetEntries() const { return m_vEntries; }

	void Clear()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		m_vEntries.clear();
	}

protected:
	void sink_it_(const spdlog::details::log_msg& msg) override
	{
		spdlog::memory_buf_t formatted;
		formatter_->format(msg, formatted);

		m_vEntries.push_back({ msg.level, fmt::to_string(formatted) });
		if (m_vEntries.size() > kMaxEntries)
			m_vEntries.pop_front();
	}

	void flush_() override {}

private:
	std::deque<LogEntry> m_vEntries;
	static constexpr size_t kMaxEntries = 512;
};

} // namespace Evo

