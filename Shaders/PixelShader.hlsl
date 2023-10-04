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

float4 main(outputToRasterizer outputVS) : SV_TARGET
{
    // Lighting Variables
    float alphaTransparency = material.d;
    float3 surfaceColor = material.Kd;
    float3 light_Color = lightColor.xyz;
    float3 lightNormal = normalize(lightDir.xyz);
    float3 surfaceNormal = normalize(outputVS.normW);
    float3 viewDir = normalize(camWorldPos.xyz - outputVS.posW.xyz);
    float3 halfVector = normalize((-lightNormal) + viewDir);
    float specularExponent = material.Ns + 0.000001f;
    float3 specularColor = material.Ks;
    float3 ambientTerm = sunAmbient.xyz;
    float3 ambientLight = ambientTerm * material.Ka;
    float3 emissive = material.Ke;
    
    // directional
    float3 directional = surfaceColor;
    float lightRatio = saturate(dot(-lightNormal, surfaceNormal));
    directional = lightRatio * light_Color;
    
    // specular
    float intensity = max(pow(saturate(dot(surfaceNormal, halfVector)), specularExponent), 0);
    float3 specular =  specularColor * intensity;
    
    // final result calculation
    float3 finalResult = saturate(directional + ambientLight) * surfaceColor + specular + emissive;
    
    return float4(finalResult, alphaTransparency);
}