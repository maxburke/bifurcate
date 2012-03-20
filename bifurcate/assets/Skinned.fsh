struct vs_output
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 main(in vs_output input) : SV_TARGET {
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}
