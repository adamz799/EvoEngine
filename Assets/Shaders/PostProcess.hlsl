// PostProcess.hlsl — Fullscreen post-processing pass.
// Reads HDR intermediate via SRV Load(), applies ACES tone mapping + sRGB gamma.

Texture2D<float4> gHDRInput : register(t0, space0);

struct VSOutput
{
	float4 position : SV_POSITION;
	float2 uv       : TEXCOORD0;
};

// Fullscreen triangle
VSOutput VSMain(uint vertexID : SV_VertexID)
{
	VSOutput output;
	output.uv = float2((vertexID << 1) & 2, vertexID & 2);
	output.position = float4(output.uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
	return output;
}

// ACES filmic tone mapping (fitted curve by Krzysztof Narkowicz)
float3 ACESFilm(float3 x)
{
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// Linear to sRGB
float3 LinearToSRGB(float3 color)
{
	return pow(max(color, 0.0), 1.0 / 2.2);
}

float4 PSMain(VSOutput input) : SV_TARGET
{
	int2 coord = int2(input.position.xy);
	float3 hdrColor = gHDRInput.Load(int3(coord, 0)).rgb;

	// Exposure (simple fixed exposure for now)
	float exposure = 1.0;
	hdrColor *= exposure;

	// Tone mapping
	float3 mapped = ACESFilm(hdrColor);

	// Gamma correction
	float3 result = LinearToSRGB(mapped);

	return float4(result, 1.0);
}
