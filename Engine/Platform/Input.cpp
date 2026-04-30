#include "Platform/Input.h"

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

namespace Evo {

void Input::NewFrame()
{
	m_vMouseDelta  = Vec2::Zero;
	m_fScrollDelta = 0.0f;
	m_pKeyState    = SDL_GetKeyboardState(&m_nNumKeys);
}

void Input::ProcessEvent(const SDL_Event& event)
{
	switch (event.type)
	{
	case SDL_EVENT_MOUSE_MOTION:
		m_vMouseDelta.x += event.motion.xrel;
		m_vMouseDelta.y += event.motion.yrel;
		break;

	case SDL_EVENT_MOUSE_BUTTON_DOWN:
		m_uMouseButtons |= (1u << event.button.button);
		break;

	case SDL_EVENT_MOUSE_BUTTON_UP:
		m_uMouseButtons &= ~(1u << event.button.button);
		break;

	case SDL_EVENT_MOUSE_WHEEL:
		m_fScrollDelta += event.wheel.y;
		break;
	}
}

bool Input::IsKeyDown(int scancode) const
{
	if (!m_pKeyState || scancode < 0 || scancode >= m_nNumKeys)
		return false;
	return m_pKeyState[scancode];
}

bool Input::IsMouseButtonDown(uint8 uButton) const
{
	return (m_uMouseButtons & (1u << uButton)) != 0;
}

} // namespace Evo

