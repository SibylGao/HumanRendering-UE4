#include "Header.hlsli"

/*�������ɰ�matte���ж�ֵ��������ȫ����Ϊ0��1*/
float4 main(VertexOut pIn) : SV_Target/*����SV_TARGET��ʾ����ֵ����Ҫ����ȾĿ��ĸ�ʽƥ�䡣*/
{
	float4 matte_value=gTex_now.Sample(gSamLinear, pIn.Tex);/*��������t2��Ӧ����������ֵ*/
	float value = matte_value[0];

	/*Step(a,value):���value<a����0��tmp=0�����value>��=a����1*/
	float tmp =step(binarization_threshold,value);
	value=tmp*1.0f+(1-tmp)*0.0f;/*�������0.9����ȫ������Ϊ1������ȫ��ԭֵ*/

	matte_value[0] = value;
	matte_value[1] = value;
	matte_value[2] = value;

	return matte_value;
}