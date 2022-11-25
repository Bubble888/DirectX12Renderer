//=============================================================================
// Performs a separable Guassian blur with a blur radius up to 5 pixels.
//=============================================================================

cbuffer cbSettings : register(b0)
{
    int curBlurTimes;
};

Texture2D gInput            : register(t0);
RWTexture2D<float4> gOutput : register(u0);
RWTexture2D<float4> gMipmap : register(u1);


SamplerState samState : register(s0);


float4 sampleCorner(int2 cornerLeftUpCoord)
{
    return (gMipmap[cornerLeftUpCoord] + gMipmap[cornerLeftUpCoord + int2(1, 0)] + 
                gMipmap[cornerLeftUpCoord + int2(0, 1)] + gMipmap[cornerLeftUpCoord + int2(1, 1)]) / 4;
}
#define N 16

[numthreads(N, N, 1)]
void DualBlurCS(int3 dispatchThreadID : SV_DispatchThreadID)
{
    int u = (dispatchThreadID.x + 1) * 2 - 1;
    int v = (dispatchThreadID.y + 1) * 2 - 1;
    if (curBlurTimes < 0){
        gMipmap[dispatchThreadID.xy] = (gInput[int2(u, v)] + gInput[int2(u, v - 1)] + gInput[int2(u - 1, v)] + gInput[int2(u - 1, v - 1)]) / 4.0f;
    
        GroupMemoryBarrierWithGroupSync();
        
        gOutput[dispatchThreadID.xy] = ((sampleCorner(dispatchThreadID.xy - int2(0, 1)) + sampleCorner(dispatchThreadID.xy - int2(1, 1)) +
                                                        sampleCorner(dispatchThreadID.xy - int2(1, 0)) +
                                                        sampleCorner(dispatchThreadID.xy)) * 2 + 
                                                        sampleCorner(dispatchThreadID.xy - int2(0,1)) + 
                                                        sampleCorner(dispatchThreadID.xy - int2(1,0)) + 
                                                        sampleCorner(dispatchThreadID.xy + int2(0, 1)) +
                                                        sampleCorner(dispatchThreadID.xy + int2(1, 0))  ) / 12;
    }
    else{
        gMipmap[int2(u, v)] = gInput[dispatchThreadID.xy];
        gMipmap[int2(u, v - 1)] = gInput[dispatchThreadID.xy];
        gMipmap[int2(u - 1, v)] = gInput[dispatchThreadID.xy];
        gMipmap[int2(u - 1, v - 1)] = gInput[dispatchThreadID.xy];
        
        GroupMemoryBarrierWithGroupSync();
        
        gOutput[dispatchThreadID.xy] = (gMipmap[dispatchThreadID.xy] * 4 + sampleCorner(dispatchThreadID.xy - int2(0, 1)) +
                                                        sampleCorner(dispatchThreadID.xy - int2(1, 1)) +
                                                        sampleCorner(dispatchThreadID.xy - int2(1, 0)) +
                                                        sampleCorner(dispatchThreadID.xy)) / 8;

    }
}

