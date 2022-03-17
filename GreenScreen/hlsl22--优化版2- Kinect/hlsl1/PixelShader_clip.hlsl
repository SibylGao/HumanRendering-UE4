#include "Header.hlsli"

/*ǯ�ƺ�ɫ��ɫ*/
float4 main(VertexOut pIn) : SV_Target
{
	float4 pixel_color=gTex_now.Sample(gSamLinear, pIn.Tex);
	int delta = kernelRadius;/*���İ뾶*/
	float tolerance = kernelTolerance;/*�����ݲ�*/
	/*�������ĳߴ�*/
	float width =gTex_now.Length.x;
	float height = gTex_now.Length.y;

	bool ok = false;
	/*��ʼ��x,y��Χ--��ת��Ϊʵ��������ת������������*/
	int start_x = max(0, pIn.Tex.x*width - delta + 1);
	int start_y = max(0, pIn.Tex.y*height - delta + 1);
	int end_x = min(pIn.Tex.x*width + delta - 1, width-1);
	int end_y = min(pIn.Tex.y*height + delta - 1, height-1);
	/*totalCount��ʾ�˷�Χ�����ص�����������(x,y)���ص�*/
	int count = 0, totalCount = (end_x - start_x + 1) * (end_y - start_y + 1) - 1;
	int thresholdCount = int(ceil(totalCount * 0.9f));
	if (delta == 0) {
		ok = true;
	}
	float value = pixel_color.r;
	/*gTex_now.Sample()���ܶ�̬���У�����չ����*/
	/*����gTex_now.SampleLevel()����Ҫ������ȷ��������mipmap���𣨶���ֱ�ӽ�����Ϊ��3���������ݣ�����˿����ڶ�̬ѭ����ʹ������*/
	for (int cx = start_x; ok == false && cx <= end_x; cx++) {
		for (int cy = start_y; ok == false && cy <= end_y; cy++) {
			if (cx == int(pIn.Tex.x*width) && cy == int(pIn.Tex.y*height)) {
				continue;
			}
			float currentValue = gTex_now.SampleLevel(gSamLinear, float2(cx/width,cy/height),0).r;
			if (abs(currentValue - value) < tolerance) {
				count++;
				if (count >= thresholdCount) {
					ok = true;
				}
			}
		}
	}
	float nowValue = pixel_color.r;/*��ǰ��ɫֵ*/
	float nowMatte = pixel_color.r;/*�����������Ľ��*/
	if (ok) {
		nowMatte = (nowValue - clipBlack) / (clipWhite - clipBlack);
		float tmp = step(clipBlack, nowValue);/*�Ƚ���Ҫ��nowValue���бȽϵ�*/
		nowMatte = (1.0 - tmp)*0.0f + tmp*nowMatte;
		tmp= step(clipWhite, nowValue);/*�Ƚ���Ҫ��nowValue���бȽϵ�*/
		nowMatte = (1.0 - tmp)*nowMatte + tmp*1.0f;
	}
	return float4(nowMatte,nowMatte,nowMatte,pixel_color.a);/*���ǯ�ƺ��ͼ��*/
}