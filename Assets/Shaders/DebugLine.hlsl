// DebugLine.hlsl -- Simple colored line rendering

cbuffer PushConstants : register(b0)
{
	row_major float4x4 ViewProjection;
};

struct VSInput
{
	float3 position : POSITION;
	float4 color    : COLOR;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float4 color    : COLOR;
};

PSInput VSMain(VSInput input)
{
	PSInput output;
	output.position = mul(float4(input.position, 1.0), ViewProjection);
	output.color    = input.color;
	return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	return input.color;
}
