#pragma once

#include "Math/Math.h"

namespace Evo {

class Input;
class Window;

/// Perspective camera with yaw/pitch orientation (LH coordinate system).
class Camera {
public:
	// ---- Projection ----
	void SetPerspective(float fFovY, float fAspect, float fNearZ, float fFarZ);
	void SetAspect(float fAspect);

	// ---- Transform ----
	void SetPosition(const Vec3& vPos);
	void SetRotation(float fYaw, float fPitch);
	void LookAt(const Vec3& vTarget);

	const Vec3& GetPosition() const { return m_vPosition; }
	float GetYaw()   const { return m_fYaw; }
	float GetPitch() const { return m_fPitch; }

	Vec3 GetForward() const;
	Vec3 GetRight()   const;
	Vec3 GetUp()      const;

	// ---- Matrices ----
	Mat4 GetViewMatrix() const;
	Mat4 GetProjectionMatrix() const;
	Mat4 GetViewProjectionMatrix() const;

private:
	Vec3  m_vPosition = Vec3::Zero;
	float m_fYaw      = 0.0f;
	float m_fPitch    = 0.0f;

	float m_fFovY   = DegToRad(60.0f);
	float m_fAspect = 16.0f / 9.0f;
	float m_fNearZ  = 0.1f;
	float m_fFarZ   = 100.0f;
};

/// FPS-style free camera controller.
///
/// Right-click + drag to rotate, WASD to move, Q/E for down/up,
/// Shift for speed boost, scroll wheel to adjust move speed.
class FreeCameraController {
public:
	void Update(Camera& camera, const Input& input, Window& window, float fDeltaTime);

	float fMoveSpeed        = 5.0f;
	float fBoostMultiplier  = 3.0f;
	float fMouseSensitivity = 0.003f;

private:
	bool m_bCapturing = false;
};

} // namespace Evo
