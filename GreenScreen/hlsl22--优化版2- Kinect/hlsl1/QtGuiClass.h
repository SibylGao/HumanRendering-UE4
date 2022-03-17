#pragma once
#include "sockettcp.h"

/*ȫ��������ָ��*/
#include <QWidget>
#include "ui_QtGuiClass.h"

#include "d3d11.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>/*����DirectX*/
#include"WICTextureLoader.h"/*���ڶ�ȡWIC����*/
#include"ScreenGrab.h"/*������Ļ��������������õ�����������ļ�*/
#include<wincodec.h>/*ʹ������һЩ����WIC�ؼ���GUID*/
#include<opencv2/opencv.hpp>
#include <wrl/client.h>/*����ָ��*/

#include "myKinect.h"

using namespace std;
using namespace cv;
// �������Ҫ���õĿ�
/*�������д���Ϳ���ʹ��D3DReadFileToBlob����*/
#pragma comment(lib,"d3dcompiler.lib")


class QtGuiClass : public QWidget
{
	Q_OBJECT

public:
	QtGuiClass(string filename,bool video,QWidget *parent = Q_NULLPTR);
	QtGuiClass(bool video,bool ifKinect, QWidget *parent = Q_NULLPTR);//���ع��캯��
	~QtGuiClass();

private:
	Ui::QtGuiClass ui;

public slots:/*�ۺ���*/
	void updateFrame();/*������һ֡*/
	void updateFrame_Kinect();/*������һ֡*/

public:
	/*��ʹ��QtĬ�ϵĻ�������*/
	QPaintEngine *paintEngine() const { return nullptr; }
	HRESULT hr;//�������Ϊ�����ַ���ֵ��bool��������������������������ֵ���ص���ʮ�����Ƶ�0x0000,�������������ʮ�����Ʊ�ʶ����Ӧ�������Ͽ����ѵ�

	float bgcolor[4];/*RGBA*/
	
	// ʹ��ģ�����(C++11)��������
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ComPtr<ID3D11Device> m_d3dDevice; //����D3D��Ҫ�ӿ�
	ComPtr<ID3D11DeviceContext> m_d3dDevContext;  //����D3D����������
	ComPtr<IDXGISwapChain> m_swapChain; //������

	ComPtr<ID3D11Texture2D> m_depthStencilBuffer; //��Ⱥ�ģ�建������Buffer���棬�������Ϊһ�Ŵ���ͼƬ
	ComPtr<ID3D11DepthStencilView> m_depthStencilView;//���ģ�建������������
	ComPtr<ID3D11RenderTargetView> m_renderTargetView;//���ƴ�������--��ȾĿ����ͼ
	D3D11_BLEND_DESC blendDesc;
	ComPtr<ID3D11BlendState> BSAlphaToCoverage;	// ���״̬��Alpha-To-Coverage

	ComPtr<ID3D11RasterizerState> m_rasterizeState;//״̬

	ComPtr<ID3D11Texture2D> backBuffer;/*�󱸻�����--һ��2D����*/

	D3D11_VIEWPORT viewport;       //�������������ӿ�

	/*��ɫ������ɵ��ֽ��뻺�嵽������*/
	ComPtr<ID3DBlob> m_vertexBlob;
	ComPtr<ID3DBlob> m_pixelBlob_matte;
	ComPtr<ID3DBlob> m_pixelBlob_fore;
	ComPtr<ID3DBlob> m_pixelBlob_output;
	ComPtr<ID3DBlob> m_pixelBlob_preBlur;
	ComPtr<ID3DBlob> m_pixelBlob_clip;
	ComPtr<ID3DBlob> m_pixelBlob_postBlur;
	ComPtr<ID3DBlob> m_pixelBlob_dilateErode;
	ComPtr<ID3DBlob> m_pixelBlob_featherX;
	ComPtr<ID3DBlob> m_pixelBlob_featherY;
	ComPtr<ID3DBlob> m_pixelBlob_GarbageMatte;
	ComPtr<ID3DBlob> m_pixelBlob_CoreMatte;
	ComPtr<ID3DBlob> m_pixelBlob_Composite;
	ComPtr<ID3DBlob> m_pixelBlob_gamma;//26���
	ComPtr<ID3DBlob> m_pixelBlob_brightANDcontrast;//26���
	ComPtr<ID3DBlob> m_pixelBlob_sharp;//26���
	/****************28�޸Ŀ�ʼ****************/
	ComPtr<ID3DBlob> m_pixelBlob_binarization;
	/****************28�޸Ľ���****************/

