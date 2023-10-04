// an ultra simple hlsl vertex shader
struct VS_IN
{
    float2 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

cbuffer SPRITE_DATA : register(b0)
{
    float2 pos_offset;
    float2 scale;
    float rotation;
    float depth;
};

VS_OUT main(VS_IN input)
{
    VS_OUT output = (VS_OUT) 0;

    float2 r = float2(cos(rotation), sin(rotation));
    float2x2 rotate = float2x2(r.x, -r.y, r.y, r.x);
    float2 pos = pos_offset + mul(rotate, input.pos * scale);

    output.pos = float4(pos, depth, 1.0f);
    output.uv = input.uv;

    return output;
}