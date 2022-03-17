#include "Header.hlsli"

/*钳制黑色白色*/
float4 main(VertexOut pIn) : SV_Target
{
	float4 pixel_color=gTex_now.Sample(gSamLinear, pIn.Tex);
	int delta = kernelRadius;/*核心半径*/
	float tolerance = kernelTolerance;/*核心容差*/
	/*获得纹理的尺寸*/
	float width =gTex_now.Length.x;
	float height = gTex_now.Length.y;

	bool ok = false;
	/*开始的x,y范围--先转换为实际坐标再转换回纹理坐标*/
	int start_x = max(0, pIn.Tex.x*width - delta + 1);
	int start_y = max(0, pIn.Tex.y*height - delta + 1);
	int end_x = min(pIn.Tex.x*width + delta - 1, width-1);
	int end_y = min(pIn.Tex.y*height + delta - 1, height-1);
	/*totalCount表示此范围内像素的数量，除了(x,y)像素点*/
	int count = 0, totalCount = (end_x - start_x + 1) * (end_y - start_y + 1) - 1;
	int thresholdCount = int(ceil(totalCount * 0.9f));
	if (delta == 0) {
		ok = true;
	}
	float value = pixel_color.r;
	/*gTex_now.Sample()不能动态运行，必须展开。*/
	/*由于gTex_now.SampleLevel()不需要导数来确定采样的mipmap级别（而是直接将其作为第3个参数传递），因此可以在动态循环中使用它。*/
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
	float nowValue = pixel_color.r;/*当前颜色值*/
	float nowMatte = pixel_color.r;/*经下述处理后的结果*/
	if (ok) {
		nowMatte = (nowValue - clipBlack) / (clipWhite - clipBlack);
		float tmp = step(clipBlack, nowValue);/*比较是要和nowValue进行比较的*/
		nowMatte = (1.0 - tmp)*0.0f + tmp*nowMatte;
		tmp= step(clipWhite, nowValue);/*比较是要和nowValue进行比较的*/
		nowMatte = (1.0 - tmp)*nowMatte + tmp*1.0f;
	}
	return float4(nowMatte,nowMatte,nowMatte,pixel_color.a);/*输出钳制后的图像*/
}