	//��ɫ������
	ComPtr<ID3D11VertexShader> m_vertexShader;/*������ɫ��*/
	ComPtr<ID3D11PixelShader> m_pixelShader_matte;/*��1��������ɫ��-����*/
	ComPtr<ID3D11PixelShader> m_pixelShader_fore;/*��2��������ɫ��-���ǰ��*/
	ComPtr<ID3D11PixelShader> m_pixelShader_output;/*��3��������ɫ��-ԭ�����*/
	ComPtr<ID3D11PixelShader> m_pixelShader_preBlur;/*��8��������ɫ��-Ԥģ��Y��*/
	ComPtr<ID3D11PixelShader> m_pixelShader_clip;/*��9��������ɫ��-ǯ�ƺ�ɫ��ɫ*/
	ComPtr<ID3D11PixelShader> m_pixelShader_postBlur;/*��11��������ɫ��-����ģ��Y��*/
	ComPtr<ID3D11PixelShader> m_pixelShader_dilateErode;/*��12��������ɫ��-���͸�ʴ*/
	ComPtr<ID3D11PixelShader> m_pixelShader_featherX;/*��13��������ɫ��-��X��*/
	ComPtr<ID3D11PixelShader> m_pixelShader_featherY;/*��14��������ɫ��-��Y��*/
	ComPtr<ID3D11PixelShader> m_pixelShader_GarbageMatte;/*��15��������ɫ��-������������*/
	ComPtr<ID3D11PixelShader> m_pixelShader_CoreMatte;/*��15��������ɫ��-������������*/
	ComPtr<ID3D11PixelShader> m_pixelShader_Composite;/*��16��������ɫ��-�ϳ�*/
	ComPtr<ID3D11PixelShader> m_pixelShader_gamma;/*������ɫ��-gammaУ��*///26���
	ComPtr<ID3D11PixelShader> m_pixelShader_brightANDcontrast;/*������ɫ��-���ȺͶԱȶ���ǿ*///26���
	ComPtr<ID3D11PixelShader> m_pixelShader_sharp;/*������ɫ��-��*///26���
		/****************28�޸Ŀ�ʼ****************/
	ComPtr<ID3D11PixelShader> m_pixelShader_binarization;
	/****************28�޸Ľ���****************/

