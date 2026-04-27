#pragma once

#include "Core/Types.h"
#include <string>

struct SDL_Window;

namespace Evo {

struct WindowDesc {
    std::string title  = "EvoEngine";
    uint32         width  = 1280;
    uint32         height = 720;
    bool        resizable = true;
};

class Window {
public:
    Window() = default;
    ~Window();

    bool Initialize(const WindowDesc& desc);
    void Shutdown();

    /// Poll events. Returns false if quit was requested.
    bool PollEvents();

    uint32  GetWidth()  const { return m_Width; }
    uint32  GetHeight() const { return m_Height; }
    bool IsMinimized() const { return m_Minimized; }

    /// Get platform-native window handle (HWND on Windows).
    void* GetNativeHandle() const;

    SDL_Window* GetSDLWindow() const { return m_Window; }

private:
    SDL_Window* m_Window    = nullptr;
    uint32         m_Width     = 0;
    uint32         m_Height    = 0;
    bool        m_Minimized = false;
};

} // namespace Evo
