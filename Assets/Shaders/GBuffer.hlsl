// GBuffer.hlsl — G-Buffer pass: outputs albedo, normal, roughness+metallic to MRT

cbuffer PushConstants : register(b0)
{
	row_major float4x4 MVP;
	float3 AlbedoColor;
	float  Roughness;
	float  Metallic;
	float3 _pad;
};

struct VSInput
{
	float3 position : POSITION;
	float3 normal   : NORMAL;
	float2 uv       : TEXCOORD0;
};

struct PSInput
{
	float4 position    : SV_POSITION;
	float3 worldNormal : NORMAL;
	float2 uv          : TEXCOORD0;
};

struct GBufferOutput
{
	float4 albedo   : SV_TARGET0;   // RGB = diffuse color, A = 1
	float4 normal   : SV_TARGET1;   // RGB = world normal [0,1], A = 1
	float4 roughMet : SV_TARGET2;   // R = roughness, G = metallic, BA = 0
};

PSInput VSMain(VSInput input)
{
	PSInput output;
	output.position    = mul(float4(input.position, 1.0), MVP);
	output.worldNormal = input.normal; // Object-space normal (no world matrix yet)
	output.uv          = input.uv;
	return output;
}

GBufferOutput PSMain(PSInput input)
{
	GBufferOutput output;
	output.albedo   = float4(AlbedoColor, 1.0);
	output.normal   = float4(normalize(input.worldNormal) * 0.5 + 0.5, 1.0);
	output.roughMet = float4(Roughness, Metallic, 0.0, 0.0);
	return output;
}

