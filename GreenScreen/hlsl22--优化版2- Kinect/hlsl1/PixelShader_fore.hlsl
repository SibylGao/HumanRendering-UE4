#include "Header.hlsli"

//输出抠像后的前景并去除绿色溢出
float4 PS(VertexOut pIn) : SV_Target/*语义SV_TARGET表示返回值类型要和渲染目标的格式匹配。*/
{
	/*第一步：获取前景值*/
	float4 pixel_color=gTex_now.Sample(gSamLinear, pIn.Tex);/*蒙版对应纹理点的像素值*/
	float4 pixel_color2=gTex.Sample(gSamLinear, pIn.Tex);/*原图对应纹理点的像素值*/
	float result[4];/*获取到的前景颜色值*/
	result[0]=pixel_color2.r;
	result[1]=pixel_color2.g;
	result[2]=pixel_color2.b;
	result[3]=pixel_color.r;
	/*第二步：去除色彩溢出*/
	int other_1 = (primary_channel + 1) % 3;
	int other_2 = (primary_channel + 2) % 3;
	int min_channel = min(other_1, other_2);
	int max_channel = max(other_1, other_2);
	float average_value, amount; 
	average_value = despill_balance * result[min_channel] + (1.0f - despill_balance) * result[max_channel];
	amount = (result[primary_channel] - average_value);
	float amount_despill = despill_factor * amount;
	
	float tmp = step(amount_despill,0.0);/*amount_despill > 0.0f则去除绿色溢出*/
	result[primary_channel] = tmp*result[primary_channel] + (1.0-tmp)*(result[primary_channel] - amount_despill);

	return float4(result[0],result[1],result[2],result[3]);/*输出去除绿溢后的前景图*/
}
