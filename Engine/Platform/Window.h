#pragma once

#include "Core/Types.h"
#include <string>

struct SDL_Window;
union SDL_Event;

namespace Evo {

class Input;

using EventCallbackFn = bool(*)(const SDL_Event&);

struct WindowDesc {
	std::string sTitle  = "EvoEngine";
	uint32         uWidth  = 1280;
	uint32         uHeight = 720;
	bool        bResizable = true;
};

class Window {
public:
	Window() = default;
	~Window();

	bool Initialize(const WindowDesc& desc);
	void Shutdown();

	/// Poll events. Returns false if quit was requested.
	/// If pInput is provided, events are forwarded for input tracking.
	bool PollEvents(Input* pInput = nullptr);

	/// Toggle relative mouse mode (cursor hidden, raw delta).
	void SetRelativeMouseMode(bool bEnable);

	uint32  GetWidth()  const { return m_uWidth; }
	uint32  GetHeight() const { return m_uHeight; }
	bool IsMinimized() const { return m_bMinimized; }

	/// Get platform-native window handle (HWND on Windows).
	void* GetNativeHandle() const;

	/// Set a callback to receive raw SDL events (e.g. for ImGui).
	void SetEventCallback(EventCallbackFn fn) { m_pEventCallback = fn; }

	SDL_Window* GetSDLWindow() const { return m_pWindow; }

private:
	SDL_Window*      m_pWindow         = nullptr;
	uint32           m_uWidth          = 0;
	uint32           m_uHeight         = 0;
	bool             m_bMinimized      = false;
	EventCallbackFn  m_pEventCallback  = nullptr;
};

} // namespace Evo

