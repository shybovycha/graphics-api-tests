cbuffer Constants : register(b0) {
    matrix worldViewProj;
};

// Texture2D g_texture : register(t0);
// SamplerState g_sampler : register(s0);

struct VSInput {
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    output.position = mul(float4(input.position, 1.0f), worldViewProj);
    output.texCoord = input.texCoord;
    output.normal = mul(input.normal, (float3x3)worldViewProj);
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    // float4 textureColor = g_texture.Sample(g_sampler, input.texCoord);
    float4 textureColor { input.position.x, input.position.y, input.position.z, 1.0 };
    float3 lightDir = normalize(float3(1.0f, 1.0f, -1.0f));
    float diffuse = max(dot(normalize(input.normal), lightDir), 0.2f);
    return textureColor * diffuse;
}
