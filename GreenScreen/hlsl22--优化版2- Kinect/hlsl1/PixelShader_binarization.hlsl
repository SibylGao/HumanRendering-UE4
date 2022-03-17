#include "Header.hlsli"

/*对现有蒙版matte进行二值化处理，即全部变为0和1*/
float4 main(VertexOut pIn) : SV_Target/*语义SV_TARGET表示返回值类型要和渲染目标的格式匹配。*/
{
	float4 matte_value=gTex_now.Sample(gSamLinear, pIn.Tex);/*输入纹理t2对应纹理点的像素值*/
	float value = matte_value[0];

	/*Step(a,value):如果value<a返回0即tmp=0；如果value>或=a返回1*/
	float tmp =step(binarization_threshold,value);
	value=tmp*1.0f+(1-tmp)*0.0f;/*如果大于0.9，则全部设置为1，否则全是原值*/

	matte_value[0] = value;
	matte_value[1] = value;
	matte_value[2] = value;

	return matte_value;
}