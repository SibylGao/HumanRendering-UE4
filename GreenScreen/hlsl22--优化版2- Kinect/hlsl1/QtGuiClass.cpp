#include "QtGuiClass.h"
#include "strsafe.h"
#include <QResizeEvent>
#include <QTimer>
#include<qDebug>/*用于输出错误信息到控制台*/
using namespace DirectX;/*可以用XMFLOAT3*/
	
////////////////////
///      socket
////////////////////
othka::comm::SocketTCP sender_rgb;
othka::comm::SocketTCP sender_depth;
//录视频
//int videoFrameIndex = 0;
//cv::VideoWriter videoOut;

bool lock_depth_rgb_pair = true;//true表示可以发送
/*顶点输入布局*/
const D3D11_INPUT_ELEMENT_DESC QtGuiClass::VertexPosColor::inputLayout[3] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};
/*返回背景颜色的最大值通道*/
int max_axis_v3(float *ScreenColorValue) {
	float x = ScreenColorValue[0];
	float y = ScreenColorValue[1];
	float z = ScreenColorValue[2];
	return ((x > y) ? ((x > z) ? 0 : 2) : ((y > z) ? 1 : 2));
}
float RE_filter_value(int type, float x)/*type是高斯*/
{
	float gaussfac = 1.6f;
	x = abs(x);
	float two_gaussfac2 = 2.0f * gaussfac * gaussfac;
	x *= 3.0f * gaussfac;
	return 1.0f / sqrt(3.14 * two_gaussfac2) * exp(-x * x / two_gaussfac2);
}
/*创建一个gausstab数组并赋值*/
void QtGuiClass::make_gausstab(float rad, const int size)/*rab和size都是羽化距离*/
{
	float sum, val;
	int i, n;
	n = 2 * size + 1;/*数组的最大长度，但是现在无法开辟不定长数组*/
	sum = 0.0f;
	float fac = (rad > 0.0f ? 1.0f / rad : 0.0f);
	for (i = -size; i <= size; i++) {
		val = RE_filter_value(5, float(i * fac));
		sum += val;
		featherParameter.gausstab[i + size] = val;
	}
	sum = 1.0f / sum;
	for (i = 0; i < n; i++) {
		featherParameter.gausstab[i] *= sum;
	}
}
void QtGuiClass::make_dist_fac_inverse(float rad, int size, int falloff)
{
	float val;
	int i, n;
	n = 2 * size + 1;
	float fac = (rad > 0.0f ? 1.0f / rad : 0.0f);
	for (i = -size; i <= size; i++) {
		val = 1.0f - abs(float(i * fac));
		switch (falloff) {
		case 0:
			val = (3.0f * val * val - 2.0f * val * val * val);
			break;
		case 1:
			val = sqrt(2.0f * val - val * val);
			break;
		case 2:
			val = sqrt(val);
			break;
		case 3:
			val = val * val;
			break;
		case 7:
			val = val * (2.0f - val);
			break;
		case 4:
			/* nothing to do */
			break;
		default:
			/* nothing */
			break;
		}
		featherParameter.distbuf_inv[i + size] = val;
	}
}
/*将string类型转换为wchar_t类型*/
//不要忘记在使用完wchar_t*后delete[]释放内存
wchar_t *multiByteToWideChar(const string& pKey)
{
	const char* pCStrKey = pKey.c_str();
	//第一次调用返回转换后的字符串长度，用于确认为wchar_t*开辟多大的内存空间
	int pSize = MultiByteToWideChar(CP_OEMCP, 0, pCStrKey, strlen(pCStrKey) + 1, NULL, 0);
	wchar_t *pWCStrKey = new wchar_t[pSize];
	//第二次调用将单字节字符串转换成双字节字符串
	MultiByteToWideChar(CP_OEMCP, 0, pCStrKey, strlen(pCStrKey) + 1, pWCStrKey, pSize);
	return pWCStrKey;
}
/*构造函数*/
QtGuiClass::QtGuiClass(string filename, bool video,QWidget *parent) :QWidget(parent)
{
	ui.setupUi(this);
	/*如果使用用户自定义绘制，则需要设置WA_PaintOnScreen*/
	QWidget::setAttribute(Qt::WA_PaintOnScreen); // 允许DX渲染 ...
	setFocusPolicy(Qt::WheelFocus); // 允许按键时间 ...
	setMouseTracking(true); // 允许鼠标移动 ...

	this->flag = 0;/*初始化输出蒙版*/
	//bgcolor[0] = 0;
	//bgcolor[1] = 0.5;
	//bgcolor[2] = 0.5;
	//bgcolor[3] = 1;
	bgcolor[0] = 1;
	bgcolor[1] = 1;
	bgcolor[2] = 1;
	bgcolor[3] = 1;
	this->video = video;
	/****************28修改开始****************/
	this->isBinary = false;
	/****************28修改结束****************/
	this->count = 0;
	this->count2 = 0;
	/*设置定时器，定时更新*/
	m_pTimer = new QTimer(this);
	/*设置定时器每个多少毫秒发送一个timeout()信号*/
	/*这个时间间隔不能小于20ms否则会出现少帧的问题*/
	m_pTimer->setInterval(20);/*设置时间间隔20毫秒*/
	/*连接定时器信号和槽函数*/
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(updateFrame()));
	this->pixels = NULL;/*初始化像素数据指针*/
	/*系统开始时间*/
	QueryPerformanceFrequency(&dwStart);
	//电脑CPU时钟频率
	pcFreqall = (double)dwStart.QuadPart / 1000000.0;
	QueryPerformanceCounter(&dwStart);

	this->readAllFrameTime = 0;
	this->writeAllFrameTime = 0;
	this->myMatteTime = 0;

	/*读第一帧开始时间*/
	LARGE_INTEGER beginTime = { 0 };
	QueryPerformanceFrequency(&beginTime);
	//电脑CPU时钟频率
	double pcFreq = (double)beginTime.QuadPart / 1000000.0;
	QueryPerformanceCounter(&beginTime);

	if (video == true) {/*读取视频第一帧*/
		cap.open(filename); //打开视频
		if (!cap.isOpened())
			return;

		/*写入视频*/
		Size size0 = Size(cap.get(CV_CAP_PROP_FRAME_WIDTH), cap.get(CV_CAP_PROP_FRAME_HEIGHT));
		writer.open("E://outDirectX1.avi", -1, cap.get(CV_CAP_PROP_FPS), size0, true);

		m_pTimer->start();/*定时器开始*///这样updateFrame就开始被调用了
		/*读取视频第一帧*/			
		cap >> src;//等价于cap.read(frame);
		count++;
	//	qDebug() << "--------read count" << count << endl; 
		if (src.empty()) {//如果某帧为空则退出循环
			pixels = NULL;
		}
	}
	/****************29修改开始****************/
	else {
		  /*用OpenCV读取一张图片的像素数据pixels--"F:\\20191219.jpg"*/
		src = imread(filename); //一般cv2.imread()读取的图片都是BGR颜色空间的图片
		int channels = src.channels();/*获取当前图片的通道数*/
		if (channels == 3) {/*输入图像是jpg格式*/
			cvtColor(src, src, COLOR_BGR2RGBA);/*必须转换为RGBA模式，否则输出有误*/
		}
		if (channels == 4) {/*输入图像是png格式*/
			cvtColor(src, src, COLOR_BGRA2RGBA);/*必须转换为RGBA模式，否则输出有误*/
		}
	}
	/****************29修改结束****************/

	width = src.cols;
	height = src.rows;
	pixels = new uint32_t[width*height * 4];
	memcpy(pixels, src.data, width*height * 4);
	src.release();

	/*读第一帧结束时间*/
	LARGE_INTEGER endTime = { 0 };
	QueryPerformanceCounter(&endTime);
	//那么下面计算得出的就是这之间的时间间隔了，单位为微秒
	int time = (endTime.QuadPart - beginTime.QuadPart) / pcFreq;
	this->readAllFrameTime = this->readAllFrameTime + time;

	this->resize(QSize(width, height));/*设置当前QT窗口大小*/

	InitD3D();
	InitEffect();
	InitResource();
}
//重载构造函数
QtGuiClass::QtGuiClass(bool video, bool ifKinect, QWidget *parent) :QWidget(parent)
{
	//实时展示rgb
	//namedWindow("enhanced", 0);
	//resizeWindow("enhanced", 640, 480);
	//录视频
	//videoOut.open("Video\\1.mov", CV_FOURCC('M', 'J', 'P', 'G'), 20, cv::Size(colorwidth, colorheight));

	//初始化sender
	sender_rgb.Connect("127.0.0.1", 1234);
	sender_rgb.Blocking(false);

	sender_depth.Connect("127.0.0.1", 1235);
	sender_depth.Blocking(false);

	ui.setupUi(this);
	/*如果使用用户自定义绘制，则需要设置WA_PaintOnScreen*/
	QWidget::setAttribute(Qt::WA_PaintOnScreen); // 允许DX渲染 ...
	setFocusPolicy(Qt::WheelFocus); // 允许按键时间 ...
	setMouseTracking(true); // 允许鼠标移动 ...

	this->flag = 0;/*初始化输出蒙版*/
	//bgcolor[0] = 0;
	//bgcolor[1] = 0.5;
	//bgcolor[2] = 0.5;
	//bgcolor[3] = 1;
	bgcolor[0] = 1;
	bgcolor[1] = 1;
	bgcolor[2] = 1;
	bgcolor[3] = 1;
	this->video = video;
	/****************28修改开始****************/
	this->isBinary = false;
	/****************28修改结束****************/
	this->ifKinect = ifKinect;
	this->count = 0;
	this->count2 = 0;
	/*设置定时器，定时更新*/
	m_pTimer = new QTimer(this);
	/*设置定时器每个多少毫秒发送一个timeout()信号*/
	/*这个时间间隔不能小于20ms否则会出现少帧的问题*/
	m_pTimer->setInterval(20);/*设置时间间隔20毫秒*/
	/*连接定时器信号和槽函数*/
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(updateFrame_Kinect()));
	this->pixels = NULL;/*初始化像素数据指针*/
	/*系统开始时间*/
	QueryPerformanceFrequency(&dwStart);
	//电脑CPU时钟频率
	pcFreqall = (double)dwStart.QuadPart / 1000000.0;
	QueryPerformanceCounter(&dwStart);

	this->readAllFrameTime = 0;
	this->writeAllFrameTime = 0;
	this->myMatteTime = 0;

	/*读第一帧开始时间*/
	LARGE_INTEGER beginTime = { 0 };
	QueryPerformanceFrequency(&beginTime);
	//电脑CPU时钟频率
	double pcFreq = (double)beginTime.QuadPart / 1000000.0;
	QueryPerformanceCounter(&beginTime);

	//初始化Kinect。要放在最开始，也就是hlsl1.cpp的构造函数，随整个工程而启动。
	//为什么把它放到最开始呢？因为Kinect启动需要时间。如果没启动就采深度数据的话，采集到的深度数据是全灰图像，深度值是52685
	if (video == false && ifKinect == true) {
		m_pTimer->start();/*定时器开始*/
		//src = imread("C:\\Users\\Administrator\\Desktop\\temp\\greenback2.jpg");//这里需要改成从Kinect流中取图片到src
		//colorFlow = (unsigned char*)malloc(sizeof(unsigned char) * colorwidth * colorheight * 3);
		//depthFlow = (unsigned short *)malloc(sizeof(unsigned short)* colorwidth*colorheight);
		getKinectData(colorFlow, depthFlow);
		//src = cv::Mat(colorheight, colorwidth, CV_16UC1, depthFlow);//深度图不用转成mat的，这里用作调试
		src = cv::Mat(colorheight, colorwidth, CV_8UC3, colorFlow);//彩色图赋给src，用于前端渲染
		//imshow("dd", src);//显示正常，说明它是BGR
		//cv::waitKey(0);
		//socket发送数据
		if (depthFlow[0] != 52685 && colorFlow[0] != 205) {//过滤灰色闪屏，灰色闪屏导致ue4爆GPU
			sender_depth.Send((char *)depthFlow, 1080 * 1920 * 2);//因为unsigned short是16位，char是8位。所以这里把通道数算作2。
			//sender_rgb.Send((char*)colorFlow, 1080 * 1920 * 3);
		}


		count++;
		if (src.empty()) {//如果某帧为空则退出循环
			pixels = NULL;
		}
	}

	cvtColor(src, src, COLOR_BGR2RGBA);/*必须转换为RGBA模式，否则输出有误*/
	width = src.cols;
	height = src.rows;
	pixels = new uint32_t[width*height * 4];
	memcpy(pixels, src.data, width*height * 4);
	src.release();
	//free(depthFlow);//不用src了才可以free
	//free(colorFlow);
	/*读第一帧结束时间*/
	LARGE_INTEGER endTime = { 0 };
	QueryPerformanceCounter(&endTime);
	//那么下面计算得出的就是这之间的时间间隔了，单位为微秒
	int time = (endTime.QuadPart - beginTime.QuadPart) / pcFreq;
	this->readAllFrameTime = this->readAllFrameTime + time;

	this->resize(QSize(width, height));/*设置当前QT窗口大小*/

	InitD3D();
	InitEffect();
	InitResource();
}