	/*��ɫ���������벼�ֶ�Ӧ��C++�ṹ��*/
	struct VertexPosColor
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 tex;
		static const D3D11_INPUT_ELEMENT_DESC inputLayout[3];
	};
	/*�����ȥ����ɫ�������Ĳ�����Ӧ�ĳ���������*/
	struct MatteDespillCB/*32�ֽ�*/
	{
		DirectX::XMFLOAT4 screen_color;/*��Ļ������ɫֵ-��ɫ*/
		float screen_balance;/*��Ļƽ��ֵ*/
		int primary_channel;/*������ɫ���ֵ��ͨ��0/1/2*/
		float despill_factor;/*�����ϵ��*/
		float despill_balance;/*�����ƽ��*/
	};

	/*һЩ������ɫ����Ӧ�ĳ�������*/
	struct constantBuffer_parameter/*44�ֽ�*/
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
		int do_subtract;/*�𻯾����Ǵ���0����С��0*/
		int sizeFeather;/*�𻯾������ֵ*/
		int falloff;/*˥��*/
		float gamma;/*gammaУ��*///26���
		float brightness;/*���ȺͶԱȶ���ǿ*///26���
		float contrast;/*���ȺͶԱȶ���ǿ*///26���
		float sharp_value;/*��*///26���
		/****************28�޸Ŀ�ʼ****************/
		float binarization_threshold;/*��ֵ������*/
		/****************28�޸Ľ���****************/
	};
	/*�������������������ݶ�Ӧ�Ľṹ��*/
	struct cBufferGarbageCore {/*36�ֽ�*/
		float xBox;/*���뷽�����ֵĲ���*/
		float yBox;
		float rotationBox;
		float heightBox;
		float widthBox;
		float transparentBox;/*͸����*/
		int maskType;/*��������*/
		int isBox;/*1�����Σ�2������Բ��*/
		//int isGarbage;/*1����������2�������*/
	};
	/*�������������*/
	struct Feather {
		float gausstab[48];
		float distbuf_inv[48];
	};
	ComPtr<ID3D11Buffer> CBufferFeather;// ����������--��
	Feather featherParameter;
	
	ComPtr<ID3D11InputLayout> m_pVertexLayout;// �������벼��
	ComPtr<ID3D11Buffer> m_pVertexBuffer;// ���㻺����
	/*3������������*/
	ComPtr<ID3D11Buffer> CBufferMatteDespill;// ����������--�����ȥ��ɫ������������
	MatteDespillCB matteDespillCB;/*�����ȥ��ɫ���������Ҫ�Ĳ���*/
	ComPtr<ID3D11Buffer> CBufferOtherParameter;/*����������*/
	constantBuffer_parameter m_parameter;/*ǯ�ƺ�ɫ��ɫ������Ĳ���*/
	ComPtr<ID3D11Buffer> GarbageCBuffer;/*����������--�洢�������ֵĲ���*/
	cBufferGarbageCore parameterGarbage;/*��������*/
	ComPtr<ID3D11Buffer> CoreCBuffer;/*����������--�洢�������ֵĲ���*/
	cBufferGarbageCore parameterCore;/*��������*/

	ComPtr<ID3D11SamplerState> m_pSamplerState;/*������״̬*/

	D3D11_TEXTURE2D_DESC texDESC;/*2D��������--CPU���ɶ�д*/
	D3D11_TEXTURE2D_DESC texDESC_cpu;/*2D��������--CPU�ɶ�д*/
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;/*��ɫ����Դ��ͼ������*/
	D3D11_SUBRESOURCE_DATA sd;/*����������Դ������--��ʼ��ʱʹ��*/

	ComPtr<ID3D11Texture2D> backBuffer_save;/*��ȡ�󱸻���������*/
	/*��ɫ����Դ��ͼ*/
	ComPtr<ID3D11ShaderResourceView> nowTexSRV;/*��ɫ����Դ��ͼ-��Ӧ�ڵ�ǰ�м�����*/
	ComPtr<ID3D11ShaderResourceView> texInputSRV;/*��ɫ����Դ��ͼ-��Ӧ������������ͼ*/
	ComPtr<ID3D11ShaderResourceView> texBackSRV;/*��ɫ����Դ��ͼ-��Ӧ�ڱ���������ͼ*/

	ComPtr<ID3D11Texture2D> nowTex;/*����ָ�룬ָ��ǰ���м�����*/
	ComPtr<ID3D11Texture2D> tex_input;/*��������*/
	ComPtr<ID3D11Texture2D> tex_matte;/*�м�����1-�洢�ɰ�*/
	ComPtr<ID3D11Texture2D> tex_fore;/*�м�����2-�洢ǰ��*/
	ComPtr<ID3D11Texture2D> tex_preBlur;
	ComPtr<ID3D11Texture2D> tex_clip;
	ComPtr<ID3D11Texture2D> tex_postBlur;
	ComPtr<ID3D11Texture2D> tex_dilateErode;
	ComPtr<ID3D11Texture2D> tex_featherX;
	ComPtr<ID3D11Texture2D> tex_featherY;
	ComPtr<ID3D11Texture2D> tex_Garbage;
	ComPtr<ID3D11Texture2D> tex_Core;
	ComPtr<ID3D11Texture2D> tex_Composite;
	ComPtr<ID3D11Texture2D> tex_back;/*��������*/
	ComPtr<ID3D11Texture2D> tex_gamma;/*gammaУ���������*///26���
	ComPtr<ID3D11Texture2D> tex_brightANDcontrast;/*���ȺͶԱȶ���ǿ*///26���
	ComPtr<ID3D11Texture2D> tex_sharp;/*��*///26���
	/****************28�޸Ŀ�ʼ****************/
	ComPtr<ID3D11Texture2D> tex_binarization;
	/****************28�޸Ľ���****************/

	QTimer* m_pTimer = nullptr;/*��ʱ��--����������Ƶ��ÿһ֡�������ݣ�Ҳ����pixels*/
	VideoCapture cap;/*��ȡ��Ƶ---�õ���OpenCV*/
	VideoWriter writer;/*д����Ƶ*/
	Mat src;/*��ʱ�洢��ȡ��֡*/
	//unsigned char * colorFlow;/*��ʱ�洢��ȡ��֡��Kinect��õĲ�ɫͼ */
	//unsigned short * depthFlow;/*��ʱ�洢��ȡ��֡��Kinect��õ����ͼ*/
	unsigned char *  colorFlow = (unsigned char*)malloc(sizeof(unsigned char) * colorwidth * colorheight * 3);
	unsigned short *  depthFlow = (unsigned short *)malloc(sizeof(unsigned short)* colorwidth*colorheight);
	//Mat src_depth;/*��ʱ�洢��ȡ��֡��Kinect��õ����ͼ*/
	//Mat src_color;/*��ʱ�洢��ȡ��֡��Kinect��õĲ�ɫͼ */

	bool video;
	bool ifKinect;
	/*һЩ����*/
	wchar_t *file_name;/*ͼƬ�ĵ�ַ*/
	uint32_t *pixels;/*ͼƬ����ֵ*/
	uchar * imgData;/*�洢��ǰ֡��������������*/
	int width, height;/*ͼƬ���*/

	string filename_bg;/*����ͼ��·��*/
	bool composite;
	int flag;/*flag=0������ɰ棬flag=1�����ǰ��*/
	float ScreenBalanceValue;/*��Ļƽ��ֵ*/
	float ScreenColorValue[4];/*��Ļ������ɫֵ*/
	float DespillFactor;/*�����ϵ��*/
	float DespillBalance;/*�����ƽ��*/
	int preBlurValue;/*Ԥģ��ֵ*/
	/*ǯ�ƺ�ɫ��ɫ�������*/
	int kernelRadius;
	float kernelTolerance;
	float clipBlack;
	float clipWhite;
	int blur_post;/*����ģ��*/
	int distance;/*���͸�ʴ����*/
	int feather_distance;/*�𻯾���*/
	int feather_falloff;/*��˥��*/
	/*�������ֺͺ���������������*/
	bool isGarbageMatte;/*��������*/
	bool isCoreMatte;/*��������*/
	bool isBoxMask_Garbage;/*��������*/
	//bool isEllipseMask;/*��Բ����*/
	bool isBoxMask_Core;/*��������*/
	//bool isEllipseMask2;/*��Բ����*/
	int maskType_Garbage;/*��������-��������*/
	int maskType_Core;/*��������-��������*/
	float x_Garbage, y_Garbage, rotation_Garbage, width_Garbage, height_Garbage, transparent_Garbage;
	float x_Core, y_Core, rotation_Core, width_Core, height_Core, transparent_Core;
	float gamma;/*gammaУ��*///26���
	float brightness;/*���ȺͶԱȶ���ǿ*///26���
	float contrast;/*���ȺͶԱȶ���ǿ*///26���
	float sharp_value;/*��*///26���
	/****************28�޸Ŀ�ʼ****************/
	float binarization_threshold;/*��ֵ������*/
	bool isBinary;/*�Ƿ���ж�ֵ������*/
	/****************28�޸Ľ���****************/

	int count;
	int count2;
	LARGE_INTEGER dwStart = { 0 };/*����ʼʱ��-΢�뼶*/
	LARGE_INTEGER dwStop = { 0 };/*�������ʱ��-΢�뼶*/
	double pcFreqall;/*ʱ��Ƶ��*/
	double readAllFrameTime;/*��֡ʱ��*/
	double writeAllFrameTime;/*д֡ʱ��*/
	double myMatteTime;/*�����õĿ���ʱ��*/
