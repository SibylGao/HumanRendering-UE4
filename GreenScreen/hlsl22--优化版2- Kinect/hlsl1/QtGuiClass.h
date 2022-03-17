#pragma once
#include "sockettcp.h"

/*全部用智能指针*/
#include <QWidget>
#include "ui_QtGuiClass.h"

#include "d3d11.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>/*用于DirectX*/
#include"WICTextureLoader.h"/*用于读取WIC纹理*/
#include"ScreenGrab.h"/*用于屏幕截屏输出或将制作好的纹理输出到文件*/
#include<wincodec.h>/*使用里面一些关于WIC控件的GUID*/
#include<opencv2/opencv.hpp>
#include <wrl/client.h>/*智能指针*/

#include "myKinect.h"

using namespace std;
using namespace cv;
// 添加所有要引用的库
/*加上这行代码就可以使用D3DReadFileToBlob函数*/
#pragma comment(lib,"d3dcompiler.lib")


class QtGuiClass : public QWidget
{
	Q_OBJECT

public:
	QtGuiClass(string filename,bool video,QWidget *parent = Q_NULLPTR);
	QtGuiClass(bool video,bool ifKinect, QWidget *parent = Q_NULLPTR);//重载构造函数
	~QtGuiClass();

private:
	Ui::QtGuiClass ui;

public slots:/*槽函数*/
	void updateFrame();/*更新下一帧*/
	void updateFrame_Kinect();/*更新下一帧*/

public:
	/*不使用Qt默认的绘制引擎*/
	QPaintEngine *paintEngine() const { return nullptr; }
	HRESULT hr;//可以理解为带各种返回值的bool变量，如果函数运行正常，这个值返回的是十六进制的0x0000,其余错误代码均是十六进制标识，对应错误网上可以搜到

	float bgcolor[4];/*RGBA*/
	
	// 使用模板别名(C++11)简化类型名
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ComPtr<ID3D11Device> m_d3dDevice; //创建D3D主要接口
	ComPtr<ID3D11DeviceContext> m_d3dDevContext;  //创建D3D绘制上下文
	ComPtr<IDXGISwapChain> m_swapChain; //交换链

	ComPtr<ID3D11Texture2D> m_depthStencilBuffer; //深度和模板缓冲区的Buffer缓存，可以理解为一张窗口图片
	ComPtr<ID3D11DepthStencilView> m_depthStencilView;//深度模板缓冲区窗口描述
	ComPtr<ID3D11RenderTargetView> m_renderTargetView;//绘制窗口描述--渲染目标视图
	D3D11_BLEND_DESC blendDesc;
	ComPtr<ID3D11BlendState> BSAlphaToCoverage;	// 混合状态：Alpha-To-Coverage

	ComPtr<ID3D11RasterizerState> m_rasterizeState;//状态

	ComPtr<ID3D11Texture2D> backBuffer;/*后备缓冲区--一张2D纹理*/

	D3D11_VIEWPORT viewport;       //我们所看到的视口

	/*着色器编译成的字节码缓冲到这里面*/
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
	ComPtr<ID3DBlob> m_pixelBlob_gamma;//26添加
	ComPtr<ID3DBlob> m_pixelBlob_brightANDcontrast;//26添加
	ComPtr<ID3DBlob> m_pixelBlob_sharp;//26添加
	/****************28修改开始****************/
	ComPtr<ID3DBlob> m_pixelBlob_binarization;
	/****************28修改结束****************/