/*析构函数*/
QtGuiClass::~QtGuiClass()
{
	if (this->pixels != NULL) {
		delete[] pixels;
	}
	// 恢复所有默认设定
	if (m_d3dDevContext) {
		m_d3dDevContext->ClearState();
	}

	//qDebug() << "----------DirectX has xigou------" << endl;
}
/*槽函数，更新纹理中的数据*/
void QtGuiClass::updateFrame_Kinect() {
	/*读帧开始时间*/
	LARGE_INTEGER beginTime = { 0 };
	QueryPerformanceFrequency(&beginTime);
	//电脑CPU时钟频率
	double pcFreq = (double)beginTime.QuadPart / 1000000.0;
	QueryPerformanceCounter(&beginTime);

	//cap >> src;//等价于cap.read(frame)
	//src = imread("C:\\Users\\Administrator\\Desktop\\temp\\greenback2.jpg");//这里需要改成从Kinect流中取图片到src
	//colorFlow = (unsigned char*)malloc(sizeof(unsigned char) * colorwidth * colorheight * 3);
	//depthFlow = (unsigned short *)malloc(sizeof(unsigned short)* colorwidth*colorheight);
	getKinectData(colorFlow, depthFlow);
	//src = cv::Mat(colorheight, colorwidth, CV_16UC1, depthFlow);//深度图不用转成mat的，这里用作调试
	src = cv::Mat(colorheight, colorwidth, CV_8UC3, colorFlow);//彩色图赋给src，用于前端渲染
	//cv::imshow("d", src);

	//录视频
	//if (videoFrameIndex++ < 500) {
	//	videoOut << src;
	//}
	//else
	//	videoOut.release();

	//实时展示rgb
	//imshow("enhanced", src);//显示正常，说明它是BGR
	//return;

	//socket发送数据
	if (depthFlow[0] != 52685 && colorFlow[0] != 205) {//过滤灰色闪屏，灰色闪屏导致ue4爆GPU
		sender_depth.Send((char *)depthFlow, 1080 * 1920 * 2);//因为unsigned short是16位，char是8位。所以这里把通道数算作2。
		//sender_rgb.Send((char*)colorFlow, 1080 * 1920 * 3);
	}
	else {//意味着不执行update，也就不发送RGB
		src.release();
		//free(depthFlow);//不用src了才可以free
		//free(colorFlow);
		return;
	}
	//cv::imshow("wwe", src);//频闪，每隔几帧会多一个全灰。(205, 205, 205)
	//cv::imshow("de", cv::Mat(colorheight, colorwidth, CV_16UC1, depthFlow));//它俩频闪的频率不一样。52685
	//cv::waitKey(0);
	count++;
	//qDebug()<< "#################count:" << count << endl;
	if (src.empty()) {//如果某帧为空则退出循环
		//qDebug() << "#################src Empty!!!" << endl;

		free(pixels);/*清除上一帧的数据*/
		pixels = NULL;
		m_pTimer->stop();/*停止计时器*/
		//qDebug() << "--------last count" << count << endl;
		/*计时结束*/
		QueryPerformanceCounter(&dwStop);
		int allTime = (dwStop.QuadPart - dwStart.QuadPart) / pcFreqall;/*所用总时间*/
		/*抠像时间*/
		double mattetime = (allTime - this->readAllFrameTime - this->writeAllFrameTime) / 1000000.0;
		qDebug() << "------------------Frame number : " << count << "frame" << endl;
		qDebug() << "------------------Running all Time : " << allTime / 1000000.0 << "s" << endl;
		this->readAllFrameTime = this->readAllFrameTime / 1000000.0;
		qDebug() << "------------------readAllFrameTime : " << this->readAllFrameTime << "s" << endl;
		this->writeAllFrameTime = this->writeAllFrameTime / 1000000.0;
		qDebug() << "------------------writeAllFrameTime : " << this->writeAllFrameTime << "s" << endl;
		qDebug() << "------------------Matting Time : " << mattetime << "s" << endl;
		qDebug() << "------------------matte rate : " << count / mattetime << endl;
		/*输出新的抠像时间*/
		this->myMatteTime = this->myMatteTime / 1000000.0;
		qDebug() << "------------------myMatteTime: " << this->myMatteTime << endl;
		qDebug() << "------------------myMatteRate : " << count / this->myMatteTime << endl;
		//writer.release();
		//cap.release();
		this->close();
		//this->~QtGuiClass();/*析构掉这个对象*/
	}
	else {
		free(pixels);/*清除上一帧的数据*/
		cvtColor(src, src, COLOR_BGR2RGBA);/*必须转换为RGBA模式，否则输出有误*/
		this->width = src.cols;
		this->height = src.rows;
		pixels = new  uint32_t[width*height * 4];
		memcpy(pixels, src.data, width*height * 4);
		src.release();
		//free(depthFlow);//不用src了才可以free
		//free(colorFlow);
		/*读帧结束时间*/
		LARGE_INTEGER endTime = { 0 };
		QueryPerformanceCounter(&endTime);
		//那么下面计算得出的就是这之间的时间间隔了，单位为微秒
		int time = (endTime.QuadPart - beginTime.QuadPart) / pcFreq;
		this->readAllFrameTime = this->readAllFrameTime + time;

		update();/*重新绘制*/
	}
}
/*槽函数，更新纹理中的数据*/
void QtGuiClass::updateFrame() {
	/*读帧开始时间*/
	LARGE_INTEGER beginTime = { 0 };
	QueryPerformanceFrequency(&beginTime);
	//电脑CPU时钟频率
	double pcFreq = (double)beginTime.QuadPart / 1000000.0;
	QueryPerformanceCounter(&beginTime);

	cap >> src;//等价于cap.read(frame);
	count++;
	//qDebug() << "--------read count" << count << endl;
	if (src.empty()) {//如果某帧为空则退出循环
		free(pixels);/*清除上一帧的数据*/
		pixels = NULL;
		m_pTimer->stop();/*停止计时器*/
		//qDebug() << "--------last count" << count << endl;
		/*计时结束*/
		QueryPerformanceCounter(&dwStop);
		int allTime = (dwStop.QuadPart - dwStart.QuadPart) / pcFreqall;/*所用总时间*/
		/*抠像时间*/
		double mattetime = (allTime - this->readAllFrameTime - this->writeAllFrameTime) / 1000000.0;
		qDebug() << "------------------Frame number : " << count << "frame" << endl;
		qDebug() << "------------------Running all Time : " << allTime / 1000000.0 << "s" << endl;
		this->readAllFrameTime = this->readAllFrameTime / 1000000.0;
		qDebug() << "------------------readAllFrameTime : " << this->readAllFrameTime << "s" << endl;
		this->writeAllFrameTime = this->writeAllFrameTime / 1000000.0;
		qDebug() << "------------------writeAllFrameTime : " << this->writeAllFrameTime << "s" << endl;
		qDebug() << "------------------Matting Time : " << mattetime << "s" << endl;
		qDebug() << "------------------matte rate : " << count / mattetime << endl;
		/*输出新的抠像时间*/
		this->myMatteTime=this->myMatteTime / 1000000.0;
		qDebug() << "------------------myMatteTime: " << this->myMatteTime << endl;
		qDebug() << "------------------myMatteRate : " << count / this->myMatteTime << endl;
		writer.release();
		cap.release();
		this->close();
		//this->~QtGuiClass();/*析构掉这个对象*/
	}
	else {
		free(pixels);/*清除上一帧的数据*/
		cvtColor(src, src, COLOR_BGR2RGBA);/*必须转换为RGBA模式，否则输出有误*/
		this->width = src.cols;
		this->height = src.rows;
		pixels = new  uint32_t[width*height * 4];
		memcpy(pixels, src.data, width*height * 4);

		/*读帧结束时间*/
		LARGE_INTEGER endTime = { 0 };
		QueryPerformanceCounter(&endTime);
		//那么下面计算得出的就是这之间的时间间隔了，单位为微秒
		int time = (endTime.QuadPart - beginTime.QuadPart) / pcFreq;
		this->readAllFrameTime = this->readAllFrameTime + time;

		update();/*重新绘制*/
	}
}
void QtGuiClass::resizeEvent(QResizeEvent *event)
{
	//ResizeD3D();
}

