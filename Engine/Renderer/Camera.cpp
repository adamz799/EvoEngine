#include "Renderer/Camera.h"
#include "Platform/Input.h"
#include "Platform/Window.h"
#include <cmath>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

namespace Evo {

// ============================================================================
// Camera
// ============================================================================

void Camera::SetPerspective(float fFovY, float fAspect, float fNearZ, float fFarZ)
{
	m_fFovY   = fFovY;
	m_fAspect = fAspect;
	m_fNearZ  = fNearZ;
	m_fFarZ   = fFarZ;
}

void Camera::SetAspect(float fAspect)
{
	m_fAspect = fAspect;
}

void Camera::SetPosition(const Vec3& vPos)
{
	m_vPosition = vPos;
}

void Camera::SetRotation(float fYaw, float fPitch)
{
	m_fYaw   = fYaw;
	m_fPitch = Clamp(fPitch, DegToRad(-89.0f), DegToRad(89.0f));
}

void Camera::LookAt(const Vec3& vTarget)
{
	Vec3 dir = (vTarget - m_vPosition).Normalized();
	m_fYaw   = std::atan2(dir.x, dir.z);
	m_fPitch = std::asin(Clamp(dir.y, -1.0f, 1.0f));
	m_fPitch = Clamp(m_fPitch, DegToRad(-89.0f), DegToRad(89.0f));
}

Vec3 Camera::GetForward() const
{
	float cp = std::cos(m_fPitch);
	return Vec3(
		std::sin(m_fYaw) * cp,
		std::sin(m_fPitch),
		std::cos(m_fYaw) * cp);
}

Vec3 Camera::GetRight() const
{
	// Simplified: right is always horizontal (no roll)
	return Vec3(std::cos(m_fYaw), 0.0f, -std::sin(m_fYaw));
}

Vec3 Camera::GetUp() const
{
	return Vec3::Cross(GetForward(), GetRight());
}

Mat4 Camera::GetViewMatrix() const
{
	return Mat4::LookAtLH(m_vPosition, m_vPosition + GetForward(), Vec3::Up);
}

Mat4 Camera::GetProjectionMatrix() const
{
	return Mat4::PerspectiveFovLH(m_fFovY, m_fAspect, m_fNearZ, m_fFarZ);
}

Mat4 Camera::GetViewProjectionMatrix() const
{
	return GetViewMatrix() * GetProjectionMatrix();
}

// ============================================================================
// FreeCameraController
// ============================================================================

void FreeCameraController::Update(Camera& camera, const Input& input,
                                   Window& window, float fDeltaTime)
{
	// ---- Mouse rotation (right-click drag) ----
	if (input.IsMouseButtonDown(SDL_BUTTON_RIGHT))
	{
		if (!m_bCapturing)
		{
			window.SetRelativeMouseMode(true);
			m_bCapturing = true;
		}

		Vec2 delta = input.GetMouseDelta();
		float yaw   = camera.GetYaw()   + delta.x * fMouseSensitivity;
		float pitch = camera.GetPitch() - delta.y * fMouseSensitivity;
		camera.SetRotation(yaw, pitch);

		// ---- WASD + QE movement ----
		Vec3 forward = camera.GetForward();
		Vec3 right   = camera.GetRight();

		float speed = fMoveSpeed;
		if (input.IsKeyDown(SDL_SCANCODE_LSHIFT) || input.IsKeyDown(SDL_SCANCODE_RSHIFT))
			speed *= fBoostMultiplier;

		Vec3 movement = Vec3::Zero;
		if (input.IsKeyDown(SDL_SCANCODE_W)) movement = movement + forward;
		if (input.IsKeyDown(SDL_SCANCODE_S)) movement = movement - forward;
		if (input.IsKeyDown(SDL_SCANCODE_D)) movement = movement + right;
		if (input.IsKeyDown(SDL_SCANCODE_A)) movement = movement - right;
		if (input.IsKeyDown(SDL_SCANCODE_E)) movement = movement + Vec3::Up;
		if (input.IsKeyDown(SDL_SCANCODE_Q)) movement = movement - Vec3::Up;

		if (movement.LengthSq() > 0.0f)
			camera.SetPosition(camera.GetPosition() + movement.Normalized() * speed * fDeltaTime);
	}
	else
	{
		if (m_bCapturing)
		{
			window.SetRelativeMouseMode(false);
			m_bCapturing = false;
		}
	}

	// ---- Scroll wheel → adjust move speed ----
	float scroll = input.GetScrollDelta();
	if (scroll != 0.0f)
		fMoveSpeed = Clamp(fMoveSpeed * (1.0f + scroll * 0.1f), 0.1f, 100.0f);
}

} // namespace Evo
