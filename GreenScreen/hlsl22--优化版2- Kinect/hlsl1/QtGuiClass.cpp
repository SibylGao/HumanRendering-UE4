#include "QtGuiClass.h"
#include "strsafe.h"
#include <QResizeEvent>
#include <QTimer>
#include<qDebug>/*�������������Ϣ������̨*/
using namespace DirectX;/*������XMFLOAT3*/
	
////////////////////
///      socket
////////////////////
othka::comm::SocketTCP sender_rgb;
othka::comm::SocketTCP sender_depth;
//¼��Ƶ
//int videoFrameIndex = 0;
//cv::VideoWriter videoOut;

bool lock_depth_rgb_pair = true;//true��ʾ���Է���
/*�������벼��*/
const D3D11_INPUT_ELEMENT_DESC QtGuiClass::VertexPosColor::inputLayout[3] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};
/*���ر�����ɫ�����ֵͨ��*/
int max_axis_v3(float *ScreenColorValue) {
	float x = ScreenColorValue[0];
	float y = ScreenColorValue[1];
	float z = ScreenColorValue[2];
	return ((x > y) ? ((x > z) ? 0 : 2) : ((y > z) ? 1 : 2));
}
float RE_filter_value(int type, float x)/*type�Ǹ�˹*/
{
	float gaussfac = 1.6f;
	x = abs(x);
	float two_gaussfac2 = 2.0f * gaussfac * gaussfac;
	x *= 3.0f * gaussfac;
	return 1.0f / sqrt(3.14 * two_gaussfac2) * exp(-x * x / two_gaussfac2);
}
/*����һ��gausstab���鲢��ֵ*/
void QtGuiClass::make_gausstab(float rad, const int size)/*rab��size�����𻯾���*/
{
	float sum, val;
	int i, n;
	n = 2 * size + 1;/*�������󳤶ȣ����������޷����ٲ���������*/
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
/*��string����ת��Ϊwchar_t����*/
//��Ҫ������ʹ����wchar_t*��delete[]�ͷ��ڴ�
wchar_t *multiByteToWideChar(const string& pKey)
{
	const char* pCStrKey = pKey.c_str();
	//��һ�ε��÷���ת������ַ������ȣ�����ȷ��Ϊwchar_t*���ٶ����ڴ�ռ�
	int pSize = MultiByteToWideChar(CP_OEMCP, 0, pCStrKey, strlen(pCStrKey) + 1, NULL, 0);
	wchar_t *pWCStrKey = new wchar_t[pSize];
	//�ڶ��ε��ý����ֽ��ַ���ת����˫�ֽ��ַ���
	MultiByteToWideChar(CP_OEMCP, 0, pCStrKey, strlen(pCStrKey) + 1, pWCStrKey, pSize);
	return pWCStrKey;
}
/*���캯��*/
QtGuiClass::QtGuiClass(string filename, bool video,QWidget *parent) :QWidget(parent)
{
	ui.setupUi(this);
	/*���ʹ���û��Զ�����ƣ�����Ҫ����WA_PaintOnScreen*/
	QWidget::setAttribute(Qt::WA_PaintOnScreen); // ����DX��Ⱦ ...
	setFocusPolicy(Qt::WheelFocus); // ������ʱ�� ...
	setMouseTracking(true); // ��������ƶ� ...

	this->flag = 0;/*��ʼ������ɰ�*/
	//bgcolor[0] = 0;
	//bgcolor[1] = 0.5;
	//bgcolor[2] = 0.5;
	//bgcolor[3] = 1;
	bgcolor[0] = 1;
	bgcolor[1] = 1;
	bgcolor[2] = 1;
	bgcolor[3] = 1;
	this->video = video;
	/****************28�޸Ŀ�ʼ****************/
	this->isBinary = false;
	/****************28�޸Ľ���****************/
	this->count = 0;
	this->count2 = 0;
	/*���ö�ʱ������ʱ����*/
	m_pTimer = new QTimer(this);
	/*���ö�ʱ��ÿ�����ٺ��뷢��һ��timeout()�ź�*/
	/*���ʱ��������С��20ms����������֡������*/
	m_pTimer->setInterval(20);/*����ʱ����20����*/
	/*���Ӷ�ʱ���źźͲۺ���*/
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(updateFrame()));
	this->pixels = NULL;/*��ʼ����������ָ��*/
	/*ϵͳ��ʼʱ��*/
	QueryPerformanceFrequency(&dwStart);
	//����CPUʱ��Ƶ��
	pcFreqall = (double)dwStart.QuadPart / 1000000.0;
	QueryPerformanceCounter(&dwStart);

	this->readAllFrameTime = 0;
	this->writeAllFrameTime = 0;
	this->myMatteTime = 0;

	/*����һ֡��ʼʱ��*/
	LARGE_INTEGER beginTime = { 0 };
	QueryPerformanceFrequency(&beginTime);
	//����CPUʱ��Ƶ��
	double pcFreq = (double)beginTime.QuadPart / 1000000.0;
	QueryPerformanceCounter(&beginTime);

	if (video == true) {/*��ȡ��Ƶ��һ֡*/
		cap.open(filename); //����Ƶ
		if (!cap.isOpened())
			return;

		/*д����Ƶ*/
		Size size0 = Size(cap.get(CV_CAP_PROP_FRAME_WIDTH), cap.get(CV_CAP_PROP_FRAME_HEIGHT));
		writer.open("E://outDirectX1.avi", -1, cap.get(CV_CAP_PROP_FPS), size0, true);

		m_pTimer->start();/*��ʱ����ʼ*///����updateFrame�Ϳ�ʼ��������
		/*��ȡ��Ƶ��һ֡*/			
		cap >> src;//�ȼ���cap.read(frame);
		count++;
	//	qDebug() << "--------read count" << count << endl; 
		if (src.empty()) {//���ĳ֡Ϊ�����˳�ѭ��
			pixels = NULL;
		}
	}
	/****************29�޸Ŀ�ʼ****************/
	else {
		  /*��OpenCV��ȡһ��ͼƬ����������pixels--"F:\\20191219.jpg"*/
		src = imread(filename); //һ��cv2.imread()��ȡ��ͼƬ����BGR��ɫ�ռ��ͼƬ
		int channels = src.channels();/*��ȡ��ǰͼƬ��ͨ����*/
		if (channels == 3) {/*����ͼ����jpg��ʽ*/
			cvtColor(src, src, COLOR_BGR2RGBA);/*����ת��ΪRGBAģʽ�������������*/
		}
		if (channels == 4) {/*����ͼ����png��ʽ*/
			cvtColor(src, src, COLOR_BGRA2RGBA);/*����ת��ΪRGBAģʽ�������������*/
		}
	}
	/****************29�޸Ľ���****************/

	width = src.cols;
	height = src.rows;
	pixels = new uint32_t[width*height * 4];
	memcpy(pixels, src.data, width*height * 4);
	src.release();

	/*����һ֡����ʱ��*/
	LARGE_INTEGER endTime = { 0 };
	QueryPerformanceCounter(&endTime);
	//��ô�������ó��ľ�����֮���ʱ�����ˣ���λΪ΢��
	int time = (endTime.QuadPart - beginTime.QuadPart) / pcFreq;
	this->readAllFrameTime = this->readAllFrameTime + time;

	this->resize(QSize(width, height));/*���õ�ǰQT���ڴ�С*/

	InitD3D();
	InitEffect();
	InitResource();
}
//���ع��캯��
QtGuiClass::QtGuiClass(bool video, bool ifKinect, QWidget *parent) :QWidget(parent)
{
	//ʵʱչʾrgb
	//namedWindow("enhanced", 0);
	//resizeWindow("enhanced", 640, 480);
	//¼��Ƶ
	//videoOut.open("Video\\1.mov", CV_FOURCC('M', 'J', 'P', 'G'), 20, cv::Size(colorwidth, colorheight));

	//��ʼ��sender
	sender_rgb.Connect("127.0.0.1", 1234);
	sender_rgb.Blocking(false);

	sender_depth.Connect("127.0.0.1", 1235);
	sender_depth.Blocking(false);

	ui.setupUi(this);
	/*���ʹ���û��Զ�����ƣ�����Ҫ����WA_PaintOnScreen*/
	QWidget::setAttribute(Qt::WA_PaintOnScreen); // ����DX��Ⱦ ...
	setFocusPolicy(Qt::WheelFocus); // ������ʱ�� ...
	setMouseTracking(true); // ��������ƶ� ...

	this->flag = 0;/*��ʼ������ɰ�*/
	//bgcolor[0] = 0;
	//bgcolor[1] = 0.5;
	//bgcolor[2] = 0.5;
	//bgcolor[3] = 1;
	bgcolor[0] = 1;
	bgcolor[1] = 1;
	bgcolor[2] = 1;
	bgcolor[3] = 1;
	this->video = video;
	/****************28�޸Ŀ�ʼ****************/
	this->isBinary = false;
	/****************28�޸Ľ���****************/
	this->ifKinect = ifKinect;
	this->count = 0;
	this->count2 = 0;
	/*���ö�ʱ������ʱ����*/
	m_pTimer = new QTimer(this);
	/*���ö�ʱ��ÿ�����ٺ��뷢��һ��timeout()�ź�*/
	/*���ʱ��������С��20ms����������֡������*/
	m_pTimer->setInterval(20);/*����ʱ����20����*/
	/*���Ӷ�ʱ���źźͲۺ���*/
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(updateFrame_Kinect()));
	this->pixels = NULL;/*��ʼ����������ָ��*/
	/*ϵͳ��ʼʱ��*/
	QueryPerformanceFrequency(&dwStart);
	//����CPUʱ��Ƶ��
	pcFreqall = (double)dwStart.QuadPart / 1000000.0;
	QueryPerformanceCounter(&dwStart);

	this->readAllFrameTime = 0;
	this->writeAllFrameTime = 0;
	this->myMatteTime = 0;

	/*����һ֡��ʼʱ��*/
	LARGE_INTEGER beginTime = { 0 };
	QueryPerformanceFrequency(&beginTime);
	//����CPUʱ��Ƶ��
	double pcFreq = (double)beginTime.QuadPart / 1000000.0;
	QueryPerformanceCounter(&beginTime);

	//��ʼ��Kinect��Ҫ�����ʼ��Ҳ����hlsl1.cpp�Ĺ��캯�������������̶�������
	//Ϊʲô�����ŵ��ʼ�أ���ΪKinect������Ҫʱ�䡣���û�����Ͳ�������ݵĻ����ɼ��������������ȫ��ͼ�����ֵ��52685
	if (video == false && ifKinect == true) {
		m_pTimer->start();/*��ʱ����ʼ*/
		//src = imread("C:\\Users\\Administrator\\Desktop\\temp\\greenback2.jpg");//������Ҫ�ĳɴ�Kinect����ȡͼƬ��src
		//colorFlow = (unsigned char*)malloc(sizeof(unsigned char) * colorwidth * colorheight * 3);
		//depthFlow = (unsigned short *)malloc(sizeof(unsigned short)* colorwidth*colorheight);
		getKinectData(colorFlow, depthFlow);
		//src = cv::Mat(colorheight, colorwidth, CV_16UC1, depthFlow);//���ͼ����ת��mat�ģ�������������
		src = cv::Mat(colorheight, colorwidth, CV_8UC3, colorFlow);//��ɫͼ����src������ǰ����Ⱦ
		//imshow("dd", src);//��ʾ������˵������BGR
		//cv::waitKey(0);
		//socket��������
		if (depthFlow[0] != 52685 && colorFlow[0] != 205) {//���˻�ɫ��������ɫ��������ue4��GPU
			sender_depth.Send((char *)depthFlow, 1080 * 1920 * 2);//��Ϊunsigned short��16λ��char��8λ�����������ͨ��������2��
			//sender_rgb.Send((char*)colorFlow, 1080 * 1920 * 3);
		}


		count++;
		if (src.empty()) {//���ĳ֡Ϊ�����˳�ѭ��
			pixels = NULL;
		}
	}

	cvtColor(src, src, COLOR_BGR2RGBA);/*����ת��ΪRGBAģʽ�������������*/
	width = src.cols;
	height = src.rows;
	pixels = new uint32_t[width*height * 4];
	memcpy(pixels, src.data, width*height * 4);
	src.release();
	//free(depthFlow);//����src�˲ſ���free
	//free(colorFlow);
	/*����һ֡����ʱ��*/
	LARGE_INTEGER endTime = { 0 };
	QueryPerformanceCounter(&endTime);
	//��ô�������ó��ľ�����֮���ʱ�����ˣ���λΪ΢��
	int time = (endTime.QuadPart - beginTime.QuadPart) / pcFreq;
	this->readAllFrameTime = this->readAllFrameTime + time;

	this->resize(QSize(width, height));/*���õ�ǰQT���ڴ�С*/

	InitD3D();
	InitEffect();
	InitResource();
}

