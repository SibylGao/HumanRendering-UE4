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
/*rgb_to_ycc函数的功能是将RGB颜色模式转换成YCC颜色模式*/
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
	return float4(y/255.0, cb/255.0, cr/255.0, alpha);/*归一化*/
}
/*预模糊-将此处的四个着色器整合为一个着色器*/
float4 main(VertexOut pIn) : SV_TARGET
{
	float4 pixel_color=gTex_now.Sample(gSamLinear, pIn.Tex);
	/*将RGB颜色模式转换为YCC颜色模式*/
	pixel_color=rgb_to_ycc(pixel_color.r, pixel_color.g, pixel_color.b, pixel_color.a);

	//获得纹理的尺寸
	float width =gTex_now.Length.x;
	float height = gTex_now.Length.y;

	int Xcount = 0,Ycount = 0;
	float XaverageCb = 0.0,YaverageCb = 0.0;
	float XaverageCr = 0.0,YaverageCr = 0.0;

	/*对Y轴进行模糊--先转换为实际坐标再转换为纹理坐标*/
	/*先计算出模糊的索引范围*/
	int startX = max(0, pIn.Tex.x*width - size + 1), endX = min(width, pIn.Tex.x*width + size);
	int startY = max(0, pIn.Tex.y*height - size + 1), endY = min(height, pIn.Tex.y*height + size);

	for (int nowY = startY; nowY < endY; nowY++) {
		/*第一步：首先获取(x,nowY)点的颜色值*/
		float4 pixelColor= gTex_now.SampleLevel(gSamLinear, float2(pIn.Tex.x,nowY/height),0);
		/*第二步：将RGB颜色模式转换为YCC颜色模式*/
		pixelColor=rgb_to_ycc(pixelColor.r, pixelColor.g, pixelColor.b, pixelColor.a);
		/*第三步：对(x,nowY)点进行X轴模糊*/
		for (int nowX = startX; nowX < endX; nowX++) {/*nowX是指当前点的实际坐标而不是纹理坐标*/
			/*获取当前纹理点nowX的颜色值RGBA颜色模式*/
			float4 pixelColorNow=gTex_now.SampleLevel(gSamLinear, float2(nowX/width,nowY/height),0);
			/*将RGBA颜色模式转换为YCC颜色模式*/
			float4 ColorNowX = rgb_to_ycc(pixelColorNow.r, pixelColorNow.g, pixelColorNow.b, pixelColorNow.a);
			XaverageCb += ColorNowX.g; /*cb*/
			XaverageCr += ColorNowX.b; /*cr*/
			Xcount++;
		}
		/*第四步：获取对(x,nowY)点X轴模糊以后的值*/
		float4 colorBlurX = float4(pixelColor.r,XaverageCb/Xcount,XaverageCr/Xcount,pixelColor.a);
		/*第五步：对Y轴进行模糊*/
		YaverageCb += colorBlurX.g;/*cb*/
		YaverageCr += colorBlurX.b; /*cr*/
		Ycount++;
	}
	float4 preBlurYColor=float4(pixel_color.r,YaverageCb/Ycount,YaverageCr/Ycount,pixel_color.a);/*对Y轴进行模糊后的颜色值YCC模式*/
	/*从YCC模式转换为RGB模式*/
	float4 colorRGB =ycc_to_rgb(preBlurYColor.r*255,preBlurYColor.g*255, preBlurYColor.b*255, preBlurYColor.a);
	return colorRGB;/*输出对Y模糊后的图像*/
}





///*预模糊-对Y轴进行模糊*/
//float4 main(VertexOut pIn) : SV_TARGET
//{
//	float4 pixel_color=gTex_now.Sample(gSamLinear, pIn.Tex);

//	//获得纹理的尺寸
//	float width =gTex_now.Length.x;
//	float height = gTex_now.Length.y;

//	int count = 0;
//	float averageCb = 0.0;
//	float averageCr = 0.0;
	
//	/*对Y轴进行模糊--先转换为实际坐标再转换为纹理坐标*/
//	/*先计算出模糊的索引范围*/
//	int start = max(0, pIn.Tex.y*height - size + 1), end = min(height, pIn.Tex.y*height + size);
//	for (int nowY = start; nowY < end; nowY++) {
//		/*获取nowY点的颜色值*/
//		float4 ColorNow= gTex_now.SampleLevel(gSamLinear, float2(pIn.Tex.x,nowY/height),0);
//		averageCb += ColorNow.g;/*cb*/
//		averageCr += ColorNow.b; /*cr*/
//		count++;
//	}
//	float4 preBlurYColor=float4(pixel_color.r,averageCb/count,averageCr/count,pixel_color.a);/*对Y轴进行模糊后的颜色值YCC模式*/
//	/*从YCC模式转换为RGB模式*/
//	float4 colorRGB =ycc_to_rgb(preBlurYColor.r*255,preBlurYColor.g*255, preBlurYColor.b*255, preBlurYColor.a);
//	return colorRGB;/*输出对Y模糊后的图像*/
//}





