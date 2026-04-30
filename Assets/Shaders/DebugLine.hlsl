// DebugLine.hlsl -- Anti-aliased colored line rendering (quad expansion)

cbuffer PushConstants : register(b0)
{
	row_major float4x4 ViewProjection;
	float2 ScreenSize;
	float  LineWidth;
	float  _pad;
};

struct VSInput
{
	float3 position : POSITION;
	float3 other    : TEXCOORD0;
	float4 color    : COLOR;
	float  edgeDist : TEXCOORD1;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float4 color    : COLOR;
	float  edgeDist : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
	PSInput output;

	float4 clipPos   = mul(float4(input.position, 1.0), ViewProjection);
	float4 clipOther = mul(float4(input.other, 1.0), ViewProjection);

	// NDC positions
	float2 ndcPos   = clipPos.xy / clipPos.w;
	float2 ndcOther = clipOther.xy / clipOther.w;

	// Screen-space direction (correct for aspect ratio)
	float2 dir = ndcOther - ndcPos;
	float aspectRatio = ScreenSize.x / ScreenSize.y;
	dir.x *= aspectRatio;
	float len = length(dir);
	if (len > 0.0001)
		dir /= len;
	else
		dir = float2(1, 0);

	// Perpendicular in aspect-corrected space, then undo aspect
	float2 perp = float2(-dir.y, dir.x);
	perp.x /= aspectRatio;

	// Offset in NDC: LineWidth pixels -> NDC units
	float2 offset = perp * (LineWidth / ScreenSize) * input.edgeDist;
	clipPos.xy += offset * clipPos.w;

	output.position = clipPos;
	output.color    = input.color;
	output.edgeDist = input.edgeDist;
	return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	float alpha = 1.0 - smoothstep(0.5, 1.0, abs(input.edgeDist));
	return float4(input.color.rgb, input.color.a * alpha);
}