/*��������*/
QtGuiClass::~QtGuiClass()
{
	if (this->pixels != NULL) {
		delete[] pixels;
	}
	// �ָ�����Ĭ���趨
	if (m_d3dDevContext) {
		m_d3dDevContext->ClearState();
	}

	//qDebug() << "----------DirectX has xigou------" << endl;
}
/*�ۺ��������������е�����*/
void QtGuiClass::updateFrame_Kinect() {
	/*��֡��ʼʱ��*/
	LARGE_INTEGER beginTime = { 0 };
	QueryPerformanceFrequency(&beginTime);
	//����CPUʱ��Ƶ��
	double pcFreq = (double)beginTime.QuadPart / 1000000.0;
	QueryPerformanceCounter(&beginTime);

	//cap >> src;//�ȼ���cap.read(frame)
	//src = imread("C:\\Users\\Administrator\\Desktop\\temp\\greenback2.jpg");//������Ҫ�ĳɴ�Kinect����ȡͼƬ��src
	//colorFlow = (unsigned char*)malloc(sizeof(unsigned char) * colorwidth * colorheight * 3);
	//depthFlow = (unsigned short *)malloc(sizeof(unsigned short)* colorwidth*colorheight);
	getKinectData(colorFlow, depthFlow);
	//src = cv::Mat(colorheight, colorwidth, CV_16UC1, depthFlow);//���ͼ����ת��mat�ģ�������������
	src = cv::Mat(colorheight, colorwidth, CV_8UC3, colorFlow);//��ɫͼ����src������ǰ����Ⱦ
	//cv::imshow("d", src);

	//¼��Ƶ
	//if (videoFrameIndex++ < 500) {
	//	videoOut << src;
	//}
	//else
	//	videoOut.release();

	//ʵʱչʾrgb
	//imshow("enhanced", src);//��ʾ������˵������BGR
	//return;

	//socket��������
	if (depthFlow[0] != 52685 && colorFlow[0] != 205) {//���˻�ɫ��������ɫ��������ue4��GPU
		sender_depth.Send((char *)depthFlow, 1080 * 1920 * 2);//��Ϊunsigned short��16λ��char��8λ�����������ͨ��������2��
		//sender_rgb.Send((char*)colorFlow, 1080 * 1920 * 3);
	}
	else {//��ζ�Ų�ִ��update��Ҳ�Ͳ�����RGB
		src.release();
		//free(depthFlow);//����src�˲ſ���free
		//free(colorFlow);
		return;
	}
	//cv::imshow("wwe", src);//Ƶ����ÿ����֡���һ��ȫ�ҡ�(205, 205, 205)
	//cv::imshow("de", cv::Mat(colorheight, colorwidth, CV_16UC1, depthFlow));//����Ƶ����Ƶ�ʲ�һ����52685
	//cv::waitKey(0);
	count++;
	//qDebug()<< "#################count:" << count << endl;
	if (src.empty()) {//���ĳ֡Ϊ�����˳�ѭ��
		//qDebug() << "#################src Empty!!!" << endl;

		free(pixels);/*�����һ֡������*/
		pixels = NULL;
		m_pTimer->stop();/*ֹͣ��ʱ��*/
		//qDebug() << "--------last count" << count << endl;
		/*��ʱ����*/
		QueryPerformanceCounter(&dwStop);
		int allTime = (dwStop.QuadPart - dwStart.QuadPart) / pcFreqall;/*������ʱ��*/
		/*����ʱ��*/
		double mattetime = (allTime - this->readAllFrameTime - this->writeAllFrameTime) / 1000000.0;
		qDebug() << "------------------Frame number : " << count << "frame" << endl;
		qDebug() << "------------------Running all Time : " << allTime / 1000000.0 << "s" << endl;
		this->readAllFrameTime = this->readAllFrameTime / 1000000.0;
		qDebug() << "------------------readAllFrameTime : " << this->readAllFrameTime << "s" << endl;
		this->writeAllFrameTime = this->writeAllFrameTime / 1000000.0;
		qDebug() << "------------------writeAllFrameTime : " << this->writeAllFrameTime << "s" << endl;
		qDebug() << "------------------Matting Time : " << mattetime << "s" << endl;
		qDebug() << "------------------matte rate : " << count / mattetime << endl;
		/*����µĿ���ʱ��*/
		this->myMatteTime = this->myMatteTime / 1000000.0;
		qDebug() << "------------------myMatteTime: " << this->myMatteTime << endl;
		qDebug() << "------------------myMatteRate : " << count / this->myMatteTime << endl;
		//writer.release();
		//cap.release();
		this->close();
		//this->~QtGuiClass();/*�������������*/
	}
	else {
		free(pixels);/*�����һ֡������*/
		cvtColor(src, src, COLOR_BGR2RGBA);/*����ת��ΪRGBAģʽ�������������*/
		this->width = src.cols;
		this->height = src.rows;
		pixels = new  uint32_t[width*height * 4];
		memcpy(pixels, src.data, width*height * 4);
		src.release();
		//free(depthFlow);//����src�˲ſ���free
		//free(colorFlow);
		/*��֡����ʱ��*/
		LARGE_INTEGER endTime = { 0 };
		QueryPerformanceCounter(&endTime);
		//��ô�������ó��ľ�����֮���ʱ�����ˣ���λΪ΢��
		int time = (endTime.QuadPart - beginTime.QuadPart) / pcFreq;
		this->readAllFrameTime = this->readAllFrameTime + time;

		update();/*���»���*/
	}
}
/*�ۺ��������������е�����*/
void QtGuiClass::updateFrame() {
	/*��֡��ʼʱ��*/
	LARGE_INTEGER beginTime = { 0 };
	QueryPerformanceFrequency(&beginTime);
	//����CPUʱ��Ƶ��
	double pcFreq = (double)beginTime.QuadPart / 1000000.0;
	QueryPerformanceCounter(&beginTime);

	cap >> src;//�ȼ���cap.read(frame);
	count++;
	//qDebug() << "--------read count" << count << endl;
	if (src.empty()) {//���ĳ֡Ϊ�����˳�ѭ��
		free(pixels);/*�����һ֡������*/
		pixels = NULL;
		m_pTimer->stop();/*ֹͣ��ʱ��*/
		//qDebug() << "--------last count" << count << endl;
		/*��ʱ����*/
		QueryPerformanceCounter(&dwStop);
		int allTime = (dwStop.QuadPart - dwStart.QuadPart) / pcFreqall;/*������ʱ��*/
		/*����ʱ��*/
		double mattetime = (allTime - this->readAllFrameTime - this->writeAllFrameTime) / 1000000.0;
		qDebug() << "------------------Frame number : " << count << "frame" << endl;
		qDebug() << "------------------Running all Time : " << allTime / 1000000.0 << "s" << endl;
		this->readAllFrameTime = this->readAllFrameTime / 1000000.0;
		qDebug() << "------------------readAllFrameTime : " << this->readAllFrameTime << "s" << endl;
		this->writeAllFrameTime = this->writeAllFrameTime / 1000000.0;
		qDebug() << "------------------writeAllFrameTime : " << this->writeAllFrameTime << "s" << endl;
		qDebug() << "------------------Matting Time : " << mattetime << "s" << endl;
		qDebug() << "------------------matte rate : " << count / mattetime << endl;
		/*����µĿ���ʱ��*/
		this->myMatteTime=this->myMatteTime / 1000000.0;
		qDebug() << "------------------myMatteTime: " << this->myMatteTime << endl;
		qDebug() << "------------------myMatteRate : " << count / this->myMatteTime << endl;
		writer.release();
		cap.release();
		this->close();
		//this->~QtGuiClass();/*�������������*/
	}
	else {
		free(pixels);/*�����һ֡������*/
		cvtColor(src, src, COLOR_BGR2RGBA);/*����ת��ΪRGBAģʽ�������������*/
		this->width = src.cols;
		this->height = src.rows;
		pixels = new  uint32_t[width*height * 4];
		memcpy(pixels, src.data, width*height * 4);

		/*��֡����ʱ��*/
		LARGE_INTEGER endTime = { 0 };
		QueryPerformanceCounter(&endTime);
		//��ô�������ó��ľ�����֮���ʱ�����ˣ���λΪ΢��
		int time = (endTime.QuadPart - beginTime.QuadPart) / pcFreq;
		this->readAllFrameTime = this->readAllFrameTime + time;

		update();/*���»���*/
	}
}
void QtGuiClass::resizeEvent(QResizeEvent *event)
{
	//ResizeD3D();
}

