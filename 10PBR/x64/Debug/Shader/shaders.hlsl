struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};


Texture2D g_textureA : register(t0);
Texture2D g_textureB : register(t1);

SamplerState g_sampler : register(s0);

PSInput VSMain(float4 position : POSITION, float2 uv : TEXCOORD)
{
	PSInput result;

	result.uv = uv;
	
    result.position = position;
	return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return g_textureA.Sample(g_sampler, input.uv) + g_textureB.Sample(g_sampler, input.uv);
}
