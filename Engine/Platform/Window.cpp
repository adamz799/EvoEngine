#include "Platform/Window.h"
#include "Platform/Input.h"
#include "Core/Log.h"

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

namespace Evo {

Window::~Window()
{
    Shutdown();
}

bool Window::Initialize(const WindowDesc& desc)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        EVO_LOG_CRITICAL("Failed to initialize SDL: {}", SDL_GetError());
        return false;
    }

    Uint64 flags = 0;
    if (desc.bResizable)
        flags |= SDL_WINDOW_RESIZABLE;

    m_pWindow = SDL_CreateWindow(desc.sTitle.c_str(), desc.uWidth, desc.uHeight, flags);
    if (!m_pWindow) {
        EVO_LOG_CRITICAL("Failed to create SDL window: {}", SDL_GetError());
        return false;
    }

    m_uWidth  = desc.uWidth;
    m_uHeight = desc.uHeight;

    EVO_LOG_INFO("Window created: {}x{} \"{}\"", m_uWidth, m_uHeight, desc.sTitle);
    return true;
}

void Window::Shutdown()
{
    if (m_pWindow) {
        SDL_DestroyWindow(m_pWindow);
        m_pWindow = nullptr;
        EVO_LOG_INFO("Window destroyed");
    }
    SDL_Quit();
}

bool Window::PollEvents(Input* pInput)
{
    if (pInput)
        pInput->NewFrame();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (m_pEventCallback)
            m_pEventCallback(event);

        if (pInput)
            pInput->ProcessEvent(event);

        switch (event.type) {
            case SDL_EVENT_QUIT:
                return false;

            case SDL_EVENT_KEY_DOWN:
                if (event.key.scancode == SDL_SCANCODE_ESCAPE)
                    return false;
                break;

            case SDL_EVENT_WINDOW_RESIZED:
                m_uWidth  = static_cast<uint32>(event.window.data1);
                m_uHeight = static_cast<uint32>(event.window.data2);
                EVO_LOG_DEBUG("Window resized: {}x{}", m_uWidth, m_uHeight);
                break;

            case SDL_EVENT_WINDOW_MINIMIZED:
                m_bMinimized = true;
                break;

            case SDL_EVENT_WINDOW_RESTORED:
                m_bMinimized = false;
                break;
        }
    }
    return true;
}

void* Window::GetNativeHandle() const
{
#ifdef EVO_PLATFORM_WINDOWS
    return SDL_GetPointerProperty(
        SDL_GetWindowProperties(m_pWindow),
        SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#elif defined(EVO_PLATFORM_LINUX)
    // Try Wayland first, fall back to X11
    void* handle = SDL_GetPointerProperty(
        SDL_GetWindowProperties(m_pWindow),
        SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
    if (!handle) {
        // X11 window is a uint32 ID, cast through uintptr
        auto xwin = SDL_GetNumberProperty(
            SDL_GetWindowProperties(m_pWindow),
            SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
        handle = reinterpret_cast<void*>(static_cast<uintptr_t>(xwin));
    }
    return handle;
#else
    return nullptr;
#endif
}

void Window::SetRelativeMouseMode(bool bEnable)
{
    if (m_pWindow)
        SDL_SetWindowRelativeMouseMode(m_pWindow, bEnable);
}

} // namespace Evo