/*��ʼ��DirectX11�Ļ������*/
void QtGuiClass::InitD3D()
{
	/*���ú�̨������������*/
	DXGI_MODE_DESC bufferDesc;//Dx�������� ��̨������ �Ľṹ��
	ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));//��������������Dx�����ṹ�壬����ı���ȫ������0
	bufferDesc.Width = width; //QT�Դ��Ļ�ȡ���ڿ�ȵķ���--��̨���������
	bufferDesc.Height = height;//QT�Դ��Ļ�ȡ���ڸ߶ȵķ���--��̨�������߶�
	bufferDesc.RefreshRate.Numerator = 60; //��ע��1��  ��ʾ����ˢ����  ˢ���������  60pfs/ÿ1��
	bufferDesc.RefreshRate.Denominator = 1; //��ע��1��  ��ʾ���ˢ����
	bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;// ��̨���������ظ�ʽ
	bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED; //ScanlineOrdering:��������ɨ���߻���ģʽ��ö�١���������Ϊ0
	bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;//��������������ʽ��ö�����ͣ�Ĭ��ʹ��DXGI_MODE_SCALING_UNSPECIFIED��
	/*���ý�����������*/
	DXGI_SWAP_CHAIN_DESC swapChainDesc;//Dx�������������������Ľṹ��
	ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
	swapChainDesc.BufferDesc = bufferDesc;//���ز�����������������
	swapChainDesc.SampleDesc.Count = 1;//���ز����Ľṹ������
	swapChainDesc.SampleDesc.Quality = 0;//���ز����Ľṹ������
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;//��Ϊ DXGI_USAGE_RENDER_TARGET_OUTPUT����Ϊ����Ҫ��������Ⱦ����̨������
	swapChainDesc.BufferCount = 1;//�������еĺ�̨����������������һ��ֻ��һ����̨��������ʵ��˫ ���档��Ȼ����Ҳ����ʹ��������̨��������ʵ�������档
	swapChainDesc.OutputWindow = (HWND)winId();//����ǳ���Ҫ��winId()��Qt��QWidget��ȡ��������ĺ����������ʾ���ǰѵ�ǰQWidget�Ŀ���Ȩ������DX����DXȥ����������
	swapChainDesc.Windowed = TRUE;//����Ϊ true ʱ�������Դ���ģʽ���У�����Ϊ false ʱ��������ȫ��ģʽ����
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;//��Ϊ DXGI_SWAP_EFFECT_DISCARD�����Կ���������ѡ�����Ч ����ʾģʽ

	//����һ�����µķ���������ͬʱ�����豸���豸�����ĺͽ�����
	/*�޸�һ�������������������Ϣ*/
	hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_DEBUG, NULL, NULL,
		D3D11_SDK_VERSION, &swapChainDesc, m_swapChain.ReleaseAndGetAddressOf(), m_d3dDevice.ReleaseAndGetAddressOf(), NULL, m_d3dDevContext.ReleaseAndGetAddressOf());
	
	//��ȡ��̨��������ָ��
	hr = m_swapChain->GetBuffer(0,__uuidof(ID3D11Texture2D),(void**)backBuffer.ReleaseAndGetAddressOf());
	if (hr != 0) {
		qDebug() << "get buffer failed!" << endl;
	}
	//���������Ĵ�����Ŀ����Ⱦ��ͼ������ƽʱ������������������ʾ���Ǹ�����viewport���Ѻ�̨�������󶨵�Ŀ����Ⱦ��ͼ
	hr = m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.ReleaseAndGetAddressOf());
	//XYB_SafeDel(backBuffer)//��ע��4������ǳ���Ҫ��������Ϊ�˰�ȫ�ͷ�COM�Զ����
	//ÿ����һ��GetBuffer ��������̨�������� COM ���ü����ͻ� ���ϵ���һ�Σ�����������ڴ���Ƭ�εĽ�β���ͷ�����ԭ��
	//������ͷ���ᷢ�ֻ��Ʋ���shader������ȴ����clear��Ļ��ɫŶ

	//������Ǵ������ģ�建����,�������̺�Ŀ����Ⱦ��ͼ����һ����������Բ�һ���Ĳ�������ע��
	D3D11_TEXTURE2D_DESC depthStencilDesc;
	depthStencilDesc.Width = width;/*(uint)width()*/
	depthStencilDesc.Height = height;/*(uint)height()*/
	depthStencilDesc.MipLevels = 1;//mipLevelsҪ���ɼ�����1��ʾ���������0��ʾ������һ���׵�mipMap�����ﲻ��Ҫmip����д1
	depthStencilDesc.ArraySize = 1;//���������е�����������������������ͼ�������棬��߾Ϳ�������Ϊ6������6�ı���
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//�����ʽ
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;//�󶨱�ʶ�������������һ�����ģ�建��ͼƬ����ʵ����ṹ����Ǹ���ͨ��2D���������Ľṹ��
	depthStencilDesc.CPUAccessFlags = 0; //���û�и㶮������Ū���˲���������°ɣ��д���֪���Ļ�ӭ������������лл�����������CPU���ʵ�
	depthStencilDesc.MiscFlags = 0;//����ǿ��������Ǹ�CPU���ʵ�

	//��GPU�ϸ��������Ǹ���������һ��������������ڴ�Buffer
	hr = m_d3dDevice->CreateTexture2D(&depthStencilDesc, NULL, m_depthStencilBuffer.ReleaseAndGetAddressOf());
	//�����buffer�󶨸��ӿ�
	hr = m_d3dDevice->CreateDepthStencilView(m_depthStencilBuffer.Get(), NULL, m_depthStencilView.ReleaseAndGetAddressOf());
	//��Ŀ����Ⱦ�ӿں����ģ���������
	m_d3dDevContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
	//���Ĳ��������ӿڴ�С��D3D11Ĭ�ϲ��������ӿڣ��˲�������ֶ�����
	viewport.TopLeftX = 0;//�ӿ����Ͻǵĺ�����
	viewport.TopLeftY = 0;//�ӿ����Ͻǵ�������
	viewport.Width = width; //��
	viewport.Height = height;//��
	viewport.MinDepth = 0.0f;//��Ȼ�����Сֵ0��dx�����ֵ��0��1��Χ�ڵģ���������ֵ��0
	viewport.MaxDepth = 1.0f;//���ֵ���ֵ1��
	//ˢ��������viewport
	m_d3dDevContext->RSSetViewports(1, &viewport);
}

