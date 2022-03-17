/*Texture2D���ͱ�����2D�������Ϣ��������ȫ�ֱ���,register(t0)��Ӧ��ʼ������0*/
Texture2D gTex : register(t0);/*��ʼ����*/
Texture2D gTex_now : register(t1);/*�м�����*/
Texture2D gTex_back : register(t2);/*��������*/
								   
/*SamplerState����ȷ��������Ӧ��ν��в�����ͬ��Ҳ��ȫ�ֱ�����register(s0)��Ӧ��ʼ������0*/
SamplerState gSamLinear : register(s0);
/*�������ֱ�������Ҫ��C++Ӧ�ò��г�ʼ���Ͱ󶨺����ʹ�á�*/

/*�����ȥ��ɫ���������Ҫ�Ĳ���*/
cbuffer ConstantBuffer_matteDespill : register(b0)/*�Ĵ���b0�ĳ���������*/
{
	float4 screen_color;/*��Ļ������ɫֵ-��ɫ*/
	float screen_balance;/*��Ļƽ��ֵ*/
	int primary_channel;/*������ɫ���ֵ��ͨ��0/1/2*/
	float despill_factor;/*�����ϵ��*/
	float despill_balance;/*�����ƽ��*/
}
/*�洢һЩ����-44+4=48+4+4=56+4=60�ֽ�*///26��ӣ�ԭ����48�����ڱ����60
cbuffer ConstantBuffer_parameter : register(b2)/*�Ĵ���b2�ĳ���������*/
{
	int size;/*Ԥģ�����õ���ģ����С*/
	/*������ǯ�ƺ�ɫ��ɫ����Ĳ���*/
	int kernelRadius;/*��Ե���İ뾶*/
	float kernelTolerance;/*��Ե�����ݲ�*/
	float clipBlack;/*ǯ�ƺ�ɫ*/
	float clipWhite;/*ǯ�ư�ɫ*/
	int size_postBlur;/*����ģ������Ҫ��ģ����С*/
	int distance;/*���͸�ʴ�ľ���*/
	int isDilateErode;/*1Ϊ���ͣ�2Ϊ��ʴ*/
	int do_subtract;/*�𻯾����Ǵ���0����С��0,����0��0��С��0��1*/
	int sizeFeather;/*�𻯾������ֵ*/
	int falloff;/*˥��*/
	float gamma;/*gammaУ��*///26���
	float brightness;/*���ȺͶԱȶ���ǿ*///26���
	float contrast;/*���ȺͶԱȶ���ǿ*///26���
	float sharp_value;/*�񻯲���*///26���
	/****************28�޸Ŀ�ʼ****************/
	float binarization_threshold;/*��ֵ������*/
	/****************28�޸Ľ���****************/
}
/*����������������ĳ���������--36�ֽ�*/
cbuffer ConstantBuffer_parameter2 : register(b3)/*�Ĵ���b3�ĳ���������*/
{
	float xBox;/*���뷽�����ֵĲ���*/
	float yBox;
	float rotationBox;
	float heightBox;
	float widthBox;
	float transparentBox;/*͸����*/
	int maskType;/*��������*/
	int isBox;/*1�����Σ�2������Բ��*/
	//int isGarbage;/*1����������2�������*/
}
/*����������������ĳ���������--36�ֽ�*/
cbuffer ConstantBuffer_parameter3 : register(b1)/*�Ĵ���b1�ĳ���������*/
{
	float xBoxCore;/*���뷽�����ֵĲ���*/
	float yBoxCore;
	float rotationBoxCore;
	float heightBoxCore;
	float widthBoxCore;
	float transparentBoxCore;/*͸����*/
	int maskTypeCore;/*��������*/
	int isBoxCore;/*1�����Σ�2������Բ��*/
	//int isGarbage;/*1����������2�������*/
}
/*�𻯵��������--384�ֽ�*/
cbuffer ConstantBuffer_parameter4 : register(b4)/*�Ĵ���b4�ĳ���������*/
{
	float4 gausstab[12];/*48*4=192�ֽ�*/
	float4 distbuf_inv[12];/*48*4=192�ֽ�*/
}

 /*������ɫ������ṹ��*/
struct VertexIn
{
	float3 pos : POSITION;
	float4 color : COLOR;
	float2 Tex : TEXCOORD;
};

/*������ɫ������ṹ��*/
struct VertexOut
{
	float4 posH : SV_POSITION;
	float4 color : COLOR;
	float2 Tex : TEXCOORD;
};