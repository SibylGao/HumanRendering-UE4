#include "Header.hlsli"

// 第3个像素着色器--原样输出即可
float4 main(VertexOut pIn) : SV_Target/*语义SV_TARGET表示返回值类型要和渲染目标的格式匹配。*/
{
	float4 pixel_color=gTex_now.Sample(gSamLinear, pIn.Tex);/*输入纹理t2对应纹理点的像素值*/
	return pixel_color;
}