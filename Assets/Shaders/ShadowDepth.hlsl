// ShadowDepth.hlsl — Depth-only pass for shadow map generation.
// VS transforms vertices by light MVP; no pixel shader output.

cbuffer PushConstants : register(b0)
{
	row_major float4x4 LightMVP;
};

struct VSInput
{
	float3 position : POSITION;
};

float4 VSMain(VSInput input) : SV_POSITION
{
	return mul(float4(input.position, 1.0), LightMVP);
}

