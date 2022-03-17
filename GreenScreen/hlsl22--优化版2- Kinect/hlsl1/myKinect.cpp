#include "myKinect.h"


// Intermediate Buffers
unsigned char rgbimage[colorwidth*colorheight * 4];    // Stores RGB color image
//Kinect�������
ColorSpacePoint depth2rgb[depthwidth*depthheight];             // Maps depth pixels to rgb pixels
DepthSpacePoint rgb2depth[colorwidth*colorheight];

CameraSpacePoint depth2xyz[depthwidth*depthheight];			 // Maps depth pixels to 3d coordinates

// Kinect Variables
IKinectSensor* sensor;             // Kinect sensor
IMultiSourceFrameReader* reader;   // Kinect data source
ICoordinateMapper* mapper;         // Converts between depth, color, and 3d coordinates

//��ʼ��Kinect����main�����Ŀ�ͷ������
bool initKinect() {
	if (FAILED(GetDefaultKinectSensor(&sensor))) {
		return false;
	}
	if (sensor) {
		sensor->get_CoordinateMapper(&mapper);

		sensor->Open();
		sensor->OpenMultiSourceFrameReader(
			FrameSourceTypes::FrameSourceTypes_Depth | FrameSourceTypes::FrameSourceTypes_Color,
			&reader);
		return reader;
	}
	else {
		return false;
	}
}


//��frame��ȡ�����ͼƬ������������
void getDepthData(IMultiSourceFrame* frame, unsigned short * ffdest) {
	IDepthFrame* depthframe;
	IDepthFrameReference* frameref = NULL;
	frame->get_DepthFrameReference(&frameref);
	frameref->AcquireFrame(&depthframe);
	if (frameref) frameref->Release();

	if (!depthframe) return;

	// Get data from frame
	unsigned int sz;
	unsigned short* buf;
	depthframe->AccessUnderlyingBuffer(&sz, &buf);

	// Fill in depth2rgb map//�õ�depth2rgb����getRgbData()����ʹ��
	mapper->MapDepthFrameToColorSpace(depthwidth*depthheight, buf, depthwidth*depthheight, depth2rgb);

	//�����ͼӳ�䵽rgb
	// Fill in rgb2depth map
	mapper->MapColorFrameToDepthSpace(depthwidth*depthheight, buf, colorwidth*colorheight, rgb2depth);
	//unsigned short * ffdest = (unsigned short *)malloc(sizeof(unsigned short)* colorwidth*colorheight);
	if (ffdest == NULL)
		exit(0);
	for (int i = 0; i < colorwidth*colorheight; i++) {
		DepthSpacePoint p = rgb2depth[i];
		// Check if color pixel coordinates are in bounds
		if (p.X < 0 || p.Y < 0 || p.X > depthwidth || p.Y > depthheight) {
			ffdest[i] = 0;
		}
		else {
			int idx = (int)p.X + depthwidth * (int)p.Y;
			ffdest[i] = buf[idx];
		}
	}
	//socket��������
	//sender_depth.Send((char *)ffdest, 1080 * 1920 * 2);//��Ϊunsigned short��16λ��char��8λ�����������ͨ��������2��

	if (depthframe) depthframe->Release();

	//free(ffdest);
}

void getRgbData(IMultiSourceFrame* frame, unsigned char * ffdest) {
	IColorFrame* colorframe;
	IColorFrameReference* frameref = NULL;
	frame->get_ColorFrameReference(&frameref);
	frameref->AcquireFrame(&colorframe);
	if (frameref) frameref->Release();

	if (!colorframe) return;

	// Get data from frame
	colorframe->CopyConvertedFrameDataToArray(colorwidth*colorheight * 4, rgbimage, ColorImageFormat_Rgba);
	if (colorframe) colorframe->Release();

	//unsigned char* ffdest = (unsigned char*)malloc(sizeof(unsigned char) * colorwidth * colorheight * 3);
	for (int i = 0; i < colorwidth * colorheight; i++) {//RGBתBGR
		ffdest[3 * i] = rgbimage[4 * i + 2];
		ffdest[3 * i + 1] = rgbimage[4 * i + 1];
		ffdest[3 * i + 2] = rgbimage[4 * i + 0];
	}
	//sender_rgb.Send((char*)ffdest, 1080 * 1920 * 3);
	////����RGB
	//for (; flag < 3; flag++)
	//{
	//	cv::Mat mColorImg(colorheight, colorwidth, CV_8UC3, ffdest);
	//	cv::imwrite("photos\\color\\" + to_string(indexOfPhoto) + ".png", mColorImg);

	//}
}

//��Kinect�л�ȡ����
void getKinectData(unsigned char * colorFlow, unsigned short * depthFlow) {//RGB�����������

	IMultiSourceFrame* frame = NULL;//Kinect.h
	if (SUCCEEDED(reader->AcquireLatestFrame(&frame))) {
		getDepthData(frame, depthFlow);//��Frame��ȡ���������(ת����rgb��size)������
		getRgbData(frame, colorFlow);
	}
	if (frame) frame->Release();
}