public:
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent *event);

	void InitD3D();/*��ʼ��DirectX�������*/
	void InitEffect();/*��ʼ����ɫ��*/
	void InitResource();/*��ʼ�����㻺������������ɫ����Դ*/
	//void ResizeD3D();/*�����ڴ�С�ı���Ҫˢ������Ķ���*/

	void setupMatte();/*����ɰ�*/
	void setupForeImage();/*���ǰ��ͼ��*/
	void setupPreBlur();/*����Ԥģ��*/
	void setupClip();/*����ǯ�ƺ�ɫ��ɫ*/
	void setupPostBlur();/*���ú���ģ��*/
	void setupDilateErode();/*�������͸�ʴ*/
	void setupFeather();/*������*/
	void setupFeatherXorY(int flag);/*����X��Y�����*/
	void setupGarbageMatte();/*������������*/
	void setupCoreMatte();/*���ú�������*/
	void setupComposite();/*�ϳ�ǰ������*/
	void setupGamma();/*��ǰ������gammaУ��*///26���
	void setupbrightANDcontrast();/*���ȺͶԱȶ���ǿ*///26���
	void setupSharp();/*��*///26���
	/****************28�޸Ŀ�ʼ****************/
	void setupBinarization();
	/****************28�޸Ľ���****************/

	void setComposite(bool composite,string filename_back);/*���ñ�����ɫ����Դ��ͼ*/
	void setConstantBuffers();/*���ó�����������ֵ*/
	void make_gausstab(float rad, const int size);
	void make_dist_fac_inverse(float rad, int size, int falloff);
};
