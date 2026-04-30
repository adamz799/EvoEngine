// DeferredLighting.hlsl -- Fullscreen deferred lighting pass with shadow mapping.
// Reads G-Buffer textures + scene depth + shadow map via SRV Load().
// No vertex buffer needed: fullscreen triangle from SV_VertexID.

// G-Buffer SRVs (space1 because push constants use space0)
Texture2D<float4> gAlbedo    : register(t0, space1);
Texture2D<float4> gNormal    : register(t1, space1);
Texture2D<float4> gRoughMet  : register(t2, space1);
Texture2D<float>  gDepth     : register(t3, space1);
Texture2D<float>  gShadowMap : register(t4, space1);

cbuffer PushConstants : register(b0, space0)
{
	row_major float4x4 InvViewProj;
	row_major float4x4 LightViewProj;
	float3 LightDir;
	float  ShadowMapSize;
	float3 LightColor;
	float  _pad;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float2 uv       : TEXCOORD0;
};

// Fullscreen triangle: 3 vertices cover the entire screen
VSOutput VSMain(uint vertexID : SV_VertexID)
{
	VSOutput output;
	// Generate oversized triangle: (0,0), (2,0), (0,2) in UV space
	output.uv = float2((vertexID << 1) & 2, vertexID & 2);
	output.position = float4(output.uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
	return output;
}

float SampleShadow(float3 worldPos)
{
	float4 lightClip = mul(float4(worldPos, 1.0), LightViewProj);

	// NDC to texture UV
	float2 shadowUV;
	shadowUV.x = lightClip.x * 0.5 + 0.5;
	shadowUV.y = -lightClip.y * 0.5 + 0.5;
	float currentDepth = lightClip.z;

	// Out-of-bounds check
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

float4 PSMain(VSOutput input) : SV_TARGET
{
	int2 coord = int2(input.position.xy);

	float4 albedoSample   = gAlbedo.Load(int3(coord, 0));
	float4 normalSample   = gNormal.Load(int3(coord, 0));
	float4 roughMetSample = gRoughMet.Load(int3(coord, 0));
	float  depth          = gDepth.Load(int3(coord, 0));

	// Skip background pixels (depth == 0 means nothing was rendered)
	if (depth >= 1.0)
		return float4(0.1, 0.12, 0.15, 1.0);

	// Decode world normal from [0,1] to [-1,1]
	float3 N = normalize(normalSample.rgb * 2.0 - 1.0);

	// Reconstruct world position from depth
	float2 screenSize;
	gAlbedo.GetDimensions(screenSize.x, screenSize.y);
	float2 ndc;
	ndc.x = (input.position.x / screenSize.x) * 2.0 - 1.0;
	ndc.y = 1.0 - (input.position.y / screenSize.y) * 2.0;
	float4 clipPos = float4(ndc, depth, 1.0);
	float4 worldPos = mul(clipPos, InvViewProj);
	worldPos /= worldPos.w;

	// Shadow
	float shadow = SampleShadow(worldPos.xyz);

	// Lighting
	float3 lightDir = normalize(LightDir);
	float ambientStrength = 0.15;

	float NdotL = saturate(dot(N, lightDir));
	float3 diffuse = albedoSample.rgb * LightColor * NdotL * shadow;
	float3 ambient = albedoSample.rgb * ambientStrength;

	float3 color = ambient + diffuse;
	return float4(color, 1.0);
}

