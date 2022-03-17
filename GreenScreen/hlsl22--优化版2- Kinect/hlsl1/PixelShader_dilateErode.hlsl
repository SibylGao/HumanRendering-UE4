#include "Header.hlsli"

/*膨胀腐蚀-像素着色器*/
float4 main(VertexOut pIn) : SV_TARGET
{
	/*获得纹理的尺寸*/
	float width =gTex_now.Length.x;
	float height = gTex_now.Length.y;

	/*膨胀腐蚀的最小距离值为3*/
	int scope = distance;
	float tmp;
	tmp = step(3, scope);
	scope = int((1.0 - tmp) * 3 + tmp*scope);

	/*获取边界范围--膨胀就是找此区域内的最大值-腐蚀就是找此区域内的最小值*/
	int minx = max(pIn.Tex.x*width-scope, 0);
	int miny = max(pIn.Tex.y*height-scope, 0);
	int maxx = min(pIn.Tex.x*width+scope, width);
	int maxy = min(pIn.Tex.y*height+scope, height);

	float mindist = distance * distance;

	float value;
	tmp = step(isDilateErode, 1);/*isDilateErode;1为膨胀，2为腐蚀*/
	value = (1.0 - tmp)*1.0f + tmp*0.0f;/*膨胀时value值为0，腐蚀时值为1*/

	for (int yi = miny; yi < maxy; yi++) {
		float dy = yi - pIn.Tex.y*height;
		for (int xi = minx; xi < maxx; xi++) {
			float dx = xi - pIn.Tex.x*width;
			float dis = dx * dx + dy * dy;

			float4 colorNow = gTex_now.SampleLevel(gSamLinear, float2(xi / width, yi / height),0);
			float value_dilate = max(colorNow.r, value);/*膨胀*/
			float value_erode = min(colorNow.r, value);/*腐蚀*/
			tmp = step(isDilateErode, 1);/*膨胀时step值为1，腐蚀时step值为0*/
			float valueTmp = (1.0 - tmp)*value_erode + tmp*value_dilate;

			tmp = step(dis, mindist);/*if (dis <= mindist)*/
			value = tmp*valueTmp + (1.0 - tmp)*value;
		}
	}
	return float4(value,value,value,1.0f);
}