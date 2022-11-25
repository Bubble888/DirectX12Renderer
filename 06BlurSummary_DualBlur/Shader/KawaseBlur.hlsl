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
	//�Ƿ���Ҫ���� ����ʱ��˫���Բ�ֵ
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
    //���ݹ�����curBlurTimes��ѭ��������ֵ�����������1���ǵ�ǰ�ڼ���blur
}

