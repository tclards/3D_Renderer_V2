#pragma pack_matrix(row_major)

struct inputFromAssembler
{
    float3 pos : POS;
    float3 uvm : UVM;
    float3 nrm : NRM;
};

struct outputToRasterizer
{
    float4 posH : SV_Position;
    float3 posW : UVM;
    float3 normW : NRM;
};

struct ATTRIBUTES
{
    float3 Kd;
    float d;
    float3 Ks;
    float Ns;
    float3 Ka;
    float sharpness;
    float3 Tf;
    float Ni;
    float3 Ke;
    unsigned int illum;
};

cbuffer SceneData : register(B0)
{
    float4x4 viewMat, projMat;

    float4 lightDir;
    float4 lightColor;
    float4 camWorldPos;
    float4 sunAmbient;
};

cbuffer MeshData : register(B1)
{
    float4x4 worldMat;
    ATTRIBUTES material;
};

outputToRasterizer main(inputFromAssembler inputVertex)
{
    float4 pos = { inputVertex.pos, 1.0f };
    pos = mul(pos, worldMat);
    float4 posW = pos;
    pos = mul(pos, viewMat);
    pos = mul(pos, projMat);
    
    float4 normal = mul(float4(inputVertex.nrm, 0), worldMat);
    
    outputToRasterizer output;
    output.posH = pos;
    output.posW = posW.xyz;
    output.normW = normalize(normal.xyz);
    
    return output;
}