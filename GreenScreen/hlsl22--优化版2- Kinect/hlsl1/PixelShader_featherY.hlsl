#include "Header.hlsli"

//羽化---Y轴

// 数组最大长度目前指定为48，因为羽化距离最大我设置为20，但blender是100
//全局变量
static float mGausstab[48] = (float[48])gausstab;
static float mDistbuf_inv[48] = (float[48])distbuf_inv;

float finv_test(float f, int test)
{
	return (test == 0) ? f : 1.0f - f;
}

float4 main(VertexOut pIn) : SV_TARGET
{
	float4 pixel_color = gTex_now.Sample(gSamLinear, pIn.Tex);
	int filtersize =sizeFeather;

	//获得纹理的尺寸
	float width =gTex_now.Length.x;
	float height = gTex_now.Length.y;

	int do_invert = do_subtract;
	int xmin = max(pIn.Tex.x*width, 0);
	int ymin = max(pIn.Tex.y*height-filtersize, 0);
	int ymax = min(pIn.Tex.y*height + filtersize + 1, height);

	int m_step = 1;/*quality==high*/
/* gauss */
	float alpha_accum = 0.0f;
	float multiplier_accum = 0.0f;
/* dilate */
	float value_max = finv_test(pixel_color.r, do_invert);
	float distfacinv_max = 1.0f;

	for (int ny = ymin; ny < ymax; ny += m_step){
		int index = ny - int(pIn.Tex.y*height) + filtersize;
		float2 bufferindex = float2(xmin / width, ny / height);
		float value = finv_test(gTex_now.SampleLevel(gSamLinear, bufferindex,0).r, do_invert);
		float multiplier;
/* gauss */
		multiplier = mGausstab[index];
		alpha_accum += value * multiplier;
		multiplier_accum += multiplier;
		
/* dilate - find most extreme color */
		if (value > value_max) {
			multiplier = mDistbuf_inv[index];
			value *= multiplier;
			float tmp = step(value, value_max);
			value_max = (1.0 - tmp)*value + tmp*value_max;
			distfacinv_max=(1.0 - tmp)*multiplier + tmp*distfacinv_max;
		}
	}
/* blend between the max value and gauss blue - gives nice feather */
	float value_blur = alpha_accum / multiplier_accum;
	float value_final = (value_max * distfacinv_max) + (value_blur * (1.0f - distfacinv_max));
	float result = finv_test(value_final, do_invert);

	return float4(result,result,result,pixel_color.a);
}
