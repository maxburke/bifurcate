sampler gPositionTexture : register(s0);
sampler gJointTexture : register(s1);

struct vs_output
{
    float4 position : POSITION;
    float2 uv : TEXCOORD;
};

float4 main(in vs_output input) : SV_TARGET {
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
