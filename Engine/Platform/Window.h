#pragma once

#include "Core/Types.h"
#include <string>

struct SDL_Window;

namespace Evo {

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
    bool PollEvents();

    uint32  GetWidth()  const { return m_uWidth; }
    uint32  GetHeight() const { return m_uHeight; }
    bool IsMinimized() const { return m_bMinimized; }

    /// Get platform-native window handle (HWND on Windows).
    void* GetNativeHandle() const;

    SDL_Window* GetSDLWindow() const { return m_pWindow; }

private:
    SDL_Window* m_pWindow    = nullptr;
    uint32         m_uWidth     = 0;
    uint32         m_uHeight    = 0;
    bool        m_bMinimized = false;
};

} // namespace Evo
