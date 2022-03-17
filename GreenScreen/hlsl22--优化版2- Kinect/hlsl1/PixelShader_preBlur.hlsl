#include "Header.hlsli"
/*YCC-RGB*/
float4 ycc_to_rgb(float y, float cb, float cr, float alpha)
{
	float r = 128.0f, g = 128.0f, b = 128.0f;
	r = 1.164f * (y - 16.0f) + 1.793f * (cr - 128.0f);
	g = 1.164f * (y - 16.0f) - 0.534f * (cr - 128.0f) - 0.213f * (cb - 128.0f);
	b = 1.164f * (y - 16.0f) + 2.115f * (cb - 128.0f);
	return float4(r / 255.0f, g / 255.0f, b / 255.0f, alpha);
}
/*rgb_to_ycc�����Ĺ����ǽ�RGB��ɫģʽת����YCC��ɫģʽ*/
float4 rgb_to_ycc(float r, float g, float b,float alpha)
{
	float sr, sg, sb;
	float y = 128.0f, cr = 128.0f, cb = 128.0f;
	sr = 255.0f * r;
	sg = 255.0f * g;
	sb = 255.0f * b;
	y = (0.183f * sr) + (0.614f * sg) + (0.062f * sb) + 16.0f;
	cb = (-0.101f * sr) - (0.338f * sg) + (0.439f * sb) + 128.0f;
	cr = (0.439f * sr) - (0.399f * sg) - (0.040f * sb) + 128.0f;
	return float4(y/255.0, cb/255.0, cr/255.0, alpha);/*��һ��*/
}
/*Ԥģ��-���˴����ĸ���ɫ������Ϊһ����ɫ��*/
float4 main(VertexOut pIn) : SV_TARGET
{
	float4 pixel_color=gTex_now.Sample(gSamLinear, pIn.Tex);
	/*��RGB��ɫģʽת��ΪYCC��ɫģʽ*/
	pixel_color=rgb_to_ycc(pixel_color.r, pixel_color.g, pixel_color.b, pixel_color.a);

	//�������ĳߴ�
	float width =gTex_now.Length.x;
	float height = gTex_now.Length.y;

	int Xcount = 0,Ycount = 0;
	float XaverageCb = 0.0,YaverageCb = 0.0;
	float XaverageCr = 0.0,YaverageCr = 0.0;

	/*��Y�����ģ��--��ת��Ϊʵ��������ת��Ϊ��������*/
	/*�ȼ����ģ����������Χ*/
	int startX = max(0, pIn.Tex.x*width - size + 1), endX = min(width, pIn.Tex.x*width + size);
	int startY = max(0, pIn.Tex.y*height - size + 1), endY = min(height, pIn.Tex.y*height + size);

	for (int nowY = startY; nowY < endY; nowY++) {
		/*��һ�������Ȼ�ȡ(x,nowY)�����ɫֵ*/
		float4 pixelColor= gTex_now.SampleLevel(gSamLinear, float2(pIn.Tex.x,nowY/height),0);
		/*�ڶ�������RGB��ɫģʽת��ΪYCC��ɫģʽ*/
		pixelColor=rgb_to_ycc(pixelColor.r, pixelColor.g, pixelColor.b, pixelColor.a);
		/*����������(x,nowY)�����X��ģ��*/
		for (int nowX = startX; nowX < endX; nowX++) {/*nowX��ָ��ǰ���ʵ�������������������*/
			/*��ȡ��ǰ�����nowX����ɫֵRGBA��ɫģʽ*/
			float4 pixelColorNow=gTex_now.SampleLevel(gSamLinear, float2(nowX/width,nowY/height),0);
			/*��RGBA��ɫģʽת��ΪYCC��ɫģʽ*/
			float4 ColorNowX = rgb_to_ycc(pixelColorNow.r, pixelColorNow.g, pixelColorNow.b, pixelColorNow.a);
			XaverageCb += ColorNowX.g; /*cb*/
			XaverageCr += ColorNowX.b; /*cr*/
			Xcount++;
		}
		/*���Ĳ�����ȡ��(x,nowY)��X��ģ���Ժ��ֵ*/
		float4 colorBlurX = float4(pixelColor.r,XaverageCb/Xcount,XaverageCr/Xcount,pixelColor.a);
		/*���岽����Y�����ģ��*/
		YaverageCb += colorBlurX.g;/*cb*/
		YaverageCr += colorBlurX.b; /*cr*/
		Ycount++;
	}
	float4 preBlurYColor=float4(pixel_color.r,YaverageCb/Ycount,YaverageCr/Ycount,pixel_color.a);/*��Y�����ģ�������ɫֵYCCģʽ*/
	/*��YCCģʽת��ΪRGBģʽ*/
	float4 colorRGB =ycc_to_rgb(preBlurYColor.r*255,preBlurYColor.g*255, preBlurYColor.b*255, preBlurYColor.a);
	return colorRGB;/*�����Yģ�����ͼ��*/
}





///*Ԥģ��-��Y�����ģ��*/
//float4 main(VertexOut pIn) : SV_TARGET
//{
//	float4 pixel_color=gTex_now.Sample(gSamLinear, pIn.Tex);

//	//�������ĳߴ�
//	float width =gTex_now.Length.x;
//	float height = gTex_now.Length.y;

//	int count = 0;
//	float averageCb = 0.0;
//	float averageCr = 0.0;
	
//	/*��Y�����ģ��--��ת��Ϊʵ��������ת��Ϊ��������*/
//	/*�ȼ����ģ����������Χ*/
//	int start = max(0, pIn.Tex.y*height - size + 1), end = min(height, pIn.Tex.y*height + size);
//	for (int nowY = start; nowY < end; nowY++) {
//		/*��ȡnowY�����ɫֵ*/
//		float4 ColorNow= gTex_now.SampleLevel(gSamLinear, float2(pIn.Tex.x,nowY/height),0);
//		averageCb += ColorNow.g;/*cb*/
//		averageCr += ColorNow.b; /*cr*/
//		count++;
//	}
//	float4 preBlurYColor=float4(pixel_color.r,averageCb/count,averageCr/count,pixel_color.a);/*��Y�����ģ�������ɫֵYCCģʽ*/
//	/*��YCCģʽת��ΪRGBģʽ*/
//	float4 colorRGB =ycc_to_rgb(preBlurYColor.r*255,preBlurYColor.g*255, preBlurYColor.b*255, preBlurYColor.a);
//	return colorRGB;/*�����Yģ�����ͼ��*/
//}