	//着色器数据
	ComPtr<ID3D11VertexShader> m_vertexShader;/*顶点着色器*/
	ComPtr<ID3D11PixelShader> m_pixelShader_matte;/*第1个像素着色器-抠像*/
	ComPtr<ID3D11PixelShader> m_pixelShader_fore;/*第2个像素着色器-输出前景*/
	ComPtr<ID3D11PixelShader> m_pixelShader_output;/*第3个像素着色器-原样输出*/
	ComPtr<ID3D11PixelShader> m_pixelShader_preBlur;/*第8个像素着色器-预模糊Y轴*/
	ComPtr<ID3D11PixelShader> m_pixelShader_clip;/*第9个像素着色器-钳制黑色白色*/
	ComPtr<ID3D11PixelShader> m_pixelShader_postBlur;/*第11个像素着色器-后期模糊Y轴*/
	ComPtr<ID3D11PixelShader> m_pixelShader_dilateErode;/*第12个像素着色器-膨胀腐蚀*/
	ComPtr<ID3D11PixelShader> m_pixelShader_featherX;/*第13个像素着色器-羽化X轴*/
	ComPtr<ID3D11PixelShader> m_pixelShader_featherY;/*第14个像素着色器-羽化Y轴*/
	ComPtr<ID3D11PixelShader> m_pixelShader_GarbageMatte;/*第15个像素着色器-垃圾核心遮罩*/
	ComPtr<ID3D11PixelShader> m_pixelShader_CoreMatte;/*第15个像素着色器-垃圾核心遮罩*/
	ComPtr<ID3D11PixelShader> m_pixelShader_Composite;/*第16个像素着色器-合成*/
	ComPtr<ID3D11PixelShader> m_pixelShader_gamma;/*像素着色器-gamma校正*///26添加
	ComPtr<ID3D11PixelShader> m_pixelShader_brightANDcontrast;/*像素着色器-亮度和对比度增强*///26添加
	ComPtr<ID3D11PixelShader> m_pixelShader_sharp;/*像素着色器-锐化*///26添加
		/****************28修改开始****************/
	ComPtr<ID3D11PixelShader> m_pixelShader_binarization;
	/****************28修改结束****************/