/*��ʼ����ɫ��*/
void QtGuiClass::InitEffect() {
	HRESULT result;
	/*��ȡ����õ���ɫ����������Ϣ*/
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
	result = D3DReadFileToBlob(L"HLSL\\PixelShader_gamma.cso", m_pixelBlob_gamma.ReleaseAndGetAddressOf());//26���
	result = D3DReadFileToBlob(L"HLSL\\PixelShader_brightANDcontrast.cso", m_pixelBlob_brightANDcontrast.ReleaseAndGetAddressOf());//26���
	result = D3DReadFileToBlob(L"HLSL\\PixelShader_sharp.cso", m_pixelBlob_sharp.ReleaseAndGetAddressOf());//26���
	/****************28�޸Ŀ�ʼ****************/
	result = D3DReadFileToBlob(L"HLSL\\PixelShader_binarization.cso", m_pixelBlob_binarization.ReleaseAndGetAddressOf());
	/****************28�޸Ľ���****************/

	/*����������ɫ��*/
	result = m_d3dDevice->CreateVertexShader(m_vertexBlob->GetBufferPointer(), m_vertexBlob->GetBufferSize(), 
		0, m_vertexShader.ReleaseAndGetAddressOf());
	// �������󶨶��㲼��
	m_d3dDevice->CreateInputLayout(VertexPosColor::inputLayout, ARRAYSIZE(VertexPosColor::inputLayout), m_vertexBlob->GetBufferPointer(),
		m_vertexBlob->GetBufferSize(), m_pVertexLayout.ReleaseAndGetAddressOf());//��Position����Shader

	/*�����ڶ�������ɫ��*/
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
		0, m_pixelShader_gamma.ReleaseAndGetAddressOf());//26���
	result = m_d3dDevice->CreatePixelShader(m_pixelBlob_brightANDcontrast->GetBufferPointer(), m_pixelBlob_brightANDcontrast->GetBufferSize(),
		0, m_pixelShader_brightANDcontrast.ReleaseAndGetAddressOf());//26���
	result = m_d3dDevice->CreatePixelShader(m_pixelBlob_sharp->GetBufferPointer(), m_pixelBlob_sharp->GetBufferSize(),
		0, m_pixelShader_sharp.ReleaseAndGetAddressOf());//26���
	/****************28�޸Ŀ�ʼ****************/
	result = m_d3dDevice->CreatePixelShader(m_pixelBlob_binarization->GetBufferPointer(), m_pixelBlob_binarization->GetBufferSize(),
		0, m_pixelShader_binarization.ReleaseAndGetAddressOf());
	/****************28�޸Ľ���****************/
}
void QtGuiClass::InitResource(){
	/*ע���ĸ�����ĸ���˳��Ӧ����˳ʱ���Ų�*/
	/*�����ı������õĶ���*/
	VertexPosColor vertices[] =
	{
		//4p->Rectangle λ������+��ɫ����+��������
		{ XMFLOAT3(-1.0f,  1.0f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f),XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3( 1.0f,  1.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3( 1.0f, -1.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),XMFLOAT2(1.0f, 1.0f) },
	};
	// ���ö��㻺��������
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof vertices;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	// �½����㻺����
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	/*����һ�����㻺����--���ڶ�����ɫ��*/
	m_d3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffer.ReleaseAndGetAddressOf());

	// ���ó�������������-������������ʹ�ò���Ҫ������Դ��ͼ
	D3D11_BUFFER_DESC cbd;
	ZeroMemory(&cbd, sizeof(cbd));
	cbd.Usage = D3D11_USAGE_DYNAMIC;/*������������ҪƵ������*/
	//cbd.ByteWidth = sizeof(constantBuffer_matte);/*����������������16�ı���*/
	cbd.ByteWidth = 64;/*����������������16�ı���*///26���
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;/*����CPUд��*/

	/*�½�����ɫ�ȿ����ȥ��ɫ������ĳ���������*/
	hr = m_d3dDevice->CreateBuffer(&cbd, nullptr, CBufferMatteDespill.ReleaseAndGetAddressOf());
	/*�½�����һЩ������ɫ���ĳ���������*/
	hr = m_d3dDevice->CreateBuffer(&cbd, nullptr, CBufferOtherParameter.ReleaseAndGetAddressOf());
	/*�½������������ֵ�������ɫ���ĳ���������*/
	hr = m_d3dDevice->CreateBuffer(&cbd, nullptr, GarbageCBuffer.ReleaseAndGetAddressOf());
	/*�½����ں������ֵ�������ɫ���ĳ���������*/
	hr = m_d3dDevice->CreateBuffer(&cbd, nullptr, CoreCBuffer.ReleaseAndGetAddressOf());

	// ���õڶ�����������������������
	D3D11_BUFFER_DESC cbd2;
	ZeroMemory(&cbd2, sizeof(cbd2));
	cbd2.Usage = D3D11_USAGE_DYNAMIC;/*������������ҪƵ������*/
	cbd2.ByteWidth = 384;/*����������������16�ı���*/
	cbd2.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd2.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;/*����CPUд��*/
	/*�½������𻯵ĳ���������*/
	hr = m_d3dDevice->CreateBuffer(&cbd2, nullptr, CBufferFeather.ReleaseAndGetAddressOf());

	// ��ʼ��������������ֵ---��paintEvent����������
	
	// ��ʼ������Ͳ�����״̬---����Ҳ��paint����������
	/*��ʼ��������״̬����*/
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	//sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;/*���Բ���*/
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;/*�����*/
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	/*����������״̬*/
	hr = m_d3dDevice->CreateSamplerState(&sampDesc, m_pSamplerState.ReleaseAndGetAddressOf());
	// ������ɫ�׶����úò�����
	m_d3dDevContext->PSSetSamplers(0, 1, m_pSamplerState.GetAddressOf());/*����HLSL�ж�Ӧ�Ĳ�����״̬�Ϳ���ʹ����*/

	// ����Ⱦ���߸����׶ΰ󶨺�������Դ
	// ����װ��׶εĶ��㻺��������
	UINT stride = sizeof(VertexPosColor);// ��Խ�ֽ���
	UINT offset = 0;// ��ʼƫ����
	//���ö�������--���㻺�����е�����--������OpenGL�е�VAO
	m_d3dDevContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);

	// ����ͼԪ���ͣ��趨���벼��
	m_d3dDevContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);/*�����ı���*/
	m_d3dDevContext->IASetInputLayout(m_pVertexLayout.Get());

	/*����2D��������--���԰󶨵���Ⱦ���ߵģ�CPU���ɶ�д��GPU����*/
	texDESC.Width = width;/*������*/
	texDESC.Height = height;/*����߶�*/
	texDESC.MipLevels = 1;/*�ܹ�����������mipmap������ָ��Ϊ0��������mipmap����Ӧ��ָ��Ϊ1*/
	texDESC.ArraySize = 1;/*ָ���������Ŀ����������ʹ��1*/
	texDESC.Format = DXGI_FORMAT_R8G8B8A8_UNORM;/*ָ������洢�����ݸ�ʽRGBA*/
	texDESC.SampleDesc.Count = 1;	// ��ʹ�ö��ز���
	texDESC.SampleDesc.Quality = 0;
	texDESC.Usage = D3D11_USAGE_DEFAULT;/*ָ�����ݵ�CPU/GPU����Ȩ�ޣ�GPU�ɶ���д*/
	texDESC.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;/*���������Ϊ��ȾĿ�������㣬����ָ����������������mipmaps*/
	texDESC.CPUAccessFlags = 0;
	texDESC.MiscFlags = 0;// ָ����Ҫ����mipmap
	
	/*����2D��������--�����԰󶨵���Ⱦ���ߵģ�CPU�ɶ�д����Ҫ��Ϊ�˽��������ݶ�ȡ���ڴ�*/
	texDESC_cpu.Width = width;/*������*/
	texDESC_cpu.Height = height;/*����߶�*/
	texDESC_cpu.MipLevels = 1;/*�ܹ�����������mipmap������ָ��Ϊ0��������mipmap����Ӧ��ָ��Ϊ1*/
	texDESC_cpu.ArraySize = 1;/*ָ���������Ŀ����������ʹ��1*/
	texDESC_cpu.Format = DXGI_FORMAT_R8G8B8A8_UNORM;/*ָ������洢�����ݸ�ʽRGBA*/
	texDESC_cpu.SampleDesc.Count = 1;	// ��ʹ�ö��ز���
	texDESC_cpu.SampleDesc.Quality = 0;
	texDESC_cpu.Usage = D3D11_USAGE_STAGING;/*ָ�����ݵ�CPU/GPU����Ȩ�ޣ�GPU+CPU�ɶ���д*/
	texDESC_cpu.BindFlags = 0;
	texDESC_cpu.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	texDESC_cpu.MiscFlags &= D3D11_RESOURCE_MISC_SHARED;/*����Ϊ�������������ᱨ��*/
	//texDESC.MiscFlags = 0;// ָ����Ҫ����mipmap

	/*������ɫ����Դ��ͼ������*/
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	/*����������Դ������*/
	sd.pSysMem = NULL;
	//sd.pSysMem = pixels;/*���ڳ�ʼ��������*/
	sd.SysMemPitch = width * sizeof(uint32_t);/*��ǰ����Դһ����ռ���ֽ���(2D/3D����ʹ��)*/
	sd.SysMemSlicePitch = width * height * sizeof(uint32_t);/*��ǰ����Դһ��������Ƭ��ռ���ֽ���(��3D����ʹ��)*/

	// ***********��ʼ�����״̬***********
	/*����������ͬλ�õ����ص㣬�涨CsrcΪԴ���ص���ɫ����������ɫ����������أ���
	CdstΪĿ�����ص���ɫ���Ѿ������ں󱸻������ϵ����أ���*/
	//D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(blendDesc));
	auto& rtDesc = blendDesc.RenderTarget[0];

	// ͸�����ģʽ
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
	rtDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;/*�����˼����������������Ϊ����ɫ*/

	hr = m_d3dDevice->CreateBlendState(&blendDesc, BSAlphaToCoverage.GetAddressOf());
}
/*��������ɰ�*/
void QtGuiClass::setupMatte() {
	//��������ɫ��������������ɫ���󶨵���Ⱦ����
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);/*ʹ�����������ɫ��*/
	m_d3dDevContext->PSSetShader(m_pixelShader_matte.Get(), nullptr, 0);/*ʹ�����Ƭ����ɫ��*/
	// ��ʼ��������������ֵ
	//�����setConstantBuffers�����г�ʼ��ɫ�ȿ�����Ҫ�ĳ�����������ֵ
	//m_d3dDevContext->PSSetConstantBuffers(0, 1, CBufferMatteDespill.GetAddressOf());
	/*��Ⱦ���Զ����ɰ�����*/
	/*����һ���Զ���2D��������Ϊ��ȾĿ��*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_matte.ReleaseAndGetAddressOf());/*��ʼ����ԴΪ��*/
	/*����������ȾĿ��󶨵�����Ϊ��2D����--��һ��RTV*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_matte.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	//����ȾĿ����ͼ�����/ģ�建������ϵ�����
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*������ɫ����Դ��ͼ*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*�����ı��ε���ȾĿ����ͼ�󶨵��Զ�������tex_matte*/
	m_d3dDevContext->Draw(4, 0);
	nowTex = tex_matte;/*��ǰ�м�����Ϊ�ڰ��ɰ�*/
	/*������ǰ�м��������ɫ����Դ��ͼSRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*����DSV-�����ͼ*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
/*�������ǰ��*/
void QtGuiClass::setupForeImage() {
	/*�ڶ��λ���-ʹ�õڶ���������ɫ��-��ǰ��������Զ����D����tex_fore*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_fore.ReleaseAndGetAddressOf());/*��ʼ����ԴΪ��*/
	//��������ɫ���͵�2��������ɫ���󶨵���Ⱦ����
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);/*ʹ�����������ɫ��*/
	m_d3dDevContext->PSSetShader(m_pixelShader_fore.Get(), nullptr, 0);/*ʹ�����Ƭ����ɫ��*/

	/*4.����������ȾĿ��󶨵�����Ϊ��2D����--��һ��RTV*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_fore.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	// ����ȾĿ����ͼ�����/ģ�建������ϵ�����
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	
	/*10. ���õڶ���������ɫ����Դ--�ø���Դ��ͼ�󶨵���Ⱦ���ߵ�ָ���׶�*/
	/*ע�⣺�����м�����Ϊ��ɫ����Դ���������°�RTV֮����Ϊ�м�����ֻ����Ϊ�����������*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());/*������HLSL���Ӧregisgter(t1)��gTex_now��ŵľ����м�����*/
	m_d3dDevContext->PSSetShaderResources(0, 1, texInputSRV.GetAddressOf());
	/*����ȥ����ɫ������õĲ�����Ӧ�ĳ���������*/
	//m_d3dDevContext->PSSetConstantBuffers(0, 1, CBufferMatteDespill.GetAddressOf());
	/*�����ı��ε���ȾĿ����ͼ�󶨵�����--�󱸻�����*/
	m_d3dDevContext->Draw(4, 0);
	nowTex = tex_fore;/*��ǰ�м�����Ϊ�ڰ��ɰ�*/
	/*������ǰ�м��������ɫ����Դ��ͼSRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	
	/*6. ����DSV-�����ͼ*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

/*����Ԥģ��*/
void QtGuiClass::setupPreBlur() {
	/*�ȴ�RGBת��ΪYCC��ɫģʽ���ٶ�X�����ģ�����ٶ�Y�����ģ�����ٽ�YCCת��ΪRGBģʽ*/
	/*��һ��������һ���յ�2D����������ȾĿ��*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_preBlur.ReleaseAndGetAddressOf());/*��ʼ����ԴΪ��*/
	/*�ڶ���������Ҫ�õ��Ķ�����ɫ����������ɫ��*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dDevContext->PSSetShader(m_pixelShader_preBlur.Get(), nullptr, 0);
	/*��������������ȾĿ����ͼ���󶨵���Ⱦ����*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_preBlur.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*���Ĳ������ö�Ӧ������ɫ��������������Դ--��Ӧ��һ���Ĵ���*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*���岽�����ö�Ӧ������ɫ���ĳ�����������Դ*/
//	m_d3dDevContext->PSSetConstantBuffers(2, 1, m_pConstantBuffer_parameter.GetAddressOf());
	/*�����������Ƶ��Զ���2D����*/
	m_d3dDevContext->Draw(4, 0);
	/*���߲�������ǰ����ָ��ָ���Զ�������������Ӧ��ɫ����Դ��ͼ*/
	nowTex = tex_preBlur;/*��ǰ�м�����Ϊ��Y��ģ���������*/
	/*������ǰ�м��������ɫ����Դ��ͼSRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*�ڰ˲����ͷ���ȾĿ����ͼ��������ģ�建��*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
/*����ǯ�ƺ�ɫ��ɫ*/
void QtGuiClass::setupClip() {
	/*��һ��������һ���յ�2D����������ȾĿ��*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_clip.ReleaseAndGetAddressOf());/*��ʼ����ԴΪ��*/
	/*�ڶ���������Ҫ�õ��Ķ�����ɫ����������ɫ��*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dDevContext->PSSetShader(m_pixelShader_clip.Get(), nullptr, 0);
	/*��������������ȾĿ����ͼ���󶨵���Ⱦ����*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_clip.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*���Ĳ������ö�Ӧ������ɫ��������������Դ--��Ӧ��һ���Ĵ���*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*���岽�����ö�Ӧ������ɫ���ĳ�����������Դ*/
	// PS������������ӦHLSL�Ĵ���b2�ĳ���������--Ҳ���ǽ�m_pConstantBuffers_despill������ӳ����˵�4��������ɫ���ĳ���������
	//m_d3dDevContext->PSSetConstantBuffers(2, 1, m_pConstantBuffer_parameter.GetAddressOf());
	/*�����������Ƶ��Զ���2D����*/
	m_d3dDevContext->Draw(4, 0);
	/*���߲�������ǰ����ָ��ָ���Զ�������������Ӧ��ɫ����Դ��ͼ*/
	nowTex = tex_clip;/*��ǰ�м�����Ϊ�ڰ��ɰ�*/
	/*������ǰ�м��������ɫ����Դ��ͼSRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*�ڰ˲����ͷ���ȾĿ����ͼ��������ģ�建��*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
/*������ɫУ��*///26���
void QtGuiClass::setupGamma() {
	/*��һ��������һ���յ�2D����������ȾĿ��*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_gamma.ReleaseAndGetAddressOf());/*��ʼ����ԴΪ��*/
	/*�ڶ���������Ҫ�õ��Ķ�����ɫ����������ɫ��*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dDevContext->PSSetShader(m_pixelShader_gamma.Get(), nullptr, 0);
	/*��������������ȾĿ����ͼ���󶨵���Ⱦ����*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_gamma.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*���Ĳ������ö�Ӧ������ɫ��������������Դ--��Ӧ��һ���Ĵ���*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*���岽�����ö�Ӧ������ɫ���ĳ�����������Դ*/
	// PS������������ӦHLSL�Ĵ���b2�ĳ���������--Ҳ���ǽ�m_pConstantBuffers_despill������ӳ����˵�4��������ɫ���ĳ���������
	//m_d3dDevContext->PSSetConstantBuffers(2, 1, m_pConstantBuffer_parameter.GetAddressOf());
	/*�����������Ƶ��Զ���2D����*/
	m_d3dDevContext->Draw(4, 0);
	/*���߲�������ǰ����ָ��ָ���Զ�������������Ӧ��ɫ����Դ��ͼ*/
	nowTex = tex_gamma;/*��ǰ�м�����Ϊ�ڰ��ɰ�*/
	/*������ǰ�м��������ɫ����Դ��ͼSRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*�ڰ˲����ͷ���ȾĿ����ͼ��������ģ�建��*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
/****************28�޸Ŀ�ʼ****************/
/*���ö�ֵ��*/
void QtGuiClass::setupBinarization() {
	/*��һ��������һ���յ�2D����������ȾĿ��*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_binarization.ReleaseAndGetAddressOf());/*��ʼ����ԴΪ��*/
	/*�ڶ���������Ҫ�õ��Ķ�����ɫ����������ɫ��*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dDevContext->PSSetShader(m_pixelShader_binarization.Get(), nullptr, 0);
	/*��������������ȾĿ����ͼ���󶨵���Ⱦ����*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_binarization.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*���Ĳ������ö�Ӧ������ɫ��������������Դ--��Ӧ��һ���Ĵ���*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*���岽�����ö�Ӧ������ɫ���ĳ�����������Դ*/
	// PS������������ӦHLSL�Ĵ���b2�ĳ���������--Ҳ���ǽ�m_pConstantBuffers_despill������ӳ����˵�4��������ɫ���ĳ���������
	//m_d3dDevContext->PSSetConstantBuffers(2, 1, m_pConstantBuffer_parameter.GetAddressOf());
	/*�����������Ƶ��Զ���2D����*/
	m_d3dDevContext->Draw(4, 0);
	/*���߲�������ǰ����ָ��ָ���Զ�������������Ӧ��ɫ����Դ��ͼ*/
	nowTex = tex_binarization;/*��ǰ�м�����Ϊ�ڰ��ɰ�*/
					   /*������ǰ�м��������ɫ����Դ��ͼSRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*�ڰ˲����ͷ���ȾĿ����ͼ��������ģ�建��*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
/****************28�޸Ľ���****************/

/*�������ȺͶԱȶ���ǿ*///26���
void QtGuiClass::setupbrightANDcontrast() {
	/*��һ��������һ���յ�2D����������ȾĿ��*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_brightANDcontrast.ReleaseAndGetAddressOf());/*��ʼ����ԴΪ��*/
	/*�ڶ���������Ҫ�õ��Ķ�����ɫ����������ɫ��*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dDevContext->PSSetShader(m_pixelShader_brightANDcontrast.Get(), nullptr, 0);
	/*��������������ȾĿ����ͼ���󶨵���Ⱦ����*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_brightANDcontrast.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*���Ĳ������ö�Ӧ������ɫ��������������Դ--��Ӧ��һ���Ĵ���*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*���岽�����ö�Ӧ������ɫ���ĳ�����������Դ*/
	// PS������������ӦHLSL�Ĵ���b2�ĳ���������--Ҳ���ǽ�m_pConstantBuffers_despill������ӳ����˵�4��������ɫ���ĳ���������
	//m_d3dDevContext->PSSetConstantBuffers(2, 1, m_pConstantBuffer_parameter.GetAddressOf());
	/*�����������Ƶ��Զ���2D����*/
	m_d3dDevContext->Draw(4, 0);
	/*���߲�������ǰ����ָ��ָ���Զ�������������Ӧ��ɫ����Դ��ͼ*/
	nowTex = tex_brightANDcontrast;/*��ǰ�м�����Ϊ�ڰ��ɰ�*/
	/*������ǰ�м��������ɫ����Դ��ͼSRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*�ڰ˲����ͷ���ȾĿ����ͼ��������ģ�建��*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
/*������*///26���
void QtGuiClass::setupSharp() {
	/*��һ��������һ���յ�2D����������ȾĿ��*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_sharp.ReleaseAndGetAddressOf());/*��ʼ����ԴΪ��*/
	/*�ڶ���������Ҫ�õ��Ķ�����ɫ����������ɫ��*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dDevContext->PSSetShader(m_pixelShader_sharp.Get(), nullptr, 0);
	/*��������������ȾĿ����ͼ���󶨵���Ⱦ����*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_sharp.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*���Ĳ������ö�Ӧ������ɫ��������������Դ--��Ӧ��һ���Ĵ���*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*���岽�����ö�Ӧ������ɫ���ĳ�����������Դ*/
	// PS������������ӦHLSL�Ĵ���b2�ĳ���������--Ҳ���ǽ�m_pConstantBuffers_despill������ӳ����˵�4��������ɫ���ĳ���������
	//m_d3dDevContext->PSSetConstantBuffers(2, 1, m_pConstantBuffer_parameter.GetAddressOf());
	/*�����������Ƶ��Զ���2D����*/
	m_d3dDevContext->Draw(4, 0);
	/*���߲�������ǰ����ָ��ָ���Զ�������������Ӧ��ɫ����Դ��ͼ*/
	nowTex = tex_sharp;/*��ǰ�м�����Ϊ�ڰ��ɰ�*/
	/*������ǰ�м��������ɫ����Դ��ͼSRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*�ڰ˲����ͷ���ȾĿ����ͼ��������ģ�建��*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

/*������������*/
void QtGuiClass::setupGarbageMatte() {
	/*��һ��������һ���յ�2D����������ȾĿ��*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_Garbage.ReleaseAndGetAddressOf());/*��ʼ����ԴΪ��*/
	/*�ڶ���������Ҫ�õ��Ķ�����ɫ����������ɫ��*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dDevContext->PSSetShader(m_pixelShader_GarbageMatte.Get(), nullptr, 0);
	/*��������������ȾĿ����ͼ���󶨵���Ⱦ����*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_Garbage.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*���Ĳ������ö�Ӧ������ɫ��������������Դ--��Ӧ��һ���Ĵ���*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*���岽�����ö�Ӧ������ɫ���ĳ�����������Դ*/
	/*��ǰ���ú���*/
	/*�����������Ƶ��Զ���2D����*/
	m_d3dDevContext->Draw(4, 0);
	/*���߲�������ǰ����ָ��ָ���Զ�������������Ӧ��ɫ����Դ��ͼ*/
	nowTex = tex_Garbage;/*��ǰ�м�����Ϊ�ڰ��ɰ�*/
	/*������ǰ�м��������ɫ����Դ��ͼSRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*�ڰ˲����ͷ���ȾĿ����ͼ��������ģ�建��*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
/*���ú�������*/
void QtGuiClass::setupCoreMatte() {
	/*��һ��������һ���յ�2D����������ȾĿ��*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_Core.ReleaseAndGetAddressOf());/*��ʼ����ԴΪ��*/
	/*�ڶ���������Ҫ�õ��Ķ�����ɫ����������ɫ��*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dDevContext->PSSetShader(m_pixelShader_CoreMatte.Get(), nullptr, 0);
	/*��������������ȾĿ����ͼ���󶨵���Ⱦ����*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_Core.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*���Ĳ������ö�Ӧ������ɫ��������������Դ--��Ӧ��һ���Ĵ���*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*���岽�����ö�Ӧ������ɫ���ĳ�����������Դ*/
	/*��ǰ���ú�*/
	/*�����������Ƶ��Զ���2D����*/
	m_d3dDevContext->Draw(4, 0);
	/*���߲�������ǰ����ָ��ָ���Զ�������������Ӧ��ɫ����Դ��ͼ*/
	nowTex = tex_Core;/*��ǰ�м�����Ϊ�ڰ��ɰ�*/
	/*������ǰ�м��������ɫ����Դ��ͼSRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*�ڰ˲����ͷ���ȾĿ����ͼ��������ģ�建��*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

/*���ú���ģ��*/
void QtGuiClass::setupPostBlur() {
	/*��һ��������һ���յ�2D����������ȾĿ��*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_postBlur.ReleaseAndGetAddressOf());/*��ʼ����ԴΪ��*/
	/*�ڶ���������Ҫ�õ��Ķ�����ɫ����������ɫ��*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dDevContext->PSSetShader(m_pixelShader_postBlur.Get(), nullptr, 0);
	/*��������������ȾĿ����ͼ���󶨵���Ⱦ����*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_postBlur.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*���Ĳ������ö�Ӧ������ɫ��������������Դ--��Ӧ��һ���Ĵ���*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*���岽�����ö�Ӧ������ɫ���ĳ�����������Դ*/
	// PS������������ӦHLSL�Ĵ���b2�ĳ���������--Ҳ���ǽ�m_pConstantBuffers_despill������ӳ����˵�4��������ɫ���ĳ���������
//	m_d3dDevContext->PSSetConstantBuffers(2, 1, m_pConstantBuffer_parameter.GetAddressOf());
	/*�����������Ƶ��Զ���2D����*/
	m_d3dDevContext->Draw(4, 0);
	/*���߲�������ǰ����ָ��ָ���Զ�������������Ӧ��ɫ����Դ��ͼ*/
	nowTex = tex_postBlur;/*��ǰ�м�����Ϊ�ڰ��ɰ�*/
	/*������ǰ�м��������ɫ����Դ��ͼSRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*�ڰ˲����ͷ���ȾĿ����ͼ��������ģ�建��*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
/*�������͸�ʴ*/
void QtGuiClass::setupDilateErode() {
	/*��һ��������һ���յ�2D����������ȾĿ��*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_dilateErode.ReleaseAndGetAddressOf());/*��ʼ����ԴΪ��*/
	/*�ڶ���������Ҫ�õ��Ķ�����ɫ����������ɫ��*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dDevContext->PSSetShader(m_pixelShader_dilateErode.Get(), nullptr, 0);
	/*��������������ȾĿ����ͼ���󶨵���Ⱦ����*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_dilateErode.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*���Ĳ������ö�Ӧ������ɫ��������������Դ--��Ӧ��һ���Ĵ���*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*���岽�����ö�Ӧ������ɫ���ĳ�����������Դ*/
	// PS������������ӦHLSL�Ĵ���b2�ĳ���������--Ҳ���ǽ�m_pConstantBuffers_despill������ӳ����˵�4��������ɫ���ĳ���������
//	m_d3dDevContext->PSSetConstantBuffers(2, 1, m_pConstantBuffer_parameter.GetAddressOf());
	/*�����������Ƶ��Զ���2D����*/
	m_d3dDevContext->Draw(4, 0);
	/*���߲�������ǰ����ָ��ָ���Զ�������������Ӧ��ɫ����Դ��ͼ*/
	nowTex = tex_dilateErode;/*��ǰ�м�����Ϊ�ڰ��ɰ�*/
							 /*������ǰ�м��������ɫ����Դ��ͼSRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*�ڰ˲����ͷ���ȾĿ����ͼ��������ģ�建��*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
/*����X��Y�����*/
void QtGuiClass::setupFeatherXorY(int flag) {
	/*flag=1���ʾ��X����д���flag=2���ʾ��Y����д���*/
	/*��һ��������һ���յ�2D����������ȾĿ��*/
	if (flag == 1) {/*X��*/
		hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_featherX.ReleaseAndGetAddressOf());/*��ʼ����ԴΪ��*/
	}
	else {
		hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_featherY.ReleaseAndGetAddressOf());/*��ʼ����ԴΪ��*/
	}
	/*�ڶ���������Ҫ�õ��Ķ�����ɫ����������ɫ��*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	ComPtr<ID3D11RenderTargetView> nowRTV;
	if (flag == 1) {/*X��*/
		m_d3dDevContext->PSSetShader(m_pixelShader_featherX.Get(), nullptr, 0);
		/*��������������ȾĿ����ͼ���󶨵���Ⱦ����*/
		hr = m_d3dDevice->CreateRenderTargetView(tex_featherX.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	}
	else {
		m_d3dDevContext->PSSetShader(m_pixelShader_featherY.Get(), nullptr, 0);
		/*��������������ȾĿ����ͼ���󶨵���Ⱦ����*/
		hr = m_d3dDevice->CreateRenderTargetView(tex_featherY.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	}
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*���Ĳ������ö�Ӧ������ɫ��������������Դ--��Ӧ��һ���Ĵ���*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	/*���岽�����ö�Ӧ������ɫ���ĳ�����������Դ*/
	// PS������������ӦHLSL�Ĵ���b2�ĳ���������--Ҳ���ǽ�m_pConstantBuffers_despill������ӳ����˵�4��������ɫ���ĳ���������
//	m_d3dDevContext->PSSetConstantBuffers(2, 1, m_pConstantBuffer_parameter.GetAddressOf());
	/*�����������Ƶ��Զ���2D����*/
	m_d3dDevContext->Draw(4, 0);
	/*���߲�������ǰ����ָ��ָ���Զ�������������Ӧ��ɫ����Դ��ͼ*/
	if (flag == 1) {/*X��*/
		nowTex = tex_featherX;
	}
	else {
		nowTex = tex_featherY;
	}
	/*������ǰ�м��������ɫ����Դ��ͼSRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*�ڰ˲����ͷ���ȾĿ����ͼ��������ģ�建��*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
/*������*/
void QtGuiClass::setupFeather() {
	/*��X��*/
	this->setupFeatherXorY(1);
	/*��Y��*/
	this->setupFeatherXorY(2);
}
/*���ñ�����ɫ����Դ��ͼ*/
void QtGuiClass::setComposite(bool composite, string filename_back) {
	this->composite = composite;
	this->filename_bg = filename_back;
	if (this->composite == true) {
		/*�½�һ����������������ɫ����Դ��ͼ*/
		/*��һ�ַ�ʽ��ȡ����ͼ��*/
		//wchar_t *str = multiByteToWideChar(this->filename_bg);
		//hr = CreateWICTextureFromFile(m_d3dDevice.Get(), str, nullptr, texBackSRV.ReleaseAndGetAddressOf());/*L"F:\\yellow.jpg"*/
		//delete[]str;
		/*�ڶ��ַ�ʽ��ȡ����ͼ��*/
		Mat src_back = imread(this->filename_bg);
		cvtColor(src_back, src_back, COLOR_BGR2RGBA);/*����ת��ΪRGBAģʽ�������������*/
		int width_back = src_back.cols;
		int height_back = src_back.rows;
		uint32_t * pixels_back = new uint32_t[width_back*height_back * 4];
		memcpy(pixels_back, src_back.data, width_back*height_back * 4);
		src_back.release();
		sd.pSysMem = pixels_back;/*���ڳ�ʼ��������*/
		/*�����������������С��������Ϊ����ͼƬ�Ĵ�С*/
		texDESC.Width = width_back;/*����������*/
		texDESC.Height = height_back;/*��������߶�*/
		/*�޸ı���ͼ��Ӧ��sd*/
		sd.SysMemPitch = width_back * sizeof(uint32_t);/*��ǰ����Դһ����ռ���ֽ���(2D/3D����ʹ��)*/
		sd.SysMemSlicePitch = width_back * height_back * sizeof(uint32_t);/*��ǰ����Դһ��������Ƭ��ռ���ֽ���(��3D����ʹ��)*/
		hr = m_d3dDevice->CreateTexture2D(&texDESC, &sd, tex_back.ReleaseAndGetAddressOf());
		hr = m_d3dDevice->CreateShaderResourceView(tex_back.Get(), &srvDesc, texBackSRV.ReleaseAndGetAddressOf());
		free(pixels_back);
		/*�����޸�2D���������Ĵ�С*/
		texDESC.Width = width;/*����������*/
		texDESC.Height = height;/*��������߶�*/
		sd.SysMemPitch = width * sizeof(uint32_t);
		sd.SysMemSlicePitch = width * height * sizeof(uint32_t);
	}
}
/*�ϳ�ǰ������*/
void QtGuiClass::setupComposite() {
	/*�½�һ����������������ɫ����Դ��ͼ*/
	/*��һ��������һ���յ�2D����������ȾĿ��*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, NULL, tex_Composite.ReleaseAndGetAddressOf());/*��ʼ����ԴΪ��*/
	/*�ڶ���������Ҫ�õ��Ķ�����ɫ����������ɫ��*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dDevContext->PSSetShader(m_pixelShader_Composite.Get(), nullptr, 0);
	/*��������������ȾĿ����ͼ���󶨵���Ⱦ����*/
	ComPtr<ID3D11RenderTargetView> nowRTV;
	hr = m_d3dDevice->CreateRenderTargetView(tex_Composite.Get(), NULL, nowRTV.ReleaseAndGetAddressOf());
	m_d3dDevContext->OMSetRenderTargets(1, nowRTV.GetAddressOf(), m_depthStencilView.Get());
	/*���Ĳ������ö�Ӧ������ɫ��������������Դ--��Ӧ��һ���Ĵ���*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());
	m_d3dDevContext->PSSetShaderResources(2, 1, texBackSRV.GetAddressOf());
	/*���岽�����ö�Ӧ������ɫ���ĳ�����������Դ*/
	// PS������������ӦHLSL�Ĵ���b2�ĳ���������--Ҳ���ǽ�m_pConstantBuffers_despill������ӳ����˵�4��������ɫ���ĳ���������
	//m_d3dDevContext->PSSetConstantBuffers(2, 1, m_pConstantBuffer_parameter.GetAddressOf());
	/*�����������Ƶ��Զ���2D����*/
	m_d3dDevContext->Draw(4, 0);
	/*���߲�������ǰ����ָ��ָ���Զ�������������Ӧ��ɫ����Դ��ͼ*/
	nowTex = tex_Composite;/*��ǰ�м�����Ϊ�ڰ��ɰ�*/
	/*������ǰ�м��������ɫ����Դ��ͼSRV*/
	hr = m_d3dDevice->CreateShaderResourceView(nowTex.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	/*�ڰ˲����ͷ���ȾĿ����ͼ��������ģ�建��*/
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
/*���ó�����������ֵ*/
void QtGuiClass::setConstantBuffers() {
	/*���ó�����������Դ---ǯ�ƺ�ɫ��ɫ������Ĳ���*/
	m_parameter.size = this->preBlurValue;/*1.0*/
	m_parameter.clipBlack = this->clipBlack;
	m_parameter.clipWhite = this->clipWhite;
	m_parameter.kernelRadius = this->kernelRadius;
	m_parameter.kernelTolerance = this->kernelTolerance;
	m_parameter.size_postBlur = this->blur_post;
	m_parameter.gamma = this->gamma;//26���
	m_parameter.brightness = this->brightness;//26���
	m_parameter.contrast = this->contrast;//26���
	m_parameter.sharp_value = this->sharp_value;//26���
	/****************28�޸Ŀ�ʼ****************/
	m_parameter.binarization_threshold = this->binarization_threshold;
	/****************28�޸Ľ���****************/

	if (this->distance > 0) {
		m_parameter.distance = this->distance;
		m_parameter.isDilateErode = 1;/*��ʾ����*/
	}
	else {
		m_parameter.distance = -this->distance;
		m_parameter.isDilateErode = 2;/*��ʾ��ʴ*/
	}
	if (this->feather_distance > 0) {
		m_parameter.do_subtract = 0;/*�𻯾������0��0*/
		m_parameter.sizeFeather = this->feather_distance;
	}
	else {
		m_parameter.do_subtract = 1;/*�𻯾���С��0��1*/
		m_parameter.sizeFeather = -this->feather_distance;
	}
	m_parameter.falloff = this->feather_falloff;

	// ���³�����������Դ--��CPU����ӳ�䵽GPU
	D3D11_MAPPED_SUBRESOURCE mappedData2;
	hr = m_d3dDevContext->Map(CBufferOtherParameter.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData2);
	memcpy_s(mappedData2.pData, sizeof(constantBuffer_parameter), &m_parameter, sizeof(constantBuffer_parameter));
	m_d3dDevContext->Unmap(CBufferOtherParameter.Get(), 0);

	//�����ȥ��ɫ���������Ҫ�Ĳ���
	matteDespillCB.screen_balance = this->ScreenBalanceValue;
	matteDespillCB.screen_color = DirectX::XMFLOAT4(this->ScreenColorValue[0], this->ScreenColorValue[1], this->ScreenColorValue[2], this->ScreenColorValue[3]);
		//���ñ���####################################################################���޸�
	//matteDespillCB.screen_color = DirectX::XMFLOAT4(98.0f/255, 175.0f/255, 136.0f/255, 1.0f);


	matteDespillCB.primary_channel = max_axis_v3(this->ScreenColorValue);
	matteDespillCB.despill_factor = this->DespillFactor;/*1.0*/
	matteDespillCB.despill_balance = this->DespillBalance;/*0.5*/
	// ����PS������������Դ--��CPU����ӳ�䵽GPU
	D3D11_MAPPED_SUBRESOURCE mappedData;
	hr = m_d3dDevContext->Map(CBufferMatteDespill.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);
	memcpy_s(mappedData.pData, sizeof(MatteDespillCB), &matteDespillCB, sizeof(MatteDespillCB));
	m_d3dDevContext->Unmap(CBufferMatteDespill.Get(), 0);
	/*������Ⱦ�����г�����������Ӧ�Ĵ���b0��ֵ--����ɫ�ȿ����ȥ��ɫ�����*/
	m_d3dDevContext->PSSetConstantBuffers(0, 1, CBufferMatteDespill.GetAddressOf());
	/*������Ⱦ�����г�����������Ӧ�Ĵ���b2��ֵ--���ڳ����������Լ��������ֺͺ����������⹦�ܵĲ���*/
	m_d3dDevContext->PSSetConstantBuffers(2, 1, CBufferOtherParameter.GetAddressOf());

	/*����������������ĳ�����������Դ*/
	if (this->isGarbageMatte) {
		parameterGarbage.xBox = this->x_Garbage;
		parameterGarbage.yBox = this->y_Garbage;
		parameterGarbage.rotationBox = this->rotation_Garbage;
		parameterGarbage.heightBox = this->height_Garbage;
		parameterGarbage.widthBox = this->width_Garbage;
		parameterGarbage.maskType = this->maskType_Garbage;/*��ӻ���ʲô*/
		parameterGarbage.transparentBox = this->transparent_Garbage;
		if (this->isBoxMask_Garbage) {
			parameterGarbage.isBox = 1;/*1�����Σ�2������Բ��*/
		}
		else {
			parameterGarbage.isBox = 2;/*1�����Σ�2������Բ��*/
		}
		//parameterGarbage.isGarbage = 1;/*1����������2�������*/
								   // ���³�����������Դ--��CPU����ӳ�䵽GPU
		D3D11_MAPPED_SUBRESOURCE mappedDataGarbage;
		hr = m_d3dDevContext->Map(GarbageCBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedDataGarbage);
		memcpy_s(mappedDataGarbage.pData, sizeof(cBufferGarbageCore), &parameterGarbage, sizeof(cBufferGarbageCore));
		m_d3dDevContext->Unmap(GarbageCBuffer.Get(), 0);

		/*������Ⱦ�����г�����������Ӧ�Ĵ���b3��ֵ--�����������ֵĲ���*/
		m_d3dDevContext->PSSetConstantBuffers(3, 1, GarbageCBuffer.GetAddressOf());
	}
	/*���ú�����������ĳ�����������Դ*/
	if (this->isCoreMatte) {
		parameterCore.xBox = this->x_Core;
		parameterCore.yBox = this->y_Core;
		parameterCore.rotationBox = this->rotation_Core;
		parameterCore.heightBox = this->height_Core;
		parameterCore.widthBox = this->width_Core;
		parameterCore.maskType = this->maskType_Core;/*��ӻ���ʲô*/
		parameterCore.transparentBox = this->transparent_Core;
		if (this->isBoxMask_Core) {
			parameterCore.isBox = 1;/*1�����Σ�2������Բ��*/
		}
		else {
			parameterCore.isBox = 2;/*1�����Σ�2������Բ��*/
		}
		//parameterGarbage.isGarbage = 2;/*1����������2�������*/
		// ���³�����������Դ--��CPU����ӳ�䵽GPU
		D3D11_MAPPED_SUBRESOURCE mappedDataCore;
		hr = m_d3dDevContext->Map(CoreCBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedDataCore);
		memcpy_s(mappedDataCore.pData, sizeof(cBufferGarbageCore), &parameterCore, sizeof(cBufferGarbageCore));
		m_d3dDevContext->Unmap(CoreCBuffer.Get(), 0);

		/*������Ⱦ�����г�����������Ӧ�Ĵ���b1��ֵ--���ں������ֵĲ���*/
		m_d3dDevContext->PSSetConstantBuffers(1, 1, CoreCBuffer.GetAddressOf());
	}

	/*�����𻯵Ĳ�������*/
	if (this->feather_distance != 0) {
		int rad = max(m_parameter.sizeFeather, 0);
		int filtersize = min(rad, 30000);
		/*��ȡ��̫�ֲ��ĸ�˹����*/
		make_gausstab(rad, filtersize);
		/*������˥�����͵Ĳ�ͬ����Ȩ��*/
		make_dist_fac_inverse(rad, filtersize, m_parameter.falloff);
		/*Ŀǰ�Ѿ���featherInput�ṹ����������鸳��ֵ��*/
		/*��������ν��ṹ�崫�ݵ�HLSL��*/
		// ���³�����������Դ--��CPU����ӳ�䵽GPU
		D3D11_MAPPED_SUBRESOURCE mappedDataFeather;
		hr = m_d3dDevContext->Map(CBufferFeather.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedDataFeather);
		memcpy_s(mappedDataFeather.pData, sizeof(Feather), &featherParameter, sizeof(Feather));
		m_d3dDevContext->Unmap(CBufferFeather.Get(), 0);
		/*������Ⱦ�����г�����������Ӧ�Ĵ���b4��ֵ--����������Ĳ���*/
		m_d3dDevContext->PSSetConstantBuffers(4, 1, CBufferFeather.GetAddressOf());
	}
}
/*��QT��ʼ����*/
void QtGuiClass::paintEvent(QPaintEvent *event)
{
	//qDebug() << "----paint start" << endl;
	/*����ʼʱ��*/
	LARGE_INTEGER beginTime_matte = { 0 };
	QueryPerformanceFrequency(&beginTime_matte);
	//����CPUʱ��Ƶ��
	double pcFreq_matte = (double)beginTime_matte.QuadPart / 1000000.0;
	QueryPerformanceCounter(&beginTime_matte);

	/*��ÿһ֡������ƵĲ����У�������Ҫ����һ����ȾĿ����ͼ�󶨵Ļ�����*/
	m_d3dDevContext->ClearRenderTargetView(m_renderTargetView.Get(), bgcolor);
	m_d3dDevContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	/*��ʼ��һ��2D������Ϊ����*/
	sd.pSysMem = pixels;/*���ڳ�ʼ�����������*/
	/*���Ĳ�������2D������ʼ��*/
	hr = m_d3dDevice->CreateTexture2D(&texDESC, &sd, tex_input.ReleaseAndGetAddressOf());/*��ʼ��һ��������Ϊ����*/
	/*���岽��������ɫ����Դ��ͼ*/
	hr = m_d3dDevice->CreateShaderResourceView(tex_input.Get(), &srvDesc, nowTexSRV.ReleaseAndGetAddressOf());
	hr = m_d3dDevice->CreateShaderResourceView(tex_input.Get(), &srvDesc, texInputSRV.ReleaseAndGetAddressOf());

	/*�������ͼ�����gammaУ��*///26���
	if (this->gamma != 0) {
		this->setupGamma();
	}

	/*�������ͼ������񻯴���*///26���
	if (this->sharp_value > 0) {
		this->setupSharp();
	}

	/*����ͼ���Ԥģ������*/
	if (this->preBlurValue > 0) {
		this->setupPreBlur();/*ִ��Ԥģ����shader*/
	}
	/*�����ɰ�*/
	this->setupMatte();/*�����ɰ浽�Զ���2D����tex_matte*/

	/*���ɰ����ȺͶԱȶ���ǿ*///26���
	if (this->brightness > 0 || this->contrast > 0) {
		this->setupbrightANDcontrast();
	}

	/*����ǯ�ƺ�ɫ��ɫ*/
	if (this->clipBlack > 0.0f || this->clipWhite < 1.0f) {
		this->setupClip();
	}
	/* ������������*/
	if (this->isGarbageMatte) {
		this->setupGarbageMatte();
	}
	/* ���ú�������*/
	if (this->isCoreMatte) {
		this->setupCoreMatte();
	}
	/*���ú���ģ��*/
	if (this->blur_post) {
		this->setupPostBlur();
	}
	/*�������͸�ʴ*/
	if (this->distance != 0) {
		this->setupDilateErode();
	}
	/* ������ */
	if (this->feather_distance != 0) {
		this->setupFeather();
	}
	/****************28�޸Ŀ�ʼ****************/
	/*�����ǰ��֮ǰ���ɰ���ж�ֵ������*/
	if (this->isBinary == true) {
		this->setupBinarization();
	}
	/****************28�޸Ľ���****************/

	/*����ǰ��*/
	if (this->flag == 1) {
		/*�����ǰ����ȥ����ɫ����ϳ���һ����ɫ����*/
		this->setupForeImage();
		
		/*�ϳ�ǰ������*/
		if (this->composite == true && this->filename_bg != "") {
			this->setupComposite();
		}
	}
	/*������������ԭ��������󱸻�����*/
	m_d3dDevContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);/*ʹ�����������ɫ��*/
	m_d3dDevContext->PSSetShader(m_pixelShader_output.Get(), nullptr, 0);/*ʹ�����Ƭ����ɫ��*/

	/*����������ȾĿ��󶨵�����Ϊ�󱸻���������*/
	hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBuffer.ReleaseAndGetAddressOf());
	hr = m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.ReleaseAndGetAddressOf());
	//����ȾĿ����ͼ�����/ģ�建������ϵ�����
	m_d3dDevContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
	/*������Ⱦ״̬--�������ģʽ*/
	m_d3dDevContext->OMSetBlendState(BSAlphaToCoverage.Get(), nullptr, 0xFFFFFFFF);

	/*���õ�3��������ɫ����Դ--�ø���ɫ����Դ��ͼ�󶨵���Ⱦ���ߵ�ָ���׶�*/
	/*ע�⣺�����м�����Ϊ��ɫ����Դ���������°�RTV֮����Ϊ�м�����ֻ����Ϊ�����������*/
	m_d3dDevContext->PSSetShaderResources(1, 1, nowTexSRV.GetAddressOf());/*������HLSL���Ӧregisgter(t1)��gTex_now��ŵľ����м�����*/
	/*�����ı��ε��󱸻�����*/
	m_d3dDevContext->Draw(4, 0);

	/*�������ʱ��*/
	LARGE_INTEGER endTime_matte = { 0 };
	QueryPerformanceCounter(&endTime_matte);
	//��ô�������ó��ľ�����֮���ʱ�����ˣ���λΪ΢��
	int time_matte = (endTime_matte.QuadPart - beginTime_matte.QuadPart) / pcFreq_matte;
	this->myMatteTime = this->myMatteTime + time_matte;

	if (this->ifKinect == false) {
		/*����ǶԵ���ͼƬ���в���������Խ�������浽����·��*/
		if (this->video == false) {
			/*�������-����󱸻����������ļ�*/
			m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer_save.ReleaseAndGetAddressOf()));
			if (this->flag == 1) {/*ǰ��*/
				hr = SaveWICTextureToFile(m_d3dDevContext.Get(), backBuffer_save.Get(), GUID_ContainerFormatPng, L"Screenshot\\output_fore.png");
			}
			else {/*�ɰ�*/
				hr = SaveWICTextureToFile(m_d3dDevContext.Get(), backBuffer_save.Get(), GUID_ContainerFormatPng, L"Screenshot\\output_matte.png");

				//ComPtr<ID3D11Texture2D> pStaging;
				//hr = m_d3dDevice->CreateTexture2D(&texDESC_cpu, nullptr, pStaging.ReleaseAndGetAddressOf());
				//ComPtr<ID3D11Texture2D> backBuffer_save;
				//m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer_save.ReleaseAndGetAddressOf()));
				//m_d3dDevContext->CopyResource(pStaging.Get(), backBuffer_save.Get());;/*������Դ��pStaging*/

				///*�ڶ�������pStaging��ȡ��CPU��һ��ָ��imgData�д洢���ز�����QT*/
				//D3D11_MAPPED_SUBRESOURCE mapped_pixels;
				//hr = m_d3dDevContext->Map(pStaging.Get(), 0, D3D11_MAP_READ, 0, &mapped_pixels);
				//this->imgData = new uchar[width*height * 4];/*���ٿռ�*/
				///*RGBA���������ģ�ÿһ�е����ݶ�����䣬�����Ҫ�����·�ʽȥ��ȡ����*/
				//unsigned char* pData = reinterpret_cast<unsigned char*>(mapped_pixels.pData);
				//for (UINT i = 0; i < height; ++i)
				//{
				//	memcpy_s(&this->imgData[i * width * 4], width * 4, pData, width * 4);/*������*4*/
				//	pData += mapped_pixels.RowPitch;
				//}
				//m_d3dDevContext->Unmap(pStaging.Get(), 0);
				///*���������γ�һ��matͼ��*/
				//Mat frame(height, width, CV_8UC4, this->imgData);
				//cvtColor(frame, frame, COLOR_RGBA2BGRA);
				//this->count2++;
				//qDebug() << "--------write count2" << count2 << endl;
				//frame.release();/*�ͷŵ������frame�ڴ�*/
				//free(this->imgData);

			}
		}
		/*����Ƕ���Ƶ���в������򽫺󱸻������������ȡ��ָ���в�д����Ƶ*/
		if (this->video == true) {
			/*д֡��ʼʱ��*/
			LARGE_INTEGER beginTime = { 0 };
			QueryPerformanceFrequency(&beginTime);
			//����CPUʱ��Ƶ��
			double pcFreq = (double)beginTime.QuadPart / 1000000.0;
			QueryPerformanceCounter(&beginTime);

			/*��һ�������ƺ󱸻���������һ��CPU�ɶ�д������pStaging*/
			ComPtr<ID3D11Texture2D> pStaging;
			hr = m_d3dDevice->CreateTexture2D(&texDESC_cpu, nullptr, pStaging.ReleaseAndGetAddressOf());
			ComPtr<ID3D11Texture2D> backBuffer_save;
			m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer_save.ReleaseAndGetAddressOf()));
			m_d3dDevContext->CopyResource(pStaging.Get(), backBuffer_save.Get());;/*������Դ��pStaging*/

			/*�ڶ�������pStaging��ȡ��CPU��һ��ָ��imgData�д洢���ز�����QT*/
			D3D11_MAPPED_SUBRESOURCE mapped_pixels;
			hr = m_d3dDevContext->Map(pStaging.Get(), 0, D3D11_MAP_READ, 0, &mapped_pixels);
			this->imgData = new uchar[width*height * 4];/*���ٿռ�*/
			/*RGBA���������ģ�ÿһ�е����ݶ�����䣬�����Ҫ�����·�ʽȥ��ȡ����*/
			unsigned char* pData = reinterpret_cast<unsigned char*>(mapped_pixels.pData);
			for (UINT i = 0; i < height; ++i)
			{
				memcpy_s(&this->imgData[i * width * 4], width * 4, pData, width * 4);/*������*4*/
				pData += mapped_pixels.RowPitch;
			}
			m_d3dDevContext->Unmap(pStaging.Get(), 0);

			/*���������γ�һ��matͼ��*/
			Mat frame(height, width, CV_8UC4, this->imgData);
			cvtColor(frame, frame, COLOR_RGBA2BGRA);
			this->count2++;
			writer << frame;//��ͬ��writer.write(frame);
			//qDebug() << "--------write count2" << count2 << endl;
			frame.release();/*�ͷŵ������frame�ڴ�*/
			free(this->imgData);

			/*д֡����ʱ��*/
			LARGE_INTEGER endTime = { 0 };
			QueryPerformanceCounter(&endTime);
			//��ô�������ó��ľ�����֮���ʱ�����ˣ���λΪ΢��
			int time = (endTime.QuadPart - beginTime.QuadPart) / pcFreq;
			this->writeAllFrameTime = this->writeAllFrameTime + time;
		}
	}
	//���ʹ��Kinect
	if (ifKinect == true) {
		/*��һ�������ƺ󱸻���������һ��CPU�ɶ�д������pStaging*/
		ComPtr<ID3D11Texture2D> pStaging;
		hr = m_d3dDevice->CreateTexture2D(&texDESC_cpu, nullptr, pStaging.ReleaseAndGetAddressOf());
		ComPtr<ID3D11Texture2D> backBuffer_save;
		m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer_save.ReleaseAndGetAddressOf()));
		m_d3dDevContext->CopyResource(pStaging.Get(), backBuffer_save.Get());/*������Դ��pStaging*/

		/*�ڶ�������pStaging��ȡ��CPU��һ��ָ��imgData�д洢���ز�����QT*/
		D3D11_MAPPED_SUBRESOURCE mapped_pixels;
		hr = m_d3dDevContext->Map(pStaging.Get(), 0, D3D11_MAP_READ, 0, &mapped_pixels);
		this->imgData = new uchar[width*height * 4];/*���ٿռ�*/
		/*RGBA���������ģ�ÿһ�е����ݶ�����䣬�����Ҫ�����·�ʽȥ��ȡ����*/
		unsigned char* pData = reinterpret_cast<unsigned char*>(mapped_pixels.pData);
		for (UINT i = 0; i < height; ++i)
		{
			memcpy_s(&this->imgData[i * width * 4], width * 4, pData, width * 4);/*������*4*/
			pData += mapped_pixels.RowPitch;
		}
		m_d3dDevContext->Unmap(pStaging.Get(), 0);

		//����RGBA���ݡ�����alphaֵ�ķ�Χ��0-255
		//���˻�ɫ������updateFrame_Kinect()���ƹ��ˣ��ܽ����������ζ�Ų��ǻ�ɫ���������Է��ķ���
		sender_rgb.Send((char*)this->imgData, 1080 * 1920 * 4);
		////����ʱ��һ��ͼƬ
		//cv::Mat imgData_BGRA = cv::Mat(colorheight, colorwidth, CV_8UC4, this->imgData);
		//cvtColor(imgData_BGRA, imgData_BGRA, COLOR_BGRA2RGBA);
		//cv::imshow("dd", imgData_BGRA);
		//cv::imwrite("C:\\Users\\Administrator\\Desktop\\temp\\output1.png", imgData_BGRA);
		//imgData_BGRA.release();

		////��RGBA��RGB����ͨ��socket����
		//unsigned char* imgData_rgb = new uchar[width*height * 3];
		//for (int i = 0; i < colorwidth * colorheight; i++) {//RGBתBGR
		//	if (imgData[4 * i + 3] >= 1) {//��ֵ��ʱ��Ϊ1
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
		////���˻�ɫ������updateFrame_Kinect()���ƹ��ˣ��ܽ����������ζ�Ų��ǻ�ɫ���������Է��ķ���
		//sender_rgb.Send((char*)imgData_rgb, 1080 * 1920 * 3);
		////����ʱ��һ��ͼƬ
		//cv::Mat imgData_BGR = cv::Mat(colorheight, colorwidth, CV_8UC3, imgData_rgb);
		//cvtColor(imgData_BGR, imgData_BGR, COLOR_RGB2BGR);
		//cv::imshow("dd", imgData_BGR);

		this->count2++;
		free(this->imgData);
	}
	//ÿ֡�����궼��Ҫ������ǰ��̨�Ļ��������Ѻ�̨���õĶ�����ʾ����Ļ��
	m_swapChain->Present(0,0);	

}
/*ComPtr<ID3D11Debug> d3dDebug;
HRESULT hr = m_d3dDevice.As(&d3dDebug);
if (SUCCEEDED(hr))
{
hr = d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
}*/

/*�������*/
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

