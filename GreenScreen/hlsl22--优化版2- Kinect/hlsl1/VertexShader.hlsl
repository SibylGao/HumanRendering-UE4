#include "Header.hlsli"

// ������ɫ��
VertexOut VS(VertexIn vIn)
{
    VertexOut vOut;
    vOut.posH = float4(vIn.pos, 1.0f);
    vOut.color = vIn.color; // ����alphaͨ����ֵĬ��Ϊ1.0
	vOut.Tex = vIn.Tex;
    return vOut;
}