/*初始化DirectX11的基本框架*/
void QtGuiClass::InitD3D()
{
	/*设置后台缓冲区的描述*/
	DXGI_MODE_DESC bufferDesc;//Dx用来描述 后台缓冲区 的结构体
	ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));//这个宏就是用来把Dx描述结构体，里面的变量全部给个0
	bufferDesc.Width = width; //QT自带的获取窗口宽度的方法--后台缓冲区宽度
	bufferDesc.Height = height;//QT自带的获取窗口高度的方法--后台缓冲区高度
	bufferDesc.RefreshRate.Numerator = 60; //（注释1）  显示最大的刷新率  刷新率如果是  60pfs/每1秒
	bufferDesc.RefreshRate.Denominator = 1; //（注释1）  显示最大刷新率
	bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;// 后台缓冲区像素格式
	bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED; //ScanlineOrdering:用于描述扫描线绘制模式的枚举。可以设置为0
	bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;//用于描述缩放形式的枚举类型，默认使用DXGI_MODE_SCALING_UNSPECIFIED。
	/*设置交换链的描述*/
	DXGI_SWAP_CHAIN_DESC swapChainDesc;//Dx用来描述交换缓冲区的结构体
	ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
	swapChainDesc.BufferDesc = bufferDesc;//多重采样数量和质量级别
	swapChainDesc.SampleDesc.Count = 1;//多重采样的结构体描述
	swapChainDesc.SampleDesc.Quality = 0;//多重采样的结构体描述
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;//设为 DXGI_USAGE_RENDER_TARGET_OUTPUT，因为我们要将场景渲染到后台缓冲区
	swapChainDesc.BufferCount = 1;//交换链中的后台缓冲区数量；我们一般只用一个后台缓冲区来实现双 缓存。当然，你也可以使用两个后台缓冲区来实现三缓存。
	swapChainDesc.OutputWindow = (HWND)winId();//这个非常重要，winId()是Qt中QWidget获取操作句柄的函数，这里表示我们把当前QWidget的控制权交给了DX，让DX去管理这个句柄
	swapChainDesc.Windowed = TRUE;//当设为 true 时，程序以窗口模式运行；当设为 false 时，程序以全屏模式运行
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;//设为 DXGI_SWAP_EFFECT_DISCARD，让显卡驱动程序选择最高效 的显示模式

	//这是一个最新的方法，用来同时创建设备、设备上下文和交换链
	/*修改一个参数可以输出错误信息*/
	hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_DEBUG, NULL, NULL,
		D3D11_SDK_VERSION, &swapChainDesc, m_swapChain.ReleaseAndGetAddressOf(), m_d3dDevice.ReleaseAndGetAddressOf(), NULL, m_d3dDevContext.ReleaseAndGetAddressOf());
	
	//获取后台缓冲区的指针
	hr = m_swapChain->GetBuffer(0,__uuidof(ID3D11Texture2D),(void**)backBuffer.ReleaseAndGetAddressOf());
	if (hr != 0) {
		qDebug() << "get buffer failed!" << endl;
	}
	//这里真正的创建了目标渲染视图，就是平时我们肉眼所看到的显示的那个界面viewport，把后台缓冲区绑定到目标渲染视图
	hr = m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.ReleaseAndGetAddressOf());
	//XYB_SafeDel(backBuffer)//（注释4）这个非常重要，宏是我为了安全释放COM自定义的
	//每调用一次GetBuffer 方法，后台缓冲区的 COM 引用计数就会 向上递增一次，这便是我们在代码片段的结尾处释放它的原因
	//这个不释放你会发现绘制不了shader，但是却可以clear屏幕颜色哦

	//下面就是创建深度模板缓冲区,创建过程和目标渲染视图基本一样，这里仅对不一样的参数做下注解
	D3D11_TEXTURE2D_DESC depthStencilDesc;
	depthStencilDesc.Width = width;/*(uint)width()*/
	depthStencilDesc.Height = height;/*(uint)height()*/
	depthStencilDesc.MipLevels = 1;//mipLevels要生成几级，1表示多采样纹理，0表示会生成一整套的mipMap，这里不需要mip所以写1
	depthStencilDesc.ArraySize = 1;//纹理数组中的纹理数量，比如立方体贴图由六个面，这边就可以设置为6，或者6的倍数
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//纹理格式
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;//绑定标识符，描述这个是一个深度模板缓冲图片，其实这个结构体就是个普通的2D纹理声明的结构体
	depthStencilDesc.CPUAccessFlags = 0; //这个没有搞懂，后续弄懂了补充更新文章吧，有大佬知道的欢迎评论区补充下谢谢，大概是允许CPU访问的
	depthStencilDesc.MiscFlags = 0;//这个是控制上面那个CPU访问的

	//在GPU上根据上面那个描述创建一块用于纹理储存的内存Buffer
	hr = m_d3dDevice->CreateTexture2D(&depthStencilDesc, NULL, m_depthStencilBuffer.ReleaseAndGetAddressOf());
	//把这个buffer绑定给视口
	hr = m_d3dDevice->CreateDepthStencilView(m_depthStencilBuffer.Get(), NULL, m_depthStencilView.ReleaseAndGetAddressOf());
	//把目标渲染视口和深度模板关联起来
	m_d3dDevContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
	//第四步，设置视口大小，D3D11默认不会设置视口，此步骤必须手动设置
	viewport.TopLeftX = 0;//视口左上角的横坐标
	viewport.TopLeftY = 0;//视口左上角的总坐标
	viewport.Width = width; //宽
	viewport.Height = height;//高
	viewport.MinDepth = 0.0f;//深度缓存最小值0，dx里深度值是0到1范围内的，所以下限值是0
	viewport.MaxDepth = 1.0f;//深度值最大值1，
	//刷新重置下viewport
	m_d3dDevContext->RSSetViewports(1, &viewport);
}

