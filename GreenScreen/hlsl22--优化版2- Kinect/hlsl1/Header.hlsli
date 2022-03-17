/*Texture2D类型保存了2D纹理的信息，在这是全局变量,register(t0)对应起始槽索引0*/
Texture2D gTex : register(t0);/*初始纹理*/
Texture2D gTex_now : register(t1);/*中间纹理*/
Texture2D gTex_back : register(t2);/*背景纹理*/
								   
/*SamplerState类型确定采样器应如何进行采样，同样也是全局变量，register(s0)对应起始槽索引0*/
SamplerState gSamLinear : register(s0);
/*上述两种变量都需要在C++应用层中初始化和绑定后才能使用。*/

/*抠像和去除色彩溢出所需要的参数*/
cbuffer ConstantBuffer_matteDespill : register(b0)/*寄存于b0的常量缓冲区*/
{
	float4 screen_color;/*屏幕背景颜色值-绿色*/
	float screen_balance;/*屏幕平衡值*/
	int primary_channel;/*背景颜色最大值的通道0/1/2*/
	float despill_factor;/*非溢出系数*/
	float despill_balance;/*非溢出平衡*/
}
/*存储一些参数-44+4=48+4+4=56+4=60字节*///26添加，原来是48，现在变成了60
cbuffer ConstantBuffer_parameter : register(b2)/*寄存于b2的常量缓冲区*/
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
	int do_subtract;/*羽化距离是大于0还是小于0,大于0是0，小于0是1*/
	int sizeFeather;/*羽化距离绝对值*/
	int falloff;/*衰减*/
	float gamma;/*gamma校正*///26添加
	float brightness;/*亮度和对比度增强*///26添加
	float contrast;/*亮度和对比度增强*///26添加
	float sharp_value;/*锐化参数*///26添加
	/****************28修改开始****************/
	float binarization_threshold;/*二值化参数*/
	/****************28修改结束****************/
}
/*垃圾遮罩所需参数的常量缓冲区--36字节*/
cbuffer ConstantBuffer_parameter2 : register(b3)/*寄存于b3的常量缓冲区*/
{
	float xBox;/*传入方形遮罩的参数*/
	float yBox;
	float rotationBox;
	float heightBox;
	float widthBox;
	float transparentBox;/*透明度*/
	int maskType;/*遮罩类型*/
	int isBox;/*1代表方形，2代表椭圆形*/
	//int isGarbage;/*1代表垃圾，2代表核心*/
}
/*核心遮罩所需参数的常量缓冲区--36字节*/
cbuffer ConstantBuffer_parameter3 : register(b1)/*寄存于b1的常量缓冲区*/
{
	float xBoxCore;/*传入方形遮罩的参数*/
	float yBoxCore;
	float rotationBoxCore;
	float heightBoxCore;
	float widthBoxCore;
	float transparentBoxCore;/*透明度*/
	int maskTypeCore;/*遮罩类型*/
	int isBoxCore;/*1代表方形，2代表椭圆形*/
	//int isGarbage;/*1代表垃圾，2代表核心*/
}
/*羽化的数组参数--384字节*/
cbuffer ConstantBuffer_parameter4 : register(b4)/*寄存于b4的常量缓冲区*/
{
	float4 gausstab[12];/*48*4=192字节*/
	float4 distbuf_inv[12];/*48*4=192字节*/
}

 /*顶点着色器输入结构体*/
struct VertexIn
{
	float3 pos : POSITION;
	float4 color : COLOR;
	float2 Tex : TEXCOORD;
};

/*顶点着色器输出结构体*/
struct VertexOut
{
	float4 posH : SV_POSITION;
	float4 color : COLOR;
	float2 Tex : TEXCOORD;
};