#include "Header.hlsli"

//���������ǰ����ȥ����ɫ���
float4 PS(VertexOut pIn) : SV_Target/*����SV_TARGET��ʾ����ֵ����Ҫ����ȾĿ��ĸ�ʽƥ�䡣*/
{
	/*��һ������ȡǰ��ֵ*/
	float4 pixel_color=gTex_now.Sample(gSamLinear, pIn.Tex);/*�ɰ��Ӧ����������ֵ*/
	float4 pixel_color2=gTex.Sample(gSamLinear, pIn.Tex);/*ԭͼ��Ӧ����������ֵ*/
	float result[4];/*��ȡ����ǰ����ɫֵ*/
	result[0]=pixel_color2.r;
	result[1]=pixel_color2.g;
	result[2]=pixel_color2.b;
	result[3]=pixel_color.r;
	/*�ڶ�����ȥ��ɫ�����*/
	int other_1 = (primary_channel + 1) % 3;
	int other_2 = (primary_channel + 2) % 3;
	int min_channel = min(other_1, other_2);
	int max_channel = max(other_1, other_2);
	float average_value, amount; 
	average_value = despill_balance * result[min_channel] + (1.0f - despill_balance) * result[max_channel];
	amount = (result[primary_channel] - average_value);
	float amount_despill = despill_factor * amount;
	
	float tmp = step(amount_despill,0.0);/*amount_despill > 0.0f��ȥ����ɫ���*/
	result[primary_channel] = tmp*result[primary_channel] + (1.0-tmp)*(result[primary_channel] - amount_despill);

	return float4(result[0],result[1],result[2],result[3]);/*���ȥ��������ǰ��ͼ*/
}
