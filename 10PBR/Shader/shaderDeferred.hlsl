struct PSInput
{
	float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
	float2 uv : TEXCOORD;
    float4 world : TEXCOORD1;
};

cbuffer MVPBuffer : register(b0)
{
	float4x4 m_MVP;
    float4x4 m_ObjectToWorld;
    float4 m_ViewPos;
    float3 lightDir;
    
    float baseColorIntensity = 1.5;
    float metallicIntensity = 0.3;
    float roughnessIntensity = 0.3;
    
    float subsurface;
    float _specular;
    float specularTint;
    float anisotropic;
    float sheen;
    float sheenTint;
    float clearcoat;
    float clearcoatGloss;
};
struct PSOutput
{
    float4 halfcolor0 : SV_Target0;
    float4 halfcolor1 : SV_Target1;
};

Texture2D baseColor : register(t0);
Texture2D metallic : register(t1);
Texture2D normal : register(t2);
Texture2D roughness : register(t3);



SamplerState g_sampler : register(s0);



float3 NormalSampleToWorldSpace(float3 normalMapSample,
    float3 unitNormalW,
    float4 tangentW)
{
    // 将读取到法向量中的每个分量从[0, 1]还原到[-1, 1]
    float3 normalT = 2.0f * normalMapSample - 1.0f;

    // 构建位于世界坐标系的切线空间
    float3 N = unitNormalW;
    float3 T = normalize(tangentW.xyz - dot(tangentW.xyz, N) * N); // 施密特正交化
    float3 B = cross(N, T);

    float3x3 TBN = float3x3(T, B, N);

    // 将凹凸法向量从切线空间变换到世界坐标系
    float3 bumpedNormalW = mul(normalT, TBN);

    return bumpedNormalW;
}




//PSInput VSMain(float3 position : POSITION, float3 normal : NORMAL, float3 tangent : TANGENT, float2 uv : TEXCOORD)
//{
//	PSInput result;

//    result.position = mul(float4(position, 1.0f), m_MVP);
//	result.uv = uv;
//    result.normal = normal;
//    result.tangent = tangent;
//	return result;
//}

//PSOutput PSMain(PSInput input)
//{
    
//    PSOutput o;
//    //NormalSampleToWorldSpace(
//    o.halfcolor0 = normal.Sample(g_sampler, input.uv) / 2.0f;
//    //o.halfcolor0 += float4(input.tangent, 1.0f);
//    o.halfcolor1 = o.halfcolor0;
//    return o;
//}









static const float PI = 3.14159265358979323846;
 
float sqr(float x)
{
    return x * x;
}
 
float SchlickFresnel(float u)
{
    float m = clamp(1 - u, 0, 1);
    float m2 = m * m;
    return m2 * m2 * m; // pow(m,5)
}
 
float GTR1(float NdotH, float a)
{
    if (a >= 1)
        return 1 / PI;
    float a2 = a * a;
    float t = 1 + (a2 - 1) * NdotH * NdotH;
    return (a2 - 1) / (PI * log(a2) * t);
}
 
float GTR2(float NdotH, float a)
{
    float a2 = a * a;
    float t = 1 + (a2 - 1) * NdotH * NdotH;
    return a2 / (PI * t * t);
}
 
float GTR2_aniso(float NdotH, float HdotX, float HdotY, float ax, float ay)
{
    return 1 / (PI * ax * ay * sqr(sqr(HdotX / ax) + sqr(HdotY / ay) + NdotH * NdotH));
}
 
float smithG_GGX(float NdotV, float alphaG)
{
    float a = alphaG * alphaG;
    float b = NdotV * NdotV;
    return 1 / (NdotV + sqrt(a + b - a * b));
}
 
float smithG_GGX_aniso(float NdotV, float VdotX, float VdotY, float ax, float ay)
{
    return 1 / (NdotV + sqrt(sqr(VdotX * ax) + sqr(VdotY * ay) + sqr(NdotV)));
}
 
float3 mon2lin(float3 x)
{
    return float3(pow(x[0], 2.2), pow(x[1], 2.2), pow(x[2], 2.2));
}
 
