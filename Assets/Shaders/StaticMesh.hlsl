// StaticMesh.hlsl — Per-object MVP via root constants

cbuffer PushConstants : register(b0)
{
	row_major float4x4 MVP;
};

struct VSInput
{
	float3 position : POSITION;
	float3 normal   : NORMAL;
	float2 uv       : TEXCOORD0;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float3 normal   : NORMAL;
	float2 uv       : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
	PSInput output;
	output.position = mul(float4(input.position, 1.0), MVP);
	output.normal   = input.normal;
	output.uv       = input.uv;
	return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	// Simple directional lighting for visualization
	float3 lightDir = normalize(float3(0.5, 1.0, -0.3));
	float ndotl = saturate(dot(normalize(input.normal), lightDir));
	float3 ambient = float3(0.15, 0.15, 0.2);
	float3 diffuse = float3(0.85, 0.85, 0.8) * ndotl;
	return float4(ambient + diffuse, 1.0);
}

