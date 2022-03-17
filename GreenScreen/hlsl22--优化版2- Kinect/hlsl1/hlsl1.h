
/**********
实现QT前端代码
***********/
#pragma once
#include <QtWidgets/QMainWindow>
#include "ui_hlsl1.h"
#include<QFileDialog>/*打开本地文件夹时使用*/
#include<QMessageBox>/*打开图像报错时使用*/
#include <QPixmap>
#include <QStatusBar>/*状态栏*/
#include <QMouseEvent>
#include<QImage>
#include<QScreen>
#include<QApplication>
#include <QDesktopWidget>/*QApplication::desktop()要用到*/
#include<QLabel>
#include<QObject>
#include<iostream>
#include"QtGuiClass.h"

#include <windows.h>
#include <string>
//#include<opencv2/opencv.hpp>/*与视频模式有关*/
#include <time.h>/*测试运行时间*/
//using namespace cv;
using namespace std;

class hlsl1 : public QMainWindow
{
	Q_OBJECT

public:
	hlsl1(QWidget *parent = Q_NULLPTR);
	~hlsl1();
private:
	Ui::hlsl1Class ui;

protected:
	//重定义了QWidget类的鼠标事件方法
	/*此函数的作用是当鼠标左键点击屏幕时读取屏幕的颜色，即取色器的功能*/
	void mousePressEvent(QMouseEvent *e);
	QLabel *MousePosLabel;/*显示背景颜色的标签*/
	
public:
	VideoCapture cap;/*读取视频---用到了OpenCV*/
	QImage resultImage;/*处理后的结果图片*/
	QtGuiClass *directX11;/*directX程序对象*/
	float screen_color[4];/*背景颜色值RGBA*/
	QString file_name;/*导入图像的路径进行抠像处理*/
	QString filename_bg;/*背景图像路径*/
	bool composite;/*合成前景背景*/
	bool video;/*输入是否为视频*/
	bool ifKinect;/*输入是否为Kinect*/

	float ScreenBalanceValue;/*屏幕平衡值*/
	float despill_factor;/*非溢出系数*/
	float despill_balance;/*非溢出平衡*/
	int preBlurValue;/*预模糊值*/
	float clip_black, clip_white;/* 钳制黑色白色 */
	int edge_kernel_radius;/*边缘核心半径*/
	float edge_kernel_tolerance;/*边缘核心容差*/
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
	float gamma;/*gamma校正参数*///26添加
	float brightness;/*亮度*///26添加
	float contrast;/*对比度*///26添加
	float sharp;/*锐化*///26添加
	/****************28修改开始****************/
	float binarization_threshold;/*二值化参数*/
	bool isBinary;/*是否进行二值化*/
	/****************28修改结束****************/

	void DisplayImage(QString filename);/*将导入的图片显示在QT前端窗口*/
	void DisplayVideo(QString filename);/*显示视频第一帧*/
	void setParameter(QtGuiClass * dX);/*给directX程序设置参数*/
/*信号和槽的自动关联，不再需要使用connect函数*/
public slots:
	void on_SelectImageButton_clicked();/*导入前景图像*/
	void on_SelectBackgroundButton_clicked();/*导入背景图像*/
	void on_CompositeRadioButton_clicked();/*前景背景合成按钮*/
	void on_SelectKinectButton_clicked();/*点击Kinect按钮*/
	void on_getOneFrameButton_clicked();/*点击Kinect获取一帧按钮*/
	
	void on_SelectVideoButton_clicked();/*导入前景视频*/
	void on_KeyingMatteButton_clicked();/*输出蒙版--不带通道*/
	void on_KeyingImageButton_clicked();/*输出前景--带通道*/
	void on_ScreenBalanceSlider_valueChanged(int value);/*设置屏幕平衡值*/
	void on_despillFactorSlider_valueChanged(int);/*设置非溢出系数*/
	void on_despillBalanceSlider_valueChanged(int);/*设置非溢出平衡*/
	void on_PreBlurSlider_valueChanged(int);/*设置预模糊值*/
	void on_gammaSlider_valueChanged(int);/*设置gamma校正值*///26添加
	void on_brightnessSlider_valueChanged(int);/*设置亮度值*///26添加
	void on_contrastSlider_valueChanged(int);/*设置对比度值*///26添加
	void on_sharpSlider_valueChanged(int);/*设置对比度值*///26添加
	/****************28修改开始****************/
	void on_binarizationSlider_valueChanged(int value);/*设置二值化*/
	void on_binaryCheckBox_clicked();/*是否进行二值化按钮*/
	/****************28修改结束****************/

	/*设置钳制黑色白色值*/
	void on_edgeKernelRadiusSlider_valueChanged(int);
	void on_edgeKernelToleranceSlider_valueChanged(int);
	void on_clipBlackSlider_valueChanged(int);
	void on_clipWhiteSlider_valueChanged(int);
	void on_PostBlurSlider_valueChanged(int value);/*设置后期模糊值*/
	void on_DilateErodeSlider_valueChanged(int value);/*设置膨胀腐蚀*/
	/*设置羽化所需参数*/
	void on_featherDistanceSlider_valueChanged(int value);
	void on_featherFalloffComboBox_currentIndexChanged(int);
	/*垃圾遮罩和核心遮罩所需参数*/
	void on_GarbageMatteCheckBox_clicked();
	void on_CoreMatteCheckBox_clicked();
	void on_MaskComboBox_currentIndexChanged(int index);
	void on_MaskComboBox_2_currentIndexChanged(int index);
	void on_MaskTypeComboBox_currentIndexChanged(int index);
	void on_MaskTypeComboBox_2_currentIndexChanged(int index);
	void on_XDoubleSpinBox_valueChanged(double);
	void on_YDoubleSpinBox_valueChanged(double);
	void on_WidthDoubleSpinBox_valueChanged(double value);
	void on_HeightDoubleSpinBox_valueChanged(double value);
	void on_RotationDoubleSpinBox_valueChanged(double value);
	void on_TransparentDoubleSpinBox_valueChanged(double value);
	void on_XDoubleSpinBox_2_valueChanged(double);
	void on_YDoubleSpinBox_2_valueChanged(double);
	void on_WidthDoubleSpinBox_2_valueChanged(double value);
	void on_HeightDoubleSpinBox_2_valueChanged(double value);
	void on_RotationDoubleSpinBox_2_valueChanged(double value);
	void on_TransparentDoubleSpinBox_2_valueChanged(double value);
};