	/*着色器顶点输入布局对应的C++结构体*/
	struct VertexPosColor
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 tex;
		static const D3D11_INPUT_ELEMENT_DESC inputLayout[3];
	};
	/*抠像和去除绿色溢出所需的参数对应的常量缓冲区*/
	struct MatteDespillCB/*32字节*/
	{
		DirectX::XMFLOAT4 screen_color;/*屏幕背景颜色值-绿色*/
		float screen_balance;/*屏幕平衡值*/
		int primary_channel;/*背景颜色最大值的通道0/1/2*/
		float despill_factor;/*非溢出系数*/
		float despill_balance;/*非溢出平衡*/
	};

	/*一些像素着色器对应的常量参数*/
	struct constantBuffer_parameter/*44字节*/
	{
		int size;/*预模糊会用到的模糊大小*/
		/*以下是钳制黑色白色所需的参数*/
		int kernelRadius;/*边缘核心半径*/
		float kernelTolerance;/*边缘核心容差*/
		float clipBlack;/*钳制黑色*/
		float clipWhite;/*钳制白色*/
		int size_postBlur;/*后期模糊所需要的模糊大小*/
		int distance;/*膨胀腐蚀的距离*/
		int isDilateErode;/*1为膨胀，2为腐蚀*/
		int do_subtract;/*羽化距离是大于0还是小于0*/
		int sizeFeather;/*羽化距离绝对值*/
		int falloff;/*衰减*/
		float gamma;/*gamma校正*///26添加
		float brightness;/*亮度和对比度增强*///26添加
		float contrast;/*亮度和对比度增强*///26添加
		float sharp_value;/*锐化*///26添加
		/****************28修改开始****************/
		float binarization_threshold;/*二值化参数*/
		/****************28修改结束****************/
	};
	/*垃圾核心遮罩所需数据对应的结构体*/
	struct cBufferGarbageCore {/*36字节*/
		float xBox;/*传入方形遮罩的参数*/
		float yBox;
		float rotationBox;
		float heightBox;
		float widthBox;
		float transparentBox;/*透明度*/
		int maskType;/*遮罩类型*/
		int isBox;/*1代表方形，2代表椭圆形*/
		//int isGarbage;/*1代表垃圾，2代表核心*/
	};
	/*羽化所需参数数组*/
	struct Feather {
		float gausstab[48];
		float distbuf_inv[48];
	};
	ComPtr<ID3D11Buffer> CBufferFeather;// 常量缓冲区--羽化
	Feather featherParameter;
	
	ComPtr<ID3D11InputLayout> m_pVertexLayout;// 顶点输入布局
	ComPtr<ID3D11Buffer> m_pVertexBuffer;// 顶点缓冲区
	/*3个常量缓冲区*/
	ComPtr<ID3D11Buffer> CBufferMatteDespill;// 常量缓冲区--抠像和去除色彩溢出所需参数
	MatteDespillCB matteDespillCB;/*抠像和去除色彩溢出所需要的参数*/
	ComPtr<ID3D11Buffer> CBufferOtherParameter;/*常量缓冲区*/
	constantBuffer_parameter m_parameter;/*钳制黑色白色等所需的参数*/
	ComPtr<ID3D11Buffer> GarbageCBuffer;/*常量缓冲区--存储垃圾遮罩的参数*/
	cBufferGarbageCore parameterGarbage;/*垃圾遮罩*/
	ComPtr<ID3D11Buffer> CoreCBuffer;/*常量缓冲区--存储垃圾遮罩的参数*/
	cBufferGarbageCore parameterCore;/*核心遮罩*/

	ComPtr<ID3D11SamplerState> m_pSamplerState;/*采样器状态*/

	D3D11_TEXTURE2D_DESC texDESC;/*2D纹理描述--CPU不可读写*/
	D3D11_TEXTURE2D_DESC texDESC_cpu;/*2D纹理描述--CPU可读写*/
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;/*着色器资源视图的描述*/
	D3D11_SUBRESOURCE_DATA sd;/*设置纹理资源的描述--初始化时使用*/

	ComPtr<ID3D11Texture2D> backBuffer_save;/*获取后备缓冲区纹理*/
	/*着色器资源视图*/
	ComPtr<ID3D11ShaderResourceView> nowTexSRV;/*着色器资源视图-对应于当前中间纹理*/
	ComPtr<ID3D11ShaderResourceView> texInputSRV;/*着色器资源视图-对应于输入纹理视图*/
	ComPtr<ID3D11ShaderResourceView> texBackSRV;/*着色器资源视图-对应于背景纹理视图*/

	ComPtr<ID3D11Texture2D> nowTex;/*纹理指针，指向当前的中间纹理*/
	ComPtr<ID3D11Texture2D> tex_input;/*输入纹理*/
	ComPtr<ID3D11Texture2D> tex_matte;/*中间纹理1-存储蒙版*/
	ComPtr<ID3D11Texture2D> tex_fore;/*中间纹理2-存储前景*/
	ComPtr<ID3D11Texture2D> tex_preBlur;
	ComPtr<ID3D11Texture2D> tex_clip;
	ComPtr<ID3D11Texture2D> tex_postBlur;
	ComPtr<ID3D11Texture2D> tex_dilateErode;
	ComPtr<ID3D11Texture2D> tex_featherX;
	ComPtr<ID3D11Texture2D> tex_featherY;
	ComPtr<ID3D11Texture2D> tex_Garbage;
	ComPtr<ID3D11Texture2D> tex_Core;
	ComPtr<ID3D11Texture2D> tex_Composite;
	ComPtr<ID3D11Texture2D> tex_back;/*背景纹理*/
	ComPtr<ID3D11Texture2D> tex_gamma;/*gamma校正后的纹理*///26添加
	ComPtr<ID3D11Texture2D> tex_brightANDcontrast;/*亮度和对比度增强*///26添加
	ComPtr<ID3D11Texture2D> tex_sharp;/*锐化*///26添加
	/****************28修改开始****************/
	ComPtr<ID3D11Texture2D> tex_binarization;
	/****************28修改结束****************/

	QTimer* m_pTimer = nullptr;/*定时器--用来更新视频的每一帧像素数据，也就是pixels*/
	VideoCapture cap;/*读取视频---用到了OpenCV*/
	VideoWriter writer;/*写入视频*/
	Mat src;/*临时存储读取的帧*/
	//unsigned char * colorFlow;/*临时存储读取的帧，Kinect获得的彩色图 */
	//unsigned short * depthFlow;/*临时存储读取的帧，Kinect获得的深度图*/
	unsigned char *  colorFlow = (unsigned char*)malloc(sizeof(unsigned char) * colorwidth * colorheight * 3);
	unsigned short *  depthFlow = (unsigned short *)malloc(sizeof(unsigned short)* colorwidth*colorheight);
	//Mat src_depth;/*临时存储读取的帧，Kinect获得的深度图*/
	//Mat src_color;/*临时存储读取的帧，Kinect获得的彩色图 */

	bool video;
	bool ifKinect;
	/*一些参数*/
	wchar_t *file_name;/*图片的地址*/
	uint32_t *pixels;/*图片像素值*/
	uchar * imgData;/*存储当前帧处理后的像素数据*/
	int width, height;/*图片宽高*/

	string filename_bg;/*背景图像路径*/
	bool composite;
	int flag;/*flag=0则输出蒙版，flag=1则输出前景*/
	float ScreenBalanceValue;/*屏幕平衡值*/
	float ScreenColorValue[4];/*屏幕背景颜色值*/
	float DespillFactor;/*非溢出系数*/
	float DespillBalance;/*非溢出平衡*/
	int preBlurValue;/*预模糊值*/
	/*钳制黑色白色所需参数*/
	int kernelRadius;
	float kernelTolerance;
	float clipBlack;
	float clipWhite;
	int blur_post;/*后期模糊*/
	int distance;/*膨胀腐蚀距离*/
	int feather_distance;/*羽化距离*/
	int feather_falloff;/*羽化衰减*/
	/*垃圾遮罩和核心遮罩所需数据*/
	bool isGarbageMatte;/*垃圾遮罩*/
	bool isCoreMatte;/*核心遮罩*/
	bool isBoxMask_Garbage;/*方形遮罩*/
	//bool isEllipseMask;/*椭圆遮罩*/
	bool isBoxMask_Core;/*方形遮罩*/
	//bool isEllipseMask2;/*椭圆遮罩*/
	int maskType_Garbage;/*遮罩类型-垃圾遮罩*/
	int maskType_Core;/*遮罩类型-核心遮罩*/
	float x_Garbage, y_Garbage, rotation_Garbage, width_Garbage, height_Garbage, transparent_Garbage;
	float x_Core, y_Core, rotation_Core, width_Core, height_Core, transparent_Core;
	float gamma;/*gamma校正*///26添加
	float brightness;/*亮度和对比度增强*///26添加
	float contrast;/*亮度和对比度增强*///26添加
	float sharp_value;/*锐化*///26添加
	/****************28修改开始****************/
	float binarization_threshold;/*二值化参数*/
	bool isBinary;/*是否进行二值化处理*/
	/****************28修改结束****************/

	int count;
	int count2;
	LARGE_INTEGER dwStart = { 0 };/*程序开始时间-微秒级*/
	LARGE_INTEGER dwStop = { 0 };/*程序结束时间-微秒级*/
	double pcFreqall;/*时钟频率*/
	double readAllFrameTime;/*读帧时间*/
	double writeAllFrameTime;/*写帧时间*/
	double myMatteTime;/*新设置的抠像时间*/
