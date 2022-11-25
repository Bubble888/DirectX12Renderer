//=============================================================================
// Performs a separable Guassian blur with a blur radius up to 5 pixels.
//=============================================================================

cbuffer cbSettings : register(b0)
{
    int curBlurTimes;
};

Texture2D gInput            : register(t0);
RWTexture2D<float4> gOutput : register(u0);

SamplerState samState : register(s0);

float4 sampleCorner(float curBlurTimes, int3 dispatchThreadID,int2 direction)
{
    float u = (dispatchThreadID.x + direction.x * (curBlurTimes - 0.5)) / 1024.0f;
    float v = (dispatchThreadID.y + direction.y * (curBlurTimes - 0.5)) / 768.0f;

    return gInput.SampleLevel(samState, float2(u, v), 0.0f);
	//是否需要设置 采样时的双线性插值
}

#define N 16

[numthreads(N, N, 1)]
void KawaseBlurCS(int3 dispatchThreadID : SV_DispatchThreadID)
{
    gOutput[dispatchThreadID.xy] = 
        (sampleCorner(curBlurTimes + 1, dispatchThreadID, float2(1, 1)) +
        sampleCorner(curBlurTimes + 1, dispatchThreadID, float2(1, -1)) +
        sampleCorner(curBlurTimes + 1, dispatchThreadID, float2(-1, 1)) +
        sampleCorner(curBlurTimes + 1, dispatchThreadID, float2(-1, -1))) / 4.0f;
    //传递过来的curBlurTimes是循环的索引值，所以这里加1才是当前第几次blur
}

