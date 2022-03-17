#include "Header.hlsli"

//获得像素饱和度--获取像素之间的差异--绿色主要通道与其他通道之间的颜色差异
float get_pixel_saturation(float4 pixelColor,int primary_channel)
{
	int other_1 = (primary_channel + 1) % 3;
	int other_2 = (primary_channel + 2) % 3;
	int min_channel = min(other_1, other_2);
	int max_channel = max(other_1, other_2);
	float val = screen_balance * pixelColor[min_channel] + (1.0f - screen_balance) * pixelColor[max_channel];
	return (pixelColor[primary_channel] - val) * abs(1.0f - val);
}
//找到三个数的最小值
float min_fff(float a, float b, float c) {
	return min(min(a, b), c);
}
// 抠像--像素着色器
float4 PS(VertexOut pIn) : SV_Target/*语义SV_TARGET表示返回值类型要和渲染目标的格式匹配。*/
{
	float4 pixel_color=gTex_now.Sample(gSamLinear, pIn.Tex);/*读取输入纹理的某纹理坐标对应的颜色*/
	/*没有过度曝光的情况下*/
	/*获取当前颜色以及背景颜色的饱和度*/
	float saturation = get_pixel_saturation(pixel_color,primary_channel);
	float screen_saturation = get_pixel_saturation(screen_color, primary_channel);
	float distance = 1.0f - saturation / screen_saturation;
	float matte_value = distance;/*否则为边缘*/

	float tmp1 = step(0.0,saturation);/*若saturation < 0则表示前景，matte值为1*/
	matte_value=(1.0 - tmp1)*1.0f + tmp1 * matte_value;

	float tmp2 = step(screen_saturation, saturation);/*saturation >= screen_saturation则表示背景，值为0*/
	matte_value=(1.0 - tmp2)*matte_value + tmp2 * 0.0f;

	/*若是出现了过度曝光的情况*/
	/*过度曝光不会发生在屏幕本身上，通常发生在镜头中的光源上，这需要单独检查，
		因为饱和度和衰减的计算是基于像素没有过度曝光的事实。*/
	float min_pixel_color = min_fff(pixel_color[0], pixel_color[1], pixel_color[2]);
	float tmp3=step(min_pixel_color , 1.0f);/*如果min_pixel_color > 1.0f则出现了过度曝光*/
	matte_value=tmp3*matte_value+(1-tmp3)*1.0f;

	return float4(matte_value,matte_value,matte_value,1.0);/*输出蒙版*/
}
//float4 pixel_color=gTex.Sample(gSamLinear, pIn.Tex);/*读取输入纹理的某纹理坐标对应的颜色*/
//float matte_value = (1.0 - tmp1)*1.0f + tmp1*(tmp2*0.0f + (1.0 - tmp2)*distance);
