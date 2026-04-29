// ForwardTransparent.hlsl — Forward-lit pass for transparent objects.
// Renders after deferred lighting, reads depth (depth test on, depth write off),
// samples shadow map, outputs alpha-blended color into HDR target.

Texture2D<float> gShadowMap : register(t0, space1);

cbuffer PushConstants : register(b0, space0)
{
	row_major float4x4 MVP;
	row_major float4x4 LightViewProj;
	float3 AlbedoColor;
	float  Roughness;
	float  Metallic;
	float  Alpha;
	float3 LightDir;
	float  ShadowMapSize;
	float3 LightColor;
	float  _pad;
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
	float3 worldPos    : TEXCOORD1;
};

PSInput VSMain(VSInput input)
{
	PSInput output;
	output.position    = mul(float4(input.position, 1.0), MVP);
	output.worldNormal = input.normal;
	output.worldPos    = input.position;
	return output;
}

float SampleShadow(float3 worldPos)
{
	float4 lightClip = mul(float4(worldPos, 1.0), LightViewProj);

	float2 shadowUV;
	shadowUV.x = lightClip.x * 0.5 + 0.5;
	shadowUV.y = -lightClip.y * 0.5 + 0.5;
	float currentDepth = lightClip.z;

	if (shadowUV.x < 0.0 || shadowUV.x > 1.0 || shadowUV.y < 0.0 || shadowUV.y > 1.0)
		return 1.0;

	// 3x3 PCF
	float shadow = 0.0;
	float texelSize = 1.0 / ShadowMapSize;
	float bias = 0.002;

	[unroll]
	for (int y = -1; y <= 1; ++y)
	{
		[unroll]
		for (int x = -1; x <= 1; ++x)
		{
			float2 sampleUV = shadowUV + float2(x, y) * texelSize;
			int2 coord = int2(sampleUV * ShadowMapSize);
			float storedDepth = gShadowMap.Load(int3(coord, 0));
			shadow += (currentDepth - bias > storedDepth) ? 0.0 : 1.0;
		}
	}

	return shadow / 9.0;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	float3 N = normalize(input.worldNormal);
	float3 lightDir = normalize(LightDir);

	float shadow = SampleShadow(input.worldPos);

	float ambientStrength = 0.15;
	float NdotL = saturate(dot(N, lightDir));
	float3 diffuse = AlbedoColor * LightColor * NdotL * shadow;
	float3 ambient = AlbedoColor * ambientStrength;

	float3 color = ambient + diffuse;
	return float4(color, Alpha);
}
