#include "Header.hlsli"

// ��3��������ɫ��--ԭ���������
float4 main(VertexOut pIn) : SV_Target/*����SV_TARGET��ʾ����ֵ����Ҫ����ȾĿ��ĸ�ʽƥ�䡣*/
{
	float4 pixel_color=gTex_now.Sample(gSamLinear, pIn.Tex);/*��������t2��Ӧ����������ֵ*/
	return pixel_color;
}