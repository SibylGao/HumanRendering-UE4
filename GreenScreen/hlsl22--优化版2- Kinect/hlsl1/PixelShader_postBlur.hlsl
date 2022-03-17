#include "Header.hlsli"

/*对蒙版进行后期模糊处理--X轴和Y轴*/
float4 main(VertexOut pIn) : SV_TARGET
{
	//获得纹理的尺寸
	float width =gTex_now.Length.x;
	float height = gTex_now.Length.y;
	
	int Xcount = 0,Ycount = 0;
	float Xaverage = 0.0,Yaverage = 0.0;

	/*模糊---先转换为实际坐标再转换为纹理坐标*/
	int startX = max(0, pIn.Tex.x*width - size_postBlur + 1), endX = min(width, pIn.Tex.x*width + size_postBlur);
	int startY = max(0, pIn.Tex.y*height - size_postBlur + 1), endY = min(height, pIn.Tex.y*height + size_postBlur);

	for (int nowY = startY; nowY < endY; nowY++) {
	/*第一步：对(x,nowY)进行X轴模糊*/
		for (int nowX = startX; nowX < endX; nowX++) {
			Xaverage += gTex_now.SampleLevel(gSamLinear, float2(nowX/width, nowY/height),0).r;
			Xcount++;
		}
		/*第二步：获取(x,nowY)点X轴模糊以后的值，并对Y轴进行模糊*/
		Yaverage += (Xaverage/Xcount);
		Ycount++;
	}
	return float4(Yaverage/Ycount,Yaverage/Ycount,Yaverage/Ycount,1.0f);/*灰度图-蒙版*/
}