/*初始化着色器*/
void QtGuiClass::InitEffect() {
	HRESULT result;
	/*读取编译好的着色器二进制信息*/
	result = D3DReadFileToBlob(L"HLSL\\VertexShader.cso", m_vertexBlob.ReleaseAndGetAddressOf());
	result = D3DReadFileToBlob(L"HLSL\\PixelShader_matte.cso", m_pixelBlob_matte.ReleaseAndGetAddressOf());
	result = D3DReadFileToBlob(L"HLSL\\PixelShader_fore.cso", m_pixelBlob_fore.ReleaseAndGetAddressOf());
	result = D3DReadFileToBlob(L"HLSL\\PixelShader_output.cso", m_pixelBlob_output.ReleaseAndGetAddressOf());
	result = D3DReadFileToBlob(L"HLSL\\PixelShader_preBlur.cso", m_pixelBlob_preBlur.ReleaseAndGetAddressOf());
	result = D3DReadFileToBlob(L"HLSL\\PixelShader_clip.cso", m_pixelBlob_clip.ReleaseAndGetAddressOf());
	result = D3DReadFileToBlob(L"HLSL\\PixelShader_postBlur.cso", m_pixelBlob_postBlur.ReleaseAndGetAddressOf());
	result = D3DReadFileToBlob(L"HLSL\\PixelShader_dilateErode.cso", m_pixelBlob_dilateErode.ReleaseAndGetAddressOf());
	result = D3DReadFileToBlob(L"HLSL\\PixelShader_featherX.cso", m_pixelBlob_featherX.ReleaseAndGetAddressOf());
	result = D3DReadFileToBlob(L"HLSL\\PixelShader_featherY.cso", m_pixelBlob_featherY.ReleaseAndGetAddressOf());
	result = D3DReadFileToBlob(L"HLSL\\PixelShader_GarbageMatte.cso", m_pixelBlob_GarbageMatte.ReleaseAndGetAddressOf());
	result = D3DReadFileToBlob(L"HLSL\\PixelShader_CoreMatte.cso", m_pixelBlob_CoreMatte.ReleaseAndGetAddressOf());
	result = D3DReadFileToBlob(L"HLSL\\PixelShader_Composite.cso", m_pixelBlob_Composite.ReleaseAndGetAddressOf());
	result = D3DReadFileToBlob(L"HLSL\\PixelShader_gamma.cso", m_pixelBlob_gamma.ReleaseAndGetAddressOf());//26添加
	result = D3DReadFileToBlob(L"HLSL\\PixelShader_brightANDcontrast.cso", m_pixelBlob_brightANDcontrast.ReleaseAndGetAddressOf());//26添加
	result = D3DReadFileToBlob(L"HLSL\\PixelShader_sharp.cso", m_pixelBlob_sharp.ReleaseAndGetAddressOf());//26添加
	/****************28修改开始****************/
	result = D3DReadFileToBlob(L"HLSL\\PixelShader_binarization.cso", m_pixelBlob_binarization.ReleaseAndGetAddressOf());
	/****************28修改结束****************/

	/*创建顶点着色器*/
	result = m_d3dDevice->CreateVertexShader(m_vertexBlob->GetBufferPointer(), m_vertexBlob->GetBufferSize(), 
		0, m_vertexShader.ReleaseAndGetAddressOf());
	// 创建并绑定顶点布局
	m_d3dDevice->CreateInputLayout(VertexPosColor::inputLayout, ARRAYSIZE(VertexPosColor::inputLayout), m_vertexBlob->GetBufferPointer(),
		m_vertexBlob->GetBufferSize(), m_pVertexLayout.ReleaseAndGetAddressOf());//把Position传入Shader

	/*创建众多像素着色器*/
	result = m_d3dDevice->CreatePixelShader(m_pixelBlob_matte->GetBufferPointer(), m_pixelBlob_matte->GetBufferSize(),
		0, m_pixelShader_matte.ReleaseAndGetAddressOf());
	result = m_d3dDevice->CreatePixelShader(m_pixelBlob_fore->GetBufferPointer(), m_pixelBlob_fore->GetBufferSize(),
		0, m_pixelShader_fore.ReleaseAndGetAddressOf());
	result = m_d3dDevice->CreatePixelShader(m_pixelBlob_output->GetBufferPointer(), m_pixelBlob_output->GetBufferSize(),
		0, m_pixelShader_output.ReleaseAndGetAddressOf());
	result = m_d3dDevice->CreatePixelShader(m_pixelBlob_preBlur->GetBufferPointer(), m_pixelBlob_preBlur->GetBufferSize(),
		0, m_pixelShader_preBlur.ReleaseAndGetAddressOf());
	result = m_d3dDevice->CreatePixelShader(m_pixelBlob_clip->GetBufferPointer(), m_pixelBlob_clip->GetBufferSize(),
		0, m_pixelShader_clip.ReleaseAndGetAddressOf());
	result = m_d3dDevice->CreatePixelShader(m_pixelBlob_postBlur->GetBufferPointer(), m_pixelBlob_postBlur->GetBufferSize(),
		0, m_pixelShader_postBlur.ReleaseAndGetAddressOf());
	result = m_d3dDevice->CreatePixelShader(m_pixelBlob_dilateErode->GetBufferPointer(), m_pixelBlob_dilateErode->GetBufferSize(),
		0, m_pixelShader_dilateErode.ReleaseAndGetAddressOf());
	result = m_d3dDevice->CreatePixelShader(m_pixelBlob_featherX->GetBufferPointer(), m_pixelBlob_featherX->GetBufferSize(),
		0, m_pixelShader_featherX.ReleaseAndGetAddressOf());
	result = m_d3dDevice->CreatePixelShader(m_pixelBlob_featherY->GetBufferPointer(), m_pixelBlob_featherY->GetBufferSize(),
		0, m_pixelShader_featherY.ReleaseAndGetAddressOf());
	result = m_d3dDevice->CreatePixelShader(m_pixelBlob_GarbageMatte->GetBufferPointer(), m_pixelBlob_GarbageMatte->GetBufferSize(),
		0, m_pixelShader_GarbageMatte.ReleaseAndGetAddressOf());
	result = m_d3dDevice->CreatePixelShader(m_pixelBlob_CoreMatte->GetBufferPointer(), m_pixelBlob_CoreMatte->GetBufferSize(),
		0, m_pixelShader_CoreMatte.ReleaseAndGetAddressOf());
	result = m_d3dDevice->CreatePixelShader(m_pixelBlob_Composite->GetBufferPointer(), m_pixelBlob_Composite->GetBufferSize(),
		0, m_pixelShader_Composite.ReleaseAndGetAddressOf());
	result = m_d3dDevice->CreatePixelShader(m_pixelBlob_gamma->GetBufferPointer(), m_pixelBlob_gamma->GetBufferSize(),
		0, m_pixelShader_gamma.ReleaseAndGetAddressOf());//26添加
	result = m_d3dDevice->CreatePixelShader(m_pixelBlob_brightANDcontrast->GetBufferPointer(), m_pixelBlob_brightANDcontrast->GetBufferSize(),
		0, m_pixelShader_brightANDcontrast.ReleaseAndGetAddressOf());//26添加
	result = m_d3dDevice->CreatePixelShader(m_pixelBlob_sharp->GetBufferPointer(), m_pixelBlob_sharp->GetBufferSize(),
		0, m_pixelShader_sharp.ReleaseAndGetAddressOf());//26添加
	/****************28修改开始****************/
	result = m_d3dDevice->CreatePixelShader(m_pixelBlob_binarization->GetBufferPointer(), m_pixelBlob_binarization->GetBufferSize(),
		0, m_pixelShader_binarization.ReleaseAndGetAddressOf());
	/****************28修改结束****************/
}
void QtGuiClass::InitResource(){
	/*注意四个顶点的给出顺序应当按顺时针排布*/
	/*绘制四边形所用的顶点*/
	VertexPosColor vertices[] =
	{
		//4p->Rectangle 位置坐标+颜色坐标+纹理坐标
		{ XMFLOAT3(-1.0f,  1.0f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f),XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3( 1.0f,  1.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3( 1.0f, -1.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),XMFLOAT2(1.0f, 1.0f) },
	};
	// 设置顶点缓冲区描述
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof vertices;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	// 新建顶点缓冲区
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	/*创建一个顶点缓冲区--用于顶点着色器*/
	m_d3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffer.ReleaseAndGetAddressOf());

	// 设置常量缓冲区描述-常量缓冲区的使用不需要创建资源视图
	D3D11_BUFFER_DESC cbd;
	ZeroMemory(&cbd, sizeof(cbd));
	cbd.Usage = D3D11_USAGE_DYNAMIC;/*常量缓冲区需要频繁更新*/
	//cbd.ByteWidth = sizeof(constantBuffer_matte);/*常量缓冲区必须是16的倍数*/
	cbd.ByteWidth = 64;/*常量缓冲区必须是16的倍数*///26添加
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;/*允许CPU写入*/

	/*新建用于色度抠像和去除色彩溢出的常量缓冲区*/
	hr = m_d3dDevice->CreateBuffer(&cbd, nullptr, CBufferMatteDespill.ReleaseAndGetAddressOf());
	/*新建用于一些像素着色器的常量缓冲区*/
	hr = m_d3dDevice->CreateBuffer(&cbd, nullptr, CBufferOtherParameter.ReleaseAndGetAddressOf());
	/*新建用于垃圾遮罩的像素着色器的常量缓冲区*/
	hr = m_d3dDevice->CreateBuffer(&cbd, nullptr, GarbageCBuffer.ReleaseAndGetAddressOf());
	/*新建用于核心遮罩的像素着色器的常量缓冲区*/
	hr = m_d3dDevice->CreateBuffer(&cbd, nullptr, CoreCBuffer.ReleaseAndGetAddressOf());

	// 设置第二个常量缓冲区描述用于羽化
	D3D11_BUFFER_DESC cbd2;
	ZeroMemory(&cbd2, sizeof(cbd2));
	cbd2.Usage = D3D11_USAGE_DYNAMIC;/*常量缓冲区需要频繁更新*/
	cbd2.ByteWidth = 384;/*常量缓冲区必须是16的倍数*/
	cbd2.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd2.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;/*允许CPU写入*/
	/*新建用于羽化的常量缓冲区*/
	hr = m_d3dDevice->CreateBuffer(&cbd2, nullptr, CBufferFeather.ReleaseAndGetAddressOf());

	// 初始化常量缓冲区的值---在paintEvent函数中设置
	
	// 初始化纹理和采样器状态---纹理也在paint函数中设置
	/*初始化采样器状态描述*/
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	//sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;/*线性采样*/
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;/*点采样*/
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	/*创建采样器状态*/
	hr = m_d3dDevice->CreateSamplerState(&sampDesc, m_pSamplerState.ReleaseAndGetAddressOf());
	// 像素着色阶段设置好采样器
	m_d3dDevContext->PSSetSamplers(0, 1, m_pSamplerState.GetAddressOf());/*这样HLSL中对应的采样器状态就可以使用了*/

	// 给渲染管线各个阶段绑定好所需资源
	// 输入装配阶段的顶点缓冲区设置
	UINT stride = sizeof(VertexPosColor);// 跨越字节数
	UINT offset = 0;// 起始偏移量
	//启用顶点数据--顶点缓冲区中的数据--类似于OpenGL中的VAO
	m_d3dDevContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);

	// 设置图元类型，设定输入布局
	m_d3dDevContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);/*绘制四边形*/
	m_d3dDevContext->IASetInputLayout(m_pVertexLayout.Get());

	/*设置2D纹理描述--可以绑定到渲染管线的，CPU不可读写，GPU可以*/
	texDESC.Width = width;/*纹理宽度*/
	texDESC.Height = height;/*纹理高度*/
	texDESC.MipLevels = 1;/*能够产生完整的mipmap，可以指定为0，不产生mipmap，则应当指定为1*/
	texDESC.ArraySize = 1;/*指定纹理的数目，单个纹理使用1*/
	texDESC.Format = DXGI_FORMAT_R8G8B8A8_UNORM;/*指定纹理存储的数据格式RGBA*/
	texDESC.SampleDesc.Count = 1;	// 不使用多重采样
	texDESC.SampleDesc.Quality = 0;
	texDESC.Usage = D3D11_USAGE_DEFAULT;/*指定数据的CPU/GPU访问权限，GPU可读可写*/
	texDESC.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;/*纹理可以作为渲染目标的输出点，并且指定它可以用于生成mipmaps*/
	texDESC.CPUAccessFlags = 0;
	texDESC.MiscFlags = 0;// 指定需要生成mipmap
	
	/*设置2D纹理描述--不可以绑定到渲染管线的，CPU可读写，主要是为了将像素数据读取到内存*/
	texDESC_cpu.Width = width;/*纹理宽度*/
	texDESC_cpu.Height = height;/*纹理高度*/
	texDESC_cpu.MipLevels = 1;/*能够产生完整的mipmap，可以指定为0，不产生mipmap，则应当指定为1*/
	texDESC_cpu.ArraySize = 1;/*指定纹理的数目，单个纹理使用1*/
	texDESC_cpu.Format = DXGI_FORMAT_R8G8B8A8_UNORM;/*指定纹理存储的数据格式RGBA*/
	texDESC_cpu.SampleDesc.Count = 1;	// 不使用多重采样
	texDESC_cpu.SampleDesc.Quality = 0;
	texDESC_cpu.Usage = D3D11_USAGE_STAGING;/*指定数据的CPU/GPU访问权限，GPU+CPU可读可写*/
	texDESC_cpu.BindFlags = 0;
	texDESC_cpu.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	texDESC_cpu.MiscFlags &= D3D11_RESOURCE_MISC_SHARED;/*必须为这个参数，否则会报错*/
	//texDESC.MiscFlags = 0;// 指定需要生成mipmap

	/*设置着色器资源视图的描述*/
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	/*设置纹理资源的描述*/
	sd.pSysMem = NULL;
	//sd.pSysMem = pixels;/*用于初始化的数据*/
	sd.SysMemPitch = width * sizeof(uint32_t);/*当前子资源一行所占的字节数(2D/3D纹理使用)*/
	sd.SysMemSlicePitch = width * height * sizeof(uint32_t);/*当前子资源一个完整切片所占的字节数(仅3D纹理使用)*/

	// ***********初始化混合状态***********
	/*对于两个相同位置的像素点，规定Csrc为源像素的颜色（从像素着色器输出的像素），
	Cdst为目标像素的颜色（已经存在于后备缓冲区上的像素）。*/
	//D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(blendDesc));
	auto& rtDesc = blendDesc.RenderTarget[0];

	// 透明混合模式
	// Color = SrcAlpha * SrcColor + (1 - SrcAlpha) * DestColor 
	// Alpha = SrcAlpha
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;
	rtDesc.BlendEnable = true;
	rtDesc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	rtDesc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	rtDesc.BlendOp = D3D11_BLEND_OP_ADD;
	rtDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
	rtDesc.DestBlendAlpha = D3D11_BLEND_ZERO;
	rtDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	rtDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;/*别忘了加上这个参数，否则为纯白色*/

	hr = m_d3dDevice->CreateBlendState(&blendDesc, BSAlphaToCoverage.GetAddressOf());
}
/*处理输出蒙版*/
void QtGuiClass::setupMatte() {
	//将顶点着色器和所需像素着色器绑定到渲染管线
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);/*使用这个顶点着色器*/
	m_d3dDevContext->PSSetShader(m_pixelShader_matte.Get(), nullptr, 0);/*使用这个片段着色器*/
	// 初始化常量缓冲区的值
	//在这个setConstantBuffers函数中初始化色度抠像需要的常量缓冲区的值
	//m_d3dDevContext->PSSetConstantBuffers(0, 1, CBufferMatteDespill.GetAddressOf());
	/*渲染到自定义蒙版纹理*/
	/*创建一个自定义2D空纹理作为渲染目标*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_matte.ReleaseAndGetAddressOf());/*初始化资源为空*/
	/*重新设置渲染目标绑定的纹理为此2D纹理--第一个RTV*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_matte.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	//将渲染目标视图和深度/模板缓冲区结合到管线
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*设置着色器资源视图*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*绘制四边形到渲染目标视图绑定的自定义纹理tex_matte*/
	m_d3dDevContext->Draw(4, 0);
	nowTex = tex_matte;/*当前中间纹理为黑白蒙版*/
	/*创建当前中间纹理的着色器资源视图SRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*清理DSV-深度视图*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
/*处理输出前景*/
void QtGuiClass::setupForeImage() {
	/*第二次绘制-使用第二个像素着色器-将前景输出到自定义二D纹理tex_fore*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_fore.ReleaseAndGetAddressOf());/*初始化资源为空*/
	//将顶点着色器和第2个像素着色器绑定到渲染管线
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);/*使用这个顶点着色器*/
	m_d3dDevContext->PSSetShader(m_pixelShader_fore.Get(), nullptr, 0);/*使用这个片段着色器*/

	/*4.重新设置渲染目标绑定的纹理为此2D纹理--第一个RTV*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_fore.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	// 将渲染目标视图和深度/模板缓冲区结合到管线
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	
	/*10. 设置第二个像素着色器资源--让该资源视图绑定到渲染管线的指定阶段*/
	/*注意：设置中间纹理为着色器资源必须在重新绑定RTV之后，因为中间纹理只能作为输出或者输入*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());/*这样在HLSL里对应regisgter(t1)的gTex_now存放的就是中间纹理*/
	m_d3dDevContext->PSSetShaderResources(0, 1, texInputSRV.GetAddressOf());
	/*设置去除绿色溢出所用的参数对应的常量缓冲区*/
	//m_d3dDevContext->PSSetConstantBuffers(0, 1, CBufferMatteDespill.GetAddressOf());
	/*绘制四边形到渲染目标视图绑定的纹理--后备缓冲区*/
	m_d3dDevContext->Draw(4, 0);
	nowTex = tex_fore;/*当前中间纹理为黑白蒙版*/
	/*创建当前中间纹理的着色器资源视图SRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	
	/*6. 清理DSV-深度视图*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

/*设置预模糊*/
void QtGuiClass::setupPreBlur() {
	/*先从RGB转换为YCC颜色模式，再对X轴进行模糊，再对Y轴进行模糊，再将YCC转换为RGB模式*/
	/*第一步：创建一个空的2D纹理用作渲染目标*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_preBlur.ReleaseAndGetAddressOf());/*初始化资源为空*/
	/*第二步：设置要用到的顶点着色器和像素着色器*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dDevContext->PSSetShader(m_pixelShader_preBlur.Get(), nullptr, 0);
	/*第三步：创建渲染目标视图并绑定到渲染管线*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_preBlur.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*第四步：设置对应像素着色器的输入纹理资源--对应第一个寄存器*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*第五步：设置对应像素着色器的常量缓冲区资源*/
//	m_d3dDevContext->PSSetConstantBuffers(2, 1, m_pConstantBuffer_parameter.GetAddressOf());
	/*第六步：绘制到自定义2D纹理*/
	m_d3dDevContext->Draw(4, 0);
	/*第七步：将当前纹理指针指向自定义纹理并创建对应着色器资源视图*/
	nowTex = tex_preBlur;/*当前中间纹理为对Y轴模糊后的纹理*/
	/*创建当前中间纹理的着色器资源视图SRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*第八步：释放渲染目标视图并清空深度模板缓存*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
/*设置钳制黑色白色*/
void QtGuiClass::setupClip() {
	/*第一步：创建一个空的2D纹理用作渲染目标*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_clip.ReleaseAndGetAddressOf());/*初始化资源为空*/
	/*第二步：设置要用到的顶点着色器和像素着色器*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dDevContext->PSSetShader(m_pixelShader_clip.Get(), nullptr, 0);
	/*第三步：创建渲染目标视图并绑定到渲染管线*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_clip.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*第四步：设置对应像素着色器的输入纹理资源--对应第一个寄存器*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*第五步：设置对应像素着色器的常量缓冲区资源*/
	// PS常量缓冲区对应HLSL寄存于b2的常量缓冲区--也就是将m_pConstantBuffers_despill的内容映射给了第4个像素着色器的常量缓冲区
	//m_d3dDevContext->PSSetConstantBuffers(2, 1, m_pConstantBuffer_parameter.GetAddressOf());
	/*第六步：绘制到自定义2D纹理*/
	m_d3dDevContext->Draw(4, 0);
	/*第七步：将当前纹理指针指向自定义纹理并创建对应着色器资源视图*/
	nowTex = tex_clip;/*当前中间纹理为黑白蒙版*/
	/*创建当前中间纹理的着色器资源视图SRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*第八步：释放渲染目标视图并清空深度模板缓存*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
/*设置颜色校正*///26添加
void QtGuiClass::setupGamma() {
	/*第一步：创建一个空的2D纹理用作渲染目标*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_gamma.ReleaseAndGetAddressOf());/*初始化资源为空*/
	/*第二步：设置要用到的顶点着色器和像素着色器*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dDevContext->PSSetShader(m_pixelShader_gamma.Get(), nullptr, 0);
	/*第三步：创建渲染目标视图并绑定到渲染管线*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_gamma.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*第四步：设置对应像素着色器的输入纹理资源--对应第一个寄存器*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*第五步：设置对应像素着色器的常量缓冲区资源*/
	// PS常量缓冲区对应HLSL寄存于b2的常量缓冲区--也就是将m_pConstantBuffers_despill的内容映射给了第4个像素着色器的常量缓冲区
	//m_d3dDevContext->PSSetConstantBuffers(2, 1, m_pConstantBuffer_parameter.GetAddressOf());
	/*第六步：绘制到自定义2D纹理*/
	m_d3dDevContext->Draw(4, 0);
	/*第七步：将当前纹理指针指向自定义纹理并创建对应着色器资源视图*/
	nowTex = tex_gamma;/*当前中间纹理为黑白蒙版*/
	/*创建当前中间纹理的着色器资源视图SRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*第八步：释放渲染目标视图并清空深度模板缓存*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
/****************28修改开始****************/
/*设置二值化*/
void QtGuiClass::setupBinarization() {
	/*第一步：创建一个空的2D纹理用作渲染目标*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_binarization.ReleaseAndGetAddressOf());/*初始化资源为空*/
	/*第二步：设置要用到的顶点着色器和像素着色器*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dDevContext->PSSetShader(m_pixelShader_binarization.Get(), nullptr, 0);
	/*第三步：创建渲染目标视图并绑定到渲染管线*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_binarization.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*第四步：设置对应像素着色器的输入纹理资源--对应第一个寄存器*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*第五步：设置对应像素着色器的常量缓冲区资源*/
	// PS常量缓冲区对应HLSL寄存于b2的常量缓冲区--也就是将m_pConstantBuffers_despill的内容映射给了第4个像素着色器的常量缓冲区
	//m_d3dDevContext->PSSetConstantBuffers(2, 1, m_pConstantBuffer_parameter.GetAddressOf());
	/*第六步：绘制到自定义2D纹理*/
	m_d3dDevContext->Draw(4, 0);
	/*第七步：将当前纹理指针指向自定义纹理并创建对应着色器资源视图*/
	nowTex = tex_binarization;/*当前中间纹理为黑白蒙版*/
					   /*创建当前中间纹理的着色器资源视图SRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*第八步：释放渲染目标视图并清空深度模板缓存*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
/****************28修改结束****************/

/*设置亮度和对比度增强*///26添加
void QtGuiClass::setupbrightANDcontrast() {
	/*第一步：创建一个空的2D纹理用作渲染目标*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_brightANDcontrast.ReleaseAndGetAddressOf());/*初始化资源为空*/
	/*第二步：设置要用到的顶点着色器和像素着色器*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dDevContext->PSSetShader(m_pixelShader_brightANDcontrast.Get(), nullptr, 0);
	/*第三步：创建渲染目标视图并绑定到渲染管线*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_brightANDcontrast.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*第四步：设置对应像素着色器的输入纹理资源--对应第一个寄存器*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*第五步：设置对应像素着色器的常量缓冲区资源*/
	// PS常量缓冲区对应HLSL寄存于b2的常量缓冲区--也就是将m_pConstantBuffers_despill的内容映射给了第4个像素着色器的常量缓冲区
	//m_d3dDevContext->PSSetConstantBuffers(2, 1, m_pConstantBuffer_parameter.GetAddressOf());
	/*第六步：绘制到自定义2D纹理*/
	m_d3dDevContext->Draw(4, 0);
	/*第七步：将当前纹理指针指向自定义纹理并创建对应着色器资源视图*/
	nowTex = tex_brightANDcontrast;/*当前中间纹理为黑白蒙版*/
	/*创建当前中间纹理的着色器资源视图SRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*第八步：释放渲染目标视图并清空深度模板缓存*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
/*设置锐化*///26添加
void QtGuiClass::setupSharp() {
	/*第一步：创建一个空的2D纹理用作渲染目标*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_sharp.ReleaseAndGetAddressOf());/*初始化资源为空*/
	/*第二步：设置要用到的顶点着色器和像素着色器*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dDevContext->PSSetShader(m_pixelShader_sharp.Get(), nullptr, 0);
	/*第三步：创建渲染目标视图并绑定到渲染管线*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_sharp.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*第四步：设置对应像素着色器的输入纹理资源--对应第一个寄存器*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*第五步：设置对应像素着色器的常量缓冲区资源*/
	// PS常量缓冲区对应HLSL寄存于b2的常量缓冲区--也就是将m_pConstantBuffers_despill的内容映射给了第4个像素着色器的常量缓冲区
	//m_d3dDevContext->PSSetConstantBuffers(2, 1, m_pConstantBuffer_parameter.GetAddressOf());
	/*第六步：绘制到自定义2D纹理*/
	m_d3dDevContext->Draw(4, 0);
	/*第七步：将当前纹理指针指向自定义纹理并创建对应着色器资源视图*/
	nowTex = tex_sharp;/*当前中间纹理为黑白蒙版*/
	/*创建当前中间纹理的着色器资源视图SRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*第八步：释放渲染目标视图并清空深度模板缓存*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

/*设置垃圾遮罩*/
void QtGuiClass::setupGarbageMatte() {
	/*第一步：创建一个空的2D纹理用作渲染目标*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_Garbage.ReleaseAndGetAddressOf());/*初始化资源为空*/
	/*第二步：设置要用到的顶点着色器和像素着色器*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dDevContext->PSSetShader(m_pixelShader_GarbageMatte.Get(), nullptr, 0);
	/*第三步：创建渲染目标视图并绑定到渲染管线*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_Garbage.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*第四步：设置对应像素着色器的输入纹理资源--对应第一个寄存器*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*第五步：设置对应像素着色器的常量缓冲区资源*/
	/*提前设置好了*/
	/*第六步：绘制到自定义2D纹理*/
	m_d3dDevContext->Draw(4, 0);
	/*第七步：将当前纹理指针指向自定义纹理并创建对应着色器资源视图*/
	nowTex = tex_Garbage;/*当前中间纹理为黑白蒙版*/
	/*创建当前中间纹理的着色器资源视图SRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*第八步：释放渲染目标视图并清空深度模板缓存*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
/*设置核心遮罩*/
void QtGuiClass::setupCoreMatte() {
	/*第一步：创建一个空的2D纹理用作渲染目标*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_Core.ReleaseAndGetAddressOf());/*初始化资源为空*/
	/*第二步：设置要用到的顶点着色器和像素着色器*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dDevContext->PSSetShader(m_pixelShader_CoreMatte.Get(), nullptr, 0);
	/*第三步：创建渲染目标视图并绑定到渲染管线*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_Core.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*第四步：设置对应像素着色器的输入纹理资源--对应第一个寄存器*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*第五步：设置对应像素着色器的常量缓冲区资源*/
	/*提前设置好*/
	/*第六步：绘制到自定义2D纹理*/
	m_d3dDevContext->Draw(4, 0);
	/*第七步：将当前纹理指针指向自定义纹理并创建对应着色器资源视图*/
	nowTex = tex_Core;/*当前中间纹理为黑白蒙版*/
	/*创建当前中间纹理的着色器资源视图SRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*第八步：释放渲染目标视图并清空深度模板缓存*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

/*设置后期模糊*/
void QtGuiClass::setupPostBlur() {
	/*第一步：创建一个空的2D纹理用作渲染目标*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_postBlur.ReleaseAndGetAddressOf());/*初始化资源为空*/
	/*第二步：设置要用到的顶点着色器和像素着色器*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dDevContext->PSSetShader(m_pixelShader_postBlur.Get(), nullptr, 0);
	/*第三步：创建渲染目标视图并绑定到渲染管线*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_postBlur.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*第四步：设置对应像素着色器的输入纹理资源--对应第一个寄存器*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*第五步：设置对应像素着色器的常量缓冲区资源*/
	// PS常量缓冲区对应HLSL寄存于b2的常量缓冲区--也就是将m_pConstantBuffers_despill的内容映射给了第4个像素着色器的常量缓冲区
//	m_d3dDevContext->PSSetConstantBuffers(2, 1, m_pConstantBuffer_parameter.GetAddressOf());
	/*第六步：绘制到自定义2D纹理*/
	m_d3dDevContext->Draw(4, 0);
	/*第七步：将当前纹理指针指向自定义纹理并创建对应着色器资源视图*/
	nowTex = tex_postBlur;/*当前中间纹理为黑白蒙版*/
	/*创建当前中间纹理的着色器资源视图SRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*第八步：释放渲染目标视图并清空深度模板缓存*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
/*设置膨胀腐蚀*/
void QtGuiClass::setupDilateErode() {
	/*第一步：创建一个空的2D纹理用作渲染目标*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_dilateErode.ReleaseAndGetAddressOf());/*初始化资源为空*/
	/*第二步：设置要用到的顶点着色器和像素着色器*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dDevContext->PSSetShader(m_pixelShader_dilateErode.Get(), nullptr, 0);
	/*第三步：创建渲染目标视图并绑定到渲染管线*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_dilateErode.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*第四步：设置对应像素着色器的输入纹理资源--对应第一个寄存器*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*第五步：设置对应像素着色器的常量缓冲区资源*/
	// PS常量缓冲区对应HLSL寄存于b2的常量缓冲区--也就是将m_pConstantBuffers_despill的内容映射给了第4个像素着色器的常量缓冲区
//	m_d3dDevContext->PSSetConstantBuffers(2, 1, m_pConstantBuffer_parameter.GetAddressOf());
	/*第六步：绘制到自定义2D纹理*/
	m_d3dDevContext->Draw(4, 0);
	/*第七步：将当前纹理指针指向自定义纹理并创建对应着色器资源视图*/
	nowTex = tex_dilateErode;/*当前中间纹理为黑白蒙版*/
							 /*创建当前中间纹理的着色器资源视图SRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*第八步：释放渲染目标视图并清空深度模板缓存*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
/*设置X或Y轴的羽化*/
void QtGuiClass::setupFeatherXorY(int flag) {
	/*flag=1则表示对X轴进行处理，flag=2则表示对Y轴进行处理*/
	/*第一步：创建一个空的2D纹理用作渲染目标*/
	if (flag == 1) {/*X轴*/
		hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_featherX.ReleaseAndGetAddressOf());/*初始化资源为空*/
	}
	else {
		hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_featherY.ReleaseAndGetAddressOf());/*初始化资源为空*/
	}
	/*第二步：设置要用到的顶点着色器和像素着色器*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	ComPtr<ID3D11RenderTargetView> nowRTV;
	if (flag == 1) {/*X轴*/
		m_d3dDevContext->PSSetShader(m_pixelShader_featherX.Get(), nullptr, 0);
		/*第三步：创建渲染目标视图并绑定到渲染管线*/
		hr = m_d3dDevice->CreateRenderTargetView(tex_featherX.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	}
	else {
		m_d3dDevContext->PSSetShader(m_pixelShader_featherY.Get(), nullptr, 0);
		/*第三步：创建渲染目标视图并绑定到渲染管线*/
		hr = m_d3dDevice->CreateRenderTargetView(tex_featherY.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	}
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*第四步：设置对应像素着色器的输入纹理资源--对应第一个寄存器*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*第五步：设置对应像素着色器的常量缓冲区资源*/
	// PS常量缓冲区对应HLSL寄存于b2的常量缓冲区--也就是将m_pConstantBuffers_despill的内容映射给了第4个像素着色器的常量缓冲区
//	m_d3dDevContext->PSSetConstantBuffers(2, 1, m_pConstantBuffer_parameter.GetAddressOf());
	/*第六步：绘制到自定义2D纹理*/
	m_d3dDevContext->Draw(4, 0);
	/*第七步：将当前纹理指针指向自定义纹理并创建对应着色器资源视图*/
	if (flag == 1) {/*X轴*/
		nowTex = tex_featherX;
	}
	else {
		nowTex = tex_featherY;
	}
	/*创建当前中间纹理的着色器资源视图SRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*第八步：释放渲染目标视图并清空深度模板缓存*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
/*设置羽化*/
void QtGuiClass::setupFeather() {
	/*羽化X轴*/
	this->setupFeatherXorY(1);
	/*羽化Y轴*/
	this->setupFeatherXorY(2);
}
/*设置背景着色器资源视图*/
void QtGuiClass::setComposite(bool composite, string filename_back) {
	this->composite = composite;
	this->filename_bg = filename_back;
	if (this->composite == true) {
		/*新建一个背景纹理并创建着色器资源视图*/
		/*第一种方式读取背景图像*/
		//wchar_t *str = multiByteToWideChar(this->filename_bg);
		//hr = CreateWICTextureFromFile(m_d3dDevice.Get(), str, nullptr, texBackSRV.ReleaseAndGetAddressOf());/*L"F:\\yellow.jpg"*/
		//delete[]str;
		/*第二种方式读取背景图像*/
		Mat src_back = imread(this->filename_bg);
		cvtColor(src_back, src_back, COLOR_BGR2RGBA);/*必须转换为RGBA模式，否则输出有误*/
		int width_back = src_back.cols;
		int height_back = src_back.rows;
		uint32_t * pixels_back = new uint32_t[width_back*height_back * 4];
		memcpy(pixels_back, src_back.data, width_back*height_back * 4);
		src_back.release();
		sd.pSysMem = pixels_back;/*用于初始化的数据*/
		/*将纹理描述的纹理大小重新设置为背景图片的大小*/
		texDESC.Width = width_back;/*背景纹理宽度*/
		texDESC.Height = height_back;/*背景纹理高度*/
		/*修改背景图对应的sd*/
		sd.SysMemPitch = width_back * sizeof(uint32_t);/*当前子资源一行所占的字节数(2D/3D纹理使用)*/
		sd.SysMemSlicePitch = width_back * height_back * sizeof(uint32_t);/*当前子资源一个完整切片所占的字节数(仅3D纹理使用)*/
		hr = m_d3dDevice->CreateTexture2D(&texDESC, &sd, tex_back.ReleaseAndGetAddressOf());
		hr = m_d3dDevice->CreateShaderResourceView(tex_back.Get(), &srvDesc, texBackSRV.ReleaseAndGetAddressOf());
		free(pixels_back);
		/*重新修改2D纹理描述的大小*/
		texDESC.Width = width;/*输入纹理宽度*/
		texDESC.Height = height;/*输入纹理高度*/
		sd.SysMemPitch = width * sizeof(uint32_t);
		sd.SysMemSlicePitch = width * height * sizeof(uint32_t);
	}
}
/*合成前景背景*/
void QtGuiClass::setupComposite() {
	/*新建一个背景纹理并创建着色器资源视图*/
	/*第一步：创建一个空的2D纹理用作渲染目标*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_Composite.ReleaseAndGetAddressOf());/*初始化资源为空*/
	/*第二步：设置要用到的顶点着色器和像素着色器*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dDevContext->PSSetShader(m_pixelShader_Composite.Get(), nullptr, 0);
	/*第三步：创建渲染目标视图并绑定到渲染管线*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_Composite.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*第四步：设置对应像素着色器的输入纹理资源--对应第一个寄存器*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	m_d3dDevContext->PSSetShaderResources(2, 1, texBackSRV.GetAddressOf());
	/*第五步：设置对应像素着色器的常量缓冲区资源*/
	// PS常量缓冲区对应HLSL寄存于b2的常量缓冲区--也就是将m_pConstantBuffers_despill的内容映射给了第4个像素着色器的常量缓冲区
	//m_d3dDevContext->PSSetConstantBuffers(2, 1, m_pConstantBuffer_parameter.GetAddressOf());
	/*第六步：绘制到自定义2D纹理*/
	m_d3dDevContext->Draw(4, 0);
	/*第七步：将当前纹理指针指向自定义纹理并创建对应着色器资源视图*/
	nowTex = tex_Composite;/*当前中间纹理为黑白蒙版*/
	/*创建当前中间纹理的着色器资源视图SRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*第八步：释放渲染目标视图并清空深度模板缓存*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
/*设置常量缓冲区的值*/
void QtGuiClass::setConstantBuffers() {
	/*设置常量缓冲区资源---钳制黑色白色等所需的参数*/
	m_parameter.size = this->preBlurValue;/*1.0*/
	m_parameter.clipBlack = this->clipBlack;
	m_parameter.clipWhite = this->clipWhite;
	m_parameter.kernelRadius = this->kernelRadius;
	m_parameter.kernelTolerance = this->kernelTolerance;
	m_parameter.size_postBlur = this->blur_post;
	m_parameter.gamma = this->gamma;//26添加
	m_parameter.brightness = this->brightness;//26添加
	m_parameter.contrast = this->contrast;//26添加
	m_parameter.sharp_value = this->sharp_value;//26添加
	/****************28修改开始****************/
	m_parameter.binarization_threshold = this->binarization_threshold;
	/****************28修改结束****************/

	if (this->distance > 0) {
		m_parameter.distance = this->distance;
		m_parameter.isDilateErode = 1;/*表示膨胀*/
	}
	else {
		m_parameter.distance = -this->distance;
		m_parameter.isDilateErode = 2;/*表示腐蚀*/
	}
	if (this->feather_distance > 0) {
		m_parameter.do_subtract = 0;/*羽化距离大于0是0*/
		m_parameter.sizeFeather = this->feather_distance;
	}
	else {
		m_parameter.do_subtract = 1;/*羽化距离小于0是1*/
		m_parameter.sizeFeather = -this->feather_distance;
	}
	m_parameter.falloff = this->feather_falloff;

	// 更新常量缓冲区资源--将CPU数据映射到GPU
	D3D11_MAPPED_SUBRESOURCE mappedData2;
	hr = m_d3dDevContext->Map(CBufferOtherParameter.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData2);
	memcpy_s(mappedData2.pData, sizeof(constantBuffer_parameter), &m_parameter, sizeof(constantBuffer_parameter));
	m_d3dDevContext->Unmap(CBufferOtherParameter.Get(), 0);

	//抠像和去除色彩溢出所需要的参数
	matteDespillCB.screen_balance = this->ScreenBalanceValue;
	matteDespillCB.screen_color = DirectX::XMFLOAT4(this->ScreenColorValue[0], this->ScreenColorValue[1], this->ScreenColorValue[2], this->ScreenColorValue[3]);
		//设置背景####################################################################待修改
	//matteDespillCB.screen_color = DirectX::XMFLOAT4(98.0f/255, 175.0f/255, 136.0f/255, 1.0f);


	matteDespillCB.primary_channel = max_axis_v3(this->ScreenColorValue);
	matteDespillCB.despill_factor = this->DespillFactor;/*1.0*/
	matteDespillCB.despill_balance = this->DespillBalance;/*0.5*/
	// 更新PS常量缓冲区资源--将CPU数据映射到GPU
	D3D11_MAPPED_SUBRESOURCE mappedData;
	hr = m_d3dDevContext->Map(CBufferMatteDespill.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);
	memcpy_s(mappedData.pData, sizeof(MatteDespillCB), &matteDespillCB, sizeof(MatteDespillCB));
	m_d3dDevContext->Unmap(CBufferMatteDespill.Get(), 0);
	/*设置渲染管线中常量缓冲区对应寄存器b0的值--用于色度抠像和去除色彩溢出*/
	m_d3dDevContext->PSSetConstantBuffers(0, 1, CBufferMatteDespill.GetAddressOf());
	/*设置渲染管线中常量缓冲区对应寄存器b2的值--用于除上述功能以及垃圾遮罩和核心遮罩以外功能的参数*/
	m_d3dDevContext->PSSetConstantBuffers(2, 1, CBufferOtherParameter.GetAddressOf());

	/*设置垃圾遮罩所需的常量缓冲区资源*/
	if (this->isGarbageMatte) {
		parameterGarbage.xBox = this->x_Garbage;
		parameterGarbage.yBox = this->y_Garbage;
		parameterGarbage.rotationBox = this->rotation_Garbage;
		parameterGarbage.heightBox = this->height_Garbage;
		parameterGarbage.widthBox = this->width_Garbage;
		parameterGarbage.maskType = this->maskType_Garbage;/*相加还是什么*/
		parameterGarbage.transparentBox = this->transparent_Garbage;
		if (this->isBoxMask_Garbage) {
			parameterGarbage.isBox = 1;/*1代表方形，2代表椭圆形*/
		}
		else {
			parameterGarbage.isBox = 2;/*1代表方形，2代表椭圆形*/
		}
		//parameterGarbage.isGarbage = 1;/*1代表垃圾，2代表核心*/
								   // 更新常量缓冲区资源--将CPU数据映射到GPU
		D3D11_MAPPED_SUBRESOURCE mappedDataGarbage;
		hr = m_d3dDevContext->Map(GarbageCBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedDataGarbage);
		memcpy_s(mappedDataGarbage.pData, sizeof(cBufferGarbageCore), &parameterGarbage, sizeof(cBufferGarbageCore));
		m_d3dDevContext->Unmap(GarbageCBuffer.Get(), 0);

		/*设置渲染管线中常量缓冲区对应寄存器b3的值--用于垃圾遮罩的参数*/
		m_d3dDevContext->PSSetConstantBuffers(3, 1, GarbageCBuffer.GetAddressOf());
	}
	/*设置核心遮罩所需的常量缓冲区资源*/
	if (this->isCoreMatte) {
		parameterCore.xBox = this->x_Core;
		parameterCore.yBox = this->y_Core;
		parameterCore.rotationBox = this->rotation_Core;
		parameterCore.heightBox = this->height_Core;
		parameterCore.widthBox = this->width_Core;
		parameterCore.maskType = this->maskType_Core;/*相加还是什么*/
		parameterCore.transparentBox = this->transparent_Core;
		if (this->isBoxMask_Core) {
			parameterCore.isBox = 1;/*1代表方形，2代表椭圆形*/
		}
		else {
			parameterCore.isBox = 2;/*1代表方形，2代表椭圆形*/
		}
		//parameterGarbage.isGarbage = 2;/*1代表垃圾，2代表核心*/
		// 更新常量缓冲区资源--将CPU数据映射到GPU
		D3D11_MAPPED_SUBRESOURCE mappedDataCore;
		hr = m_d3dDevContext->Map(CoreCBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedDataCore);
		memcpy_s(mappedDataCore.pData, sizeof(cBufferGarbageCore), &parameterCore, sizeof(cBufferGarbageCore));
		m_d3dDevContext->Unmap(CoreCBuffer.Get(), 0);

		/*设置渲染管线中常量缓冲区对应寄存器b1的值--用于核心遮罩的参数*/
		m_d3dDevContext->PSSetConstantBuffers(1, 1, CoreCBuffer.GetAddressOf());
	}

	/*设置羽化的参数数组*/
	if (this->feather_distance != 0) {
		int rad = max(m_parameter.sizeFeather, 0);
		int filtersize = min(rad, 30000);
		/*获取正太分布的高斯数组*/
		make_gausstab(rad, filtersize);
		/*根据羽化衰减类型的不同定义权重*/
		make_dist_fac_inverse(rad, filtersize, m_parameter.falloff);
		/*目前已经给featherInput结构体的两个数组赋好值了*/
		/*接下来如何将结构体传递到HLSL中*/
		// 更新常量缓冲区资源--将CPU数据映射到GPU
		D3D11_MAPPED_SUBRESOURCE mappedDataFeather;
		hr = m_d3dDevContext->Map(CBufferFeather.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedDataFeather);
		memcpy_s(mappedDataFeather.pData, sizeof(Feather), &featherParameter, sizeof(Feather));
		m_d3dDevContext->Unmap(CBufferFeather.Get(), 0);
		/*设置渲染管线中常量缓冲区对应寄存器b4的值--用于羽化数组的参数*/
		m_d3dDevContext->PSSetConstantBuffers(4, 1, CBufferFeather.GetAddressOf());
	}
}
/*用QT开始绘制*/
void QtGuiClass::paintEvent(QPaintEvent *event)
{
	//qDebug() << "----paint start" << endl;
	/*抠像开始时间*/
	LARGE_INTEGER beginTime_matte = { 0 };
	QueryPerformanceFrequency(&beginTime_matte);
	//电脑CPU时钟频率
	double pcFreq_matte = (double)beginTime_matte.QuadPart / 1000000.0;
	QueryPerformanceCounter(&beginTime_matte);

	/*在每一帧画面绘制的操作中，我们需要清理一遍渲染目标视图绑定的缓冲区*/
	m_d3dDevContext->ClearRenderTargetView(m_renderTargetView.Get(), bgcolor);
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	/*初始化一张2D纹理作为输入*/
	sd.pSysMem = pixels;/*用于初始化纹理的数据*/
	/*第四步：创建2D纹理并初始化*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, &sd, tex_input.ReleaseAndGetAddressOf());/*初始化一张纹理作为输入*/
	/*第五步：创建着色器资源视图*/
	hr = m_d3dDevice->CreateShaderResourceView(tex_input.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	hr = m_d3dDevice->CreateShaderResourceView(tex_input.Get(), &srvDesc, texInputSRV.ReleaseAndGetAddressOf());

	/*对输入的图像进行gamma校正*///26添加
	if (this->gamma != 0) {
		this->setupGamma();
	}

	/*对输入的图像进行锐化处理*///26添加
	if (this->sharp_value > 0) {
		this->setupSharp();
	}

	/*输入图像的预模糊操作*/
	if (this->preBlurValue > 0) {
		this->setupPreBlur();/*执行预模糊的shader*/
	}
	/*处理蒙版*/
	this->setupMatte();/*绘制蒙版到自定义2D纹理tex_matte*/

	/*对蒙版亮度和对比度增强*///26添加
	if (this->brightness > 0 || this->contrast > 0) {
		this->setupbrightANDcontrast();
	}

	/*设置钳制黑色白色*/
	if (this->clipBlack > 0.0f || this->clipWhite < 1.0f) {
		this->setupClip();
	}
	/* 设置垃圾遮罩*/
	if (this->isGarbageMatte) {
		this->setupGarbageMatte();
	}
	/* 设置核心遮罩*/
	if (this->isCoreMatte) {
		this->setupCoreMatte();
	}
	/*设置后期模糊*/
	if (this->blur_post) {
		this->setupPostBlur();
	}
	/*设置膨胀腐蚀*/
	if (this->distance != 0) {
		this->setupDilateErode();
	}
	/* 设置羽化 */
	if (this->feather_distance != 0) {
		this->setupFeather();
	}
	/****************28修改开始****************/
	/*在输出前景之前对蒙版进行二值化操作*/
	if (this->isBinary == true) {
		this->setupBinarization();
	}
	/****************28修改结束****************/

	/*处理前景*/
	if (this->flag == 1) {
		/*将输出前景和去除绿色溢出合成在一个着色器里*/
		this->setupForeImage();
		
		/*合成前景背景*/
		if (this->composite == true && this->filename_bg != "") {
			this->setupComposite();
		}
	}
	/*将上述处理结果原样输出到后备缓冲区*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);/*使用这个顶点着色器*/
	m_d3dDevContext->PSSetShader(m_pixelShader_output.Get(), nullptr, 0);/*使用这个片段着色器*/

	/*重新设置渲染目标绑定的纹理为后备缓冲区纹理*/
	hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBuffer.ReleaseAndGetAddressOf());
	hr = m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.ReleaseAndGetAddressOf());
	//将渲染目标视图和深度/模板缓冲区结合到管线
	m_d3dDevContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
	/*设置渲染状态--开启混合模式*/
	m_d3dDevContext->OMSetBlendState(BSAlphaToCoverage.Get(), nullptr, 0xFFFFFFFF);

	/*设置第3个像素着色器资源--让该着色器资源视图绑定到渲染管线的指定阶段*/
	/*注意：设置中间纹理为着色器资源必须在重新绑定RTV之后，因为中间纹理只能作为输出或者输入*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());/*这样在HLSL里对应regisgter(t1)的gTex_now存放的就是中间纹理*/
	/*绘制四边形到后备缓冲区*/
	m_d3dDevContext->Draw(4, 0);

	/*抠像结束时间*/
	LARGE_INTEGER endTime_matte = { 0 };
	QueryPerformanceCounter(&endTime_matte);
	//那么下面计算得出的就是这之间的时间间隔了，单位为微秒
	int time_matte = (endTime_matte.QuadPart - beginTime_matte.QuadPart) / pcFreq_matte;
	this->myMatteTime = this->myMatteTime + time_matte;

	if (this->ifKinect == false) {
		/*如果是对单张图片进行操作，则可以将结果保存到本地路径*/
		if (this->video == false) {
			/*输出截屏-保存后备缓冲区纹理到文件*/
			m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer_save.ReleaseAndGetAddressOf()));
			if (this->flag == 1) {/*前景*/
				hr = SaveWICTextureToFile(m_d3dDevContext.Get(), backBuffer_save.Get(), GUID_ContainerFormatPng, L"Screenshot\\output_fore.png");
			}
			else {/*蒙版*/
				hr = SaveWICTextureToFile(m_d3dDevContext.Get(), backBuffer_save.Get(), GUID_ContainerFormatPng, L"Screenshot\\output_matte.png");

				//ComPtr<ID3D11Texture2D> pStaging;
				//hr = m_d3dDevice->CreateTexture2D(&texDESC_cpu, nullptr, pStaging.ReleaseAndGetAddressOf());
				//ComPtr<ID3D11Texture2D> backBuffer_save;
				//m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer_save.ReleaseAndGetAddressOf()));
				//m_d3dDevContext->CopyResource(pStaging.Get(), backBuffer_save.Get());;/*复制资源到pStaging*/

				///*第二步：将pStaging读取到CPU的一个指针imgData中存储像素并返回QT*/
				//D3D11_MAPPED_SUBRESOURCE mapped_pixels;
				//hr = m_d3dDevContext->Map(pStaging.Get(), 0, D3D11_MAP_READ, 0, &mapped_pixels);
				//this->imgData = new uchar[width*height * 4];/*开辟空间*/
				///*RGBA不是连续的，每一行的数据都有填充，因此需要用以下方式去读取数据*/
				//unsigned char* pData = reinterpret_cast<unsigned char*>(mapped_pixels.pData);
				//for (UINT i = 0; i < height; ++i)
				//{
				//	memcpy_s(&this->imgData[i * width * 4], width * 4, pData, width * 4);/*别忘了*4*/
				//	pData += mapped_pixels.RowPitch;
				//}
				//m_d3dDevContext->Unmap(pStaging.Get(), 0);
				///*第三步，形成一个mat图像*/
				//Mat frame(height, width, CV_8UC4, this->imgData);
				//cvtColor(frame, frame, COLOR_RGBA2BGRA);
				//this->count2++;
				//qDebug() << "--------write count2" << count2 << endl;
				//frame.release();/*释放掉申请的frame内存*/
				//free(this->imgData);

			}
		}
		/*如果是对视频进行操作，则将后备缓冲区的纹理读取到指针中并写入视频*/
		if (this->video == true) {
			/*写帧开始时间*/
			LARGE_INTEGER beginTime = { 0 };
			QueryPerformanceFrequency(&beginTime);
			//电脑CPU时钟频率
			double pcFreq = (double)beginTime.QuadPart / 1000000.0;
			QueryPerformanceCounter(&beginTime);

			/*第一步：复制后备缓冲区纹理到一个CPU可读写的纹理pStaging*/
			ComPtr<ID3D11Texture2D> pStaging;
			hr = m_d3dDevice->CreateTexture2D(&texDESC_cpu, nullptr, pStaging.ReleaseAndGetAddressOf());
			ComPtr<ID3D11Texture2D> backBuffer_save;
			m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer_save.ReleaseAndGetAddressOf()));
			m_d3dDevContext->CopyResource(pStaging.Get(), backBuffer_save.Get());;/*复制资源到pStaging*/

			/*第二步：将pStaging读取到CPU的一个指针imgData中存储像素并返回QT*/
			D3D11_MAPPED_SUBRESOURCE mapped_pixels;
			hr = m_d3dDevContext->Map(pStaging.Get(), 0, D3D11_MAP_READ, 0, &mapped_pixels);
			this->imgData = new uchar[width*height * 4];/*开辟空间*/
			/*RGBA不是连续的，每一行的数据都有填充，因此需要用以下方式去读取数据*/
			unsigned char* pData = reinterpret_cast<unsigned char*>(mapped_pixels.pData);
			for (UINT i = 0; i < height; ++i)
			{
				memcpy_s(&this->imgData[i * width * 4], width * 4, pData, width * 4);/*别忘了*4*/
				pData += mapped_pixels.RowPitch;
			}
			m_d3dDevContext->Unmap(pStaging.Get(), 0);

			/*第三步，形成一个mat图像*/
			Mat frame(height, width, CV_8UC4, this->imgData);
			cvtColor(frame, frame, COLOR_RGBA2BGRA);
			this->count2++;
			writer << frame;//等同于writer.write(frame);
			//qDebug() << "--------write count2" << count2 << endl;
			frame.release();/*释放掉申请的frame内存*/
			free(this->imgData);

			/*写帧结束时间*/
			LARGE_INTEGER endTime = { 0 };
			QueryPerformanceCounter(&endTime);
			//那么下面计算得出的就是这之间的时间间隔了，单位为微秒
			int time = (endTime.QuadPart - beginTime.QuadPart) / pcFreq;
			this->writeAllFrameTime = this->writeAllFrameTime + time;
		}
	}
	//如果使用Kinect
	if (ifKinect == true) {
		/*第一步：复制后备缓冲区纹理到一个CPU可读写的纹理pStaging*/
		ComPtr<ID3D11Texture2D> pStaging;
		hr = m_d3dDevice->CreateTexture2D(&texDESC_cpu, nullptr, pStaging.ReleaseAndGetAddressOf());
		ComPtr<ID3D11Texture2D> backBuffer_save;
		m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer_save.ReleaseAndGetAddressOf()));
		m_d3dDevContext->CopyResource(pStaging.Get(), backBuffer_save.Get());/*复制资源到pStaging*/

		/*第二步：将pStaging读取到CPU的一个指针imgData中存储像素并返回QT*/
		D3D11_MAPPED_SUBRESOURCE mapped_pixels;
		hr = m_d3dDevContext->Map(pStaging.Get(), 0, D3D11_MAP_READ, 0, &mapped_pixels);
		this->imgData = new uchar[width*height * 4];/*开辟空间*/
		/*RGBA不是连续的，每一行的数据都有填充，因此需要用以下方式去读取数据*/
		unsigned char* pData = reinterpret_cast<unsigned char*>(mapped_pixels.pData);
		for (UINT i = 0; i < height; ++i)
		{
			memcpy_s(&this->imgData[i * width * 4], width * 4, pData, width * 4);/*别忘了*4*/
			pData += mapped_pixels.RowPitch;
		}
		m_d3dDevContext->Unmap(pStaging.Get(), 0);

		//发送RGBA数据。这里alpha值的范围是0-255
		//过滤灰色闪屏在updateFrame_Kinect()控制过了，能进到这儿就意味着不是灰色闪屏。可以放心发送
		sender_rgb.Send((char*)this->imgData, 1080 * 1920 * 4);
		////调试时候看一下图片
		//cv::Mat imgData_BGRA = cv::Mat(colorheight, colorwidth, CV_8UC4, this->imgData);
		//cvtColor(imgData_BGRA, imgData_BGRA, COLOR_BGRA2RGBA);
		//cv::imshow("dd", imgData_BGRA);
		//cv::imwrite("C:\\Users\\Administrator\\Desktop\\temp\\output1.png", imgData_BGRA);
		//imgData_BGRA.release();

		////将RGBA的RGB部分通过socket发送
		//unsigned char* imgData_rgb = new uchar[width*height * 3];
		//for (int i = 0; i < colorwidth * colorheight; i++) {//RGB转BGR
		//	if (imgData[4 * i + 3] >= 1) {//阈值暂时设为1
		//		imgData_rgb[3 * i] = imgData[4 * i + 0];
		//		imgData_rgb[3 * i + 1] = imgData[4 * i + 1];
		//		imgData_rgb[3 * i + 2] = imgData[4 * i + 2];
		//	}
		//	else {
		//		imgData_rgb[3 * i] = 1;
		//		imgData_rgb[3 * i + 1] = 1;
		//		imgData_rgb[3 * i + 2] = 1;
		//	}
		//}
		////过滤灰色闪屏在updateFrame_Kinect()控制过了，能进到这儿就意味着不是灰色闪屏。可以放心发送
		//sender_rgb.Send((char*)imgData_rgb, 1080 * 1920 * 3);
		////调试时候看一下图片
		//cv::Mat imgData_BGR = cv::Mat(colorheight, colorwidth, CV_8UC3, imgData_rgb);
		//cvtColor(imgData_BGR, imgData_BGR, COLOR_RGB2BGR);
		//cv::imshow("dd", imgData_BGR);

		this->count2++;
		free(this->imgData);
	}
	//每帧更新完都需要交换下前后台的缓冲区，把后台画好的东西显示到屏幕上
	m_swapChain->Present(0,0);	

}
/*ComPtr<ID3D11Debug> d3dDebug;
HRESULT hr = m_d3dDevice.As(&d3dDebug);
if (SUCCEEDED(hr))
{
hr = d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
}*/

/*清空纹理*/
//tex_input.Reset();
//tex_yccTorgb.Reset();
//tex_matte.Reset();
//tex_clip.Reset();
//tex_Garbage.Reset();
//tex_Core.Reset();
//tex_postBlurX.Reset();
//tex_postBlurY.Reset();
//tex_dilateErode.Reset();
//tex_featherX.Reset();
//tex_featherY.Reset();
//tex_fore.Reset();
//tex_despill.Reset();
//tex_Composite.Reset();

