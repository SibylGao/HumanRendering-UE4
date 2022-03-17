#include "Header.hlsli"

//#define FLT_EPSILON    0x1.0p-23f
/*0x1.0p-23=0x1.0* 2^-23近似为0*/

/*增强亮度和对比度*/
/*预乘那里没有摘取*/
float4 main(VertexOut pIn) : SV_Target
{
	float4 inputValue=gTex_now.Sample(gSamLinear, pIn.Tex);/*读取输入纹理的某纹理坐标对应的颜色*/
	float4 output = float4(0.0,0.0,0.0,1.0);
	float bright = brightness / 100.0f;
	float delta = contrast / 200.0f;
	float a=0.0f;
	float b=0.0f;

	float a1 = 1.0f - delta * 2.0f;
	a1 = 1.0f / max(a1, 0.0f);/*这里将FLT_EPSILON改为0*/
	float b1 = a1 * (bright - delta);

	float delta2=delta*(-1);
	float a2 = max(1.0f - delta2 * 2.0f, 0.0f);
	float b2 = a2 * bright + delta2;

	//Step(a,x):如果x<a返回0；如果x>或=a返回1
	float tmp =step(contrast,0.0);/*if(contrast > 0)tmp=0 else tmp=1*/
	a=(1.0-tmp)*a1+tmp*a2;
	b=(1.0-tmp)*b1+tmp*b2;

	output[0] = a * inputValue[0] + b;
	output[1] = a * inputValue[1] + b;
	output[2] = a * inputValue[2] + b;
	output[3] = inputValue[3];
	return output;
}
/*if原始代码*/
	//if (contrast > 0) {
	//	a = 1.0f - delta * 2.0f;
	//	a = 1.0f / max(a, 0.0f);/*这里将FLT_EPSILON改为0*/
	//	b = a * (bright - delta);
	//}
	//else {
	//	delta *= -1;
	//	a = max(1.0f - delta * 2.0f, 0.0f);
	//	b = a * bright + delta;
	//}