public:
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent *event);

	void InitD3D();/*初始化DirectX基本框架*/
	void InitEffect();/*初始化着色器*/
	void InitResource();/*初始化顶点缓冲区并设置着色器资源*/
	//void ResizeD3D();/*当窗口大小改变需要刷新下面的东西*/

	void setupMatte();/*输出蒙版*/
	void setupForeImage();/*输出前景图像*/
	void setupPreBlur();/*设置预模糊*/
	void setupClip();/*设置钳制黑色白色*/
	void setupPostBlur();/*设置后期模糊*/
	void setupDilateErode();/*设置膨胀腐蚀*/
	void setupFeather();/*设置羽化*/
	void setupFeatherXorY(int flag);/*设置X或Y轴的羽化*/
	void setupGarbageMatte();/*设置垃圾遮罩*/
	void setupCoreMatte();/*设置核心遮罩*/
	void setupComposite();/*合成前景背景*/
	void setupGamma();/*对前景进行gamma校正*///26添加
	void setupbrightANDcontrast();/*亮度和对比度增强*///26添加
	void setupSharp();/*锐化*///26添加
	/****************28修改开始****************/
	void setupBinarization();
	/****************28修改结束****************/

	void setComposite(bool composite,string filename_back);/*设置背景着色器资源视图*/
	void setConstantBuffers();/*设置常量缓冲区的值*/
	void make_gausstab(float rad, const int size);
	void make_dist_fac_inverse(float rad, int size, int falloff);
};
