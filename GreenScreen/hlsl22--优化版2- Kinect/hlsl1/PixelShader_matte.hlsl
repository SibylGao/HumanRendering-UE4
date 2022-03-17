#include "Header.hlsli"

//������ر��Ͷ�--��ȡ����֮��Ĳ���--��ɫ��Ҫͨ��������ͨ��֮�����ɫ����
float get_pixel_saturation(float4 pixelColor,int primary_channel)
{
	int other_1 = (primary_channel + 1) % 3;
	int other_2 = (primary_channel + 2) % 3;
	int min_channel = min(other_1, other_2);
	int max_channel = max(other_1, other_2);
	float val = screen_balance * pixelColor[min_channel] + (1.0f - screen_balance) * pixelColor[max_channel];
	return (pixelColor[primary_channel] - val) * abs(1.0f - val);
}
//�ҵ�����������Сֵ
float min_fff(float a, float b, float c) {
	return min(min(a, b), c);
}
// ����--������ɫ��
float4 PS(VertexOut pIn) : SV_Target/*����SV_TARGET��ʾ����ֵ����Ҫ����ȾĿ��ĸ�ʽƥ�䡣*/
{
	float4 pixel_color=gTex_now.Sample(gSamLinear, pIn.Tex);/*��ȡ���������ĳ���������Ӧ����ɫ*/
	/*û�й����ع�������*/
	/*��ȡ��ǰ��ɫ�Լ�������ɫ�ı��Ͷ�*/
	float saturation = get_pixel_saturation(pixel_color,primary_channel);
	float screen_saturation = get_pixel_saturation(screen_color, primary_channel);
	float distance = 1.0f - saturation / screen_saturation;
	float matte_value = distance;/*����Ϊ��Ե*/

	float tmp1 = step(0.0,saturation);/*��saturation < 0���ʾǰ����matteֵΪ1*/
	matte_value=(1.0 - tmp1)*1.0f + tmp1 * matte_value;

	float tmp2 = step(screen_saturation, saturation);/*saturation >= screen_saturation���ʾ������ֵΪ0*/
	matte_value=(1.0 - tmp2)*matte_value + tmp2 * 0.0f;

	/*���ǳ����˹����ع�����*/
	/*�����عⲻ�ᷢ������Ļ�����ϣ�ͨ�������ھ�ͷ�еĹ�Դ�ϣ�����Ҫ������飬
		��Ϊ���ͶȺ�˥���ļ����ǻ�������û�й����ع����ʵ��*/
	float min_pixel_color = min_fff(pixel_color[0], pixel_color[1], pixel_color[2]);
	float tmp3=step(min_pixel_color , 1.0f);/*���min_pixel_color > 1.0f������˹����ع�*/
	matte_value=tmp3*matte_value+(1-tmp3)*1.0f;

	return float4(matte_value,matte_value,matte_value,1.0);/*����ɰ�*/
}
//float4 pixel_color=gTex.Sample(gSamLinear, pIn.Tex);/*��ȡ���������ĳ���������Ӧ����ɫ*/
//float matte_value = (1.0 - tmp1)*1.0f + tmp1*(tmp2*0.0f + (1.0 - tmp2)*distance);
