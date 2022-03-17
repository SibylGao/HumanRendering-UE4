#include "Header.hlsli"

/*���ɰ���к���ģ������--X���Y��*/
float4 main(VertexOut pIn) : SV_TARGET
{
	//�������ĳߴ�
	float width =gTex_now.Length.x;
	float height = gTex_now.Length.y;
	
	int Xcount = 0,Ycount = 0;
	float Xaverage = 0.0,Yaverage = 0.0;

	/*ģ��---��ת��Ϊʵ��������ת��Ϊ��������*/
	int startX = max(0, pIn.Tex.x*width - size_postBlur + 1), endX = min(width, pIn.Tex.x*width + size_postBlur);
	int startY = max(0, pIn.Tex.y*height - size_postBlur + 1), endY = min(height, pIn.Tex.y*height + size_postBlur);

	for (int nowY = startY; nowY < endY; nowY++) {
	/*��һ������(x,nowY)����X��ģ��*/
		for (int nowX = startX; nowX < endX; nowX++) {
			Xaverage += gTex_now.SampleLevel(gSamLinear, float2(nowX/width, nowY/height),0).r;
			Xcount++;
		}
		/*�ڶ�������ȡ(x,nowY)��X��ģ���Ժ��ֵ������Y�����ģ��*/
		Yaverage += (Xaverage/Xcount);
		Ycount++;
	}
	return float4(Yaverage/Ycount,Yaverage/Ycount,Yaverage/Ycount,1.0f);/*�Ҷ�ͼ-�ɰ�*/
}