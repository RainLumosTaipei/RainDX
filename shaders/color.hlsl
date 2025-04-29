//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj; 
	float4 gPulseColor;
	float gTime;
};

struct VertexIn
{
	float3 PosL  : POSITION;
    float4 Color : COLOR;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
    float4 Color : COLOR;
};

// 顶点着色器
VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// 齐次坐标变换
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
	
	// 颜色不变
    vout.Color = vin.Color;
    
    return vout;
}

// 像素着色器
float4 PS(VertexOut pin) : SV_Target
{
	const float PI = 3.14159;
	float s = 0.5f * sin(2 * gTime - 0.25f * PI) + 0.5f;
	float c = lerp(pin.Color, gPulseColor, s);
    return c;
}


