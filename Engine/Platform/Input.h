#pragma once

#include "Core/Types.h"
#include "Math/Vec2.h"

union SDL_Event;

namespace Evo {

/// Tracks per-frame input state from SDL events.
///
/// Usage:
///   window.PollEvents(&input)   -- calls NewFrame() + ProcessEvent() internally
///   input.IsKeyDown(...)        -- query after PollEvents returns
class Input {
public:
	/// Reset per-frame deltas (mouse, scroll). Called automatically by Window::PollEvents.
	void NewFrame();

	/// Accumulate a single SDL event. Called automatically by Window::PollEvents.
	void ProcessEvent(const SDL_Event& event);

	// ---- Keyboard ----
	/// Query key state by SDL scancode (e.g. SDL_SCANCODE_W).
	bool IsKeyDown(int scancode) const;

	// ---- Mouse ----
	bool  IsMouseButtonDown(uint8 uButton) const;
	Vec2  GetMouseDelta() const  { return m_vMouseDelta; }
	float GetScrollDelta() const { return m_fScrollDelta; }

private:
	const bool* m_pKeyState  = nullptr;
	int         m_nNumKeys   = 0;
	Vec2        m_vMouseDelta;
	float       m_fScrollDelta  = 0.0f;
	uint32      m_uMouseButtons = 0;
};

} // namespace Evo
