struct PSInput
{
	float4 position : SV_POSITION;
    float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

cbuffer MVPBuffer : register(b0)
{
	float4x4 m_MVP;
};
struct PSOutput
{
    float4 halfcolor0 : SV_Target0;
    float4 halfcolor1 : SV_Target1;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(float3 position : POSITION, float3 normal : NORMAL, float2 uv : TEXCOORD)
{
	PSInput result;

    result.position = mul(float4(position, 1.0f), m_MVP);
	result.uv = uv;
    result.normal = normal;

	return result;
}

PSOutput PSMain(PSInput input)
{
    PSOutput o;
	o.halfcolor0 = g_texture.Sample(g_sampler, input.uv) / 2.0f;
    o.halfcolor1 = o.halfcolor0;
    return o;
}
