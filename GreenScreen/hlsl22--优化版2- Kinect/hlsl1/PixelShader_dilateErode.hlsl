#include "Header.hlsli"

/*���͸�ʴ-������ɫ��*/
float4 main(VertexOut pIn) : SV_TARGET
{
	/*�������ĳߴ�*/
	float width =gTex_now.Length.x;
	float height = gTex_now.Length.y;

	/*���͸�ʴ����С����ֵΪ3*/
	int scope = distance;
	float tmp;
	tmp = step(3, scope);
	scope = int((1.0 - tmp) * 3 + tmp*scope);

	/*��ȡ�߽緶Χ--���;����Ҵ������ڵ����ֵ-��ʴ�����Ҵ������ڵ���Сֵ*/
	int minx = max(pIn.Tex.x*width-scope, 0);
	int miny = max(pIn.Tex.y*height-scope, 0);
	int maxx = min(pIn.Tex.x*width+scope, width);
	int maxy = min(pIn.Tex.y*height+scope, height);

	float mindist = distance * distance;

	float value;
	tmp = step(isDilateErode, 1);/*isDilateErode;1Ϊ���ͣ�2Ϊ��ʴ*/
	value = (1.0 - tmp)*1.0f + tmp*0.0f;/*����ʱvalueֵΪ0����ʴʱֵΪ1*/

	for (int yi = miny; yi < maxy; yi++) {
		float dy = yi - pIn.Tex.y*height;
		for (int xi = minx; xi < maxx; xi++) {
			float dx = xi - pIn.Tex.x*width;
			float dis = dx * dx + dy * dy;

			float4 colorNow = gTex_now.SampleLevel(gSamLinear, float2(xi / width, yi / height),0);
			float value_dilate = max(colorNow.r, value);/*����*/
			float value_erode = min(colorNow.r, value);/*��ʴ*/
			tmp = step(isDilateErode, 1);/*����ʱstepֵΪ1����ʴʱstepֵΪ0*/
			float valueTmp = (1.0 - tmp)*value_erode + tmp*value_dilate;

			tmp = step(dis, mindist);/*if (dis <= mindist)*/
			value = tmp*valueTmp + (1.0 - tmp)*value;
		}
	}
	return float4(value,value,value,1.0f);
}