float3 BRDF(float3 L, float3 V, float3 N, float3 X, float3 Y, float2 uv)
{
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
 
    float3 H = normalize(L + V);
    float NdotH = max(dot(N, H), 0.0);
    float LdotH = max(dot(L, H), 0.0);
    float3 baseColorP = baseColor.Sample(g_sampler, uv).xyz * baseColorIntensity;
    float metallicP = metallic.Sample(g_sampler, uv).x * metallicIntensity;
    float roughnessP = roughness.Sample(g_sampler, uv).x * roughnessIntensity;
    
    float3 Cdlin = mon2lin(baseColorP);
    float Cdlum = .3 * Cdlin[0] + .6 * Cdlin[1] + .1 * Cdlin[2]; // luminance approx.
 
    float3 Ctint = Cdlum > 0 ? Cdlin / Cdlum : float3(1, 1, 1); // normalize lum. to isolate hue+sat
    float3 Cspec0 = lerp(_specular * .08 * lerp(float3(1, 1, 1), Ctint, specularTint), Cdlin, metallicP);
    float3 Csheen = lerp(float3(1, 1, 1), Ctint, sheenTint);
                // Diffuse fresnel - go from 1 at normal incidence to .5 at grazing
                // and lerp in diffuse retro-reflection based on roughness
    float FL = SchlickFresnel(NdotL), FV = SchlickFresnel(NdotV);
    float Fd90 = 0.5 + 2 * LdotH * LdotH * roughnessP;
    float Fd = lerp(1.0, Fd90, FL) * lerp(1.0, Fd90, FV);
    
                // Based on Hanrahan-Krueger brdf approximation of isotropic bssrdf
                // 1.25 scale is used to (roughly) preserve albedo
                // Fss90 used to "flatten" retroreflection based on roughness
    float Fss90 = LdotH * LdotH * roughnessP;
    float Fss = lerp(1.0, Fss90, FL) * lerp(1.0, Fss90, FV);
    float ss = 1.25 * (Fss * (1 / (NdotL + NdotV) - .5) + .5);
    //return N; //ss / 40.0f;
    //上边去计算了sss和diffuse，这俩其实是一回事，为啥要计算两个呢？虽说是一回事，但是还是有些区别的，重点是下面他们俩合并的方式是通过插值来合并的，所以这是非常合理的。
              

    // specular
    float aspect = sqrt(1 - anisotropic * .9);
    float ax = max(.001, sqr(roughnessP) / aspect);
    float ay = max(.001, sqr(roughnessP) * aspect);
    float Ds = GTR2_aniso(NdotH, dot(H, X), dot(H, Y), ax, ay);
    float FH = SchlickFresnel(LdotH);
    float3 Fs = lerp(Cspec0, float3(1, 1, 1), FH);
    float Gs = smithG_GGX_aniso(NdotL, dot(L, X), dot(L, Y), ax, ay);
    Gs *= smithG_GGX_aniso(NdotV, dot(V, X), dot(V, Y), ax, ay);

                // sheen
    float3 Fsheen = FH * sheen * Csheen;
 
                // clearcoat (ior = 1.5 -> F0 = 0.04)
    float Dr = GTR1(NdotH, lerp(.1, .001, clearcoatGloss));
    float Fr = lerp(.04, 1.0, FH);
    float Gr = smithG_GGX(NdotL, .25) * smithG_GGX(NdotV, .25);
 
    return ((1 / PI) * lerp(Fd, ss, subsurface) * Cdlin + Fsheen) * (1 - metallicP) +Gs * Fs * Ds + .25 * clearcoat * Gr * Fr * Dr;
}


struct VSInput
{
    float3 vertex : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD;
};

PSInput VSMain(VSInput i)
{
    PSInput o;
    o.world = mul(m_ObjectToWorld, float4(i.vertex, 1));
    o.position = mul(float4(i.vertex, 1.0f), m_MVP);
    o.normal = i.normal;
    //2.0f; //normal.Sample(g_sampler, i.uv).xyz;//    NormalSampleToWorldSpace(normal.Sample(g_sampler, i.uv).xyz, mul(m_ObjectToWorld, float4(i.normal, 1)).xyz, mul(m_ObjectToWorld, float4(i.tangent, 1)));
    o.tangent = i.tangent;
    o.uv = i.uv;
    return o;
}
 
float4 PSMain(PSInput i) : SV_TARGET
{
    float3 LightDirection = normalize(lightDir); //float3 LightDirection = normalize(lerp(_WorldSpaceLightPos0.xyz, _WorldSpaceLightPos0.xyz - world, _WorldSpaceLightPos0.w));
    float3 normalW = NormalSampleToWorldSpace(normal.Sample(g_sampler, i.uv).xyz, mul(m_ObjectToWorld, float4(i.normal, 1)).xyz, mul(m_ObjectToWorld, float4(i.tangent, 1)));
    float3 NormalDirection = normalize(normalW); //   normalize(mul((float3x3) m_ObjectToWorld, i.normal));
    float3 ViewDirection = normalize(m_ViewPos.xyz - i.world.xyz);
    float3 WorldTangent = mul((float3x3) m_ObjectToWorld, i.tangent.xyz);
    float3 WorldBinormal = cross(NormalDirection, WorldTangent);// * tangent.w;模型中读取的是3维的tangent，这里就不加了。
    return float4(BRDF(LightDirection, ViewDirection, NormalDirection, WorldTangent, WorldBinormal, i.uv), 1.0);
}

