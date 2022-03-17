#include "Header.hlsli"

float4 madd_v4_v4fl(float4 r, float4 a, float f)
{
  r[0] += a[0] * f;
  r[1] += a[1] * f;
  r[2] += a[2] * f;
  r[3] += a[3] * f;
  return r;
}

/*锐化图像*/
float4 main(VertexOut pIn) : SV_TARGET
{
	//float4 value=gTex_now.Sample(gSamLinear, pIn.Tex);/*当前点像素颜色*/
	float mval = 1.0f - sharp_value;
	/*获得纹理的尺寸*/
	float width =gTex_now.Length.x;
	float height = gTex_now.Length.y;

	int x1 = pIn.Tex.x*width - 1;
	int x2 = pIn.Tex.x*width;//当前点实际的位置
	int x3 = pIn.Tex.x*width + 1;
	int y1 = pIn.Tex.y*height - 1;
	int y2 = pIn.Tex.y*height;
	int y3 = pIn.Tex.y*height + 1;
	clamp(x1, 0, int(width) - 1);/*clamp函数小于最小值返回最小值，大于最大值返回最大值*/
	clamp(x2, 0, int(width) - 1);
	clamp(x3, 0, int(width) - 1);
	clamp(y1, 0, int(height) - 1);
	clamp(y2, 0, int(height) - 1);
	clamp(y3, 0, int(height) - 1);

	//3*3的滤波器值3x3Filter(-1, -1, -1, -1, 9, -1, -1, -1, -1);
	float4 filter1=float4(-1, -1, -1, -1);
	float median=9;
	float4 filter2=float4(-1, -1, -1, -1);
	//读取周围点像素值
	float4 output=float4(0.0,0.0,0.0,0.0);
	float4 in1=float4(0.0,0.0,0.0,0.0);
	float4 in2=float4(0.0,0.0,0.0,0.0);
	in1=gTex_now.Sample(gSamLinear, float2(x1/width,y1/height));/*(x1,y1)点像素颜色*/
	output = madd_v4_v4fl(output, in1, filter1[0]);
	in1=gTex_now.Sample(gSamLinear, float2(x2/width,y1/height));
	output = madd_v4_v4fl(output, in1, filter1[1]);
	in1=gTex_now.Sample(gSamLinear, float2(x3/width,y1/height));
	output = madd_v4_v4fl(output, in1, filter1[2]);
	in1=gTex_now.Sample(gSamLinear, float2(x1/width,y2/height));
	output = madd_v4_v4fl(output, in1, filter1[3]);
	//in2=gTex_now.Sample(gSamLinear, float2(x2/width,y2/height));
	in2=gTex_now.Sample(gSamLinear, pIn.Tex);/*当前点像素颜色*/
	output = madd_v4_v4fl(output, in2, median);
	in1=gTex_now.Sample(gSamLinear, float2(x3/width,y2/height));
	output = madd_v4_v4fl(output, in1, filter2[0]);
	in1=gTex_now.Sample(gSamLinear, float2(x1/width,y3/height));
	output = madd_v4_v4fl(output, in1, filter2[1]);
	in1=gTex_now.Sample(gSamLinear, float2(x2/width,y3/height));
	output = madd_v4_v4fl(output, in1, filter2[2]);
	in1=gTex_now.Sample(gSamLinear, float2(x3/width,y3/height));
	output = madd_v4_v4fl(output, in1, filter2[3]);

	output[0] = output[0] * sharp_value + in2[0] * mval;
	output[1] = output[1] * sharp_value + in2[1] * mval;
	output[2] = output[2] * sharp_value + in2[2] * mval;
	output[3] = output[3] * sharp_value + in2[3] * mval;

	/* Make sure we don't return negative color. */
	output[0] = max(output[0], 0.0f);
	output[1] = max(output[1], 0.0f);
	output[2] = max(output[2], 0.0f);
	output[3] = max(output[3], 0.0f);

	return output;
}