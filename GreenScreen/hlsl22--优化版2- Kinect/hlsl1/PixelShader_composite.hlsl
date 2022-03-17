#include "Header.hlsli"

/*合成前景背景操作*/
float4 main(VertexOut pIn) : SV_TARGET
{
	float4 pixel_color_fore=gTex_now.Sample(gSamLinear, pIn.Tex);/*前景颜色值*/
	float4 pixel_color_back=gTex_back.Sample(gSamLinear, pIn.Tex);/*背景颜色值*/
	float4 result=float4(0.0,0.0,0.0,1.0);

	float premul = pixel_color_fore.a;
	float mul = 1.0f - premul;
	result[0] = (mul * pixel_color_back[0]) + premul * pixel_color_fore[0];
	result[1] = (mul * pixel_color_back[1]) + premul * pixel_color_fore[1];
	result[2] = (mul * pixel_color_back[2]) + premul * pixel_color_fore[2];
	result[3] = (mul * pixel_color_back[3]) + 1.0 * pixel_color_fore[3];
	float tmp = step(pixel_color_fore[3], 0.0f);
	result = tmp*pixel_color_back + (1.0 - tmp)*result;
	tmp = step(1.0f, pixel_color_fore[3]);
	result = tmp*pixel_color_fore + (1.0 - tmp)*result;

	return result;
}