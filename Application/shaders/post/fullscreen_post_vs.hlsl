struct VSOut
{
    float4 PositionCS : SV_POSITION;
    float2 UV         : TEXCOORD0;
};

VSOut main(uint vid : SV_VertexID)
{
    VSOut o;
    float2 pos;
    float2 uv;

    if (vid == 0)
    {
        pos = float2(-1.0f, -1.0f);
        uv  = float2(0.0f, 1.0f);
    }
    else if (vid == 1)
    {
        pos = float2(-1.0f,  3.0f);
        uv  = float2(0.0f, -1.0f);
    }
    else
    {
        pos = float2( 3.0f, -1.0f);
        uv  = float2(2.0f, 1.0f);
    }

    o.PositionCS = float4(pos, 0.0f, 1.0f);
    o.UV = uv;
    return o;
}
