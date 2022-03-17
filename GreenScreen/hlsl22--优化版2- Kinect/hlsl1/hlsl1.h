
/**********
ʵ��QTǰ�˴���
***********/
#pragma once
#include <QtWidgets/QMainWindow>
#include "ui_hlsl1.h"
#include<QFileDialog>/*�򿪱����ļ���ʱʹ��*/
#include<QMessageBox>/*��ͼ�񱨴�ʱʹ��*/
#include <QPixmap>
#include <QStatusBar>/*״̬��*/
#include <QMouseEvent>
#include<QImage>
#include<QScreen>
#include<QApplication>
#include <QDesktopWidget>/*QApplication::desktop()Ҫ�õ�*/
#include<QLabel>
#include<QObject>
#include<iostream>
#include"QtGuiClass.h"

#include <windows.h>
#include <string>
//#include<opencv2/opencv.hpp>/*����Ƶģʽ�й�*/
#include <time.h>/*��������ʱ��*/
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
	//�ض�����QWidget�������¼�����
	/*�˺����������ǵ������������Ļʱ��ȡ��Ļ����ɫ����ȡɫ���Ĺ���*/
	void mousePressEvent(QMouseEvent *e);
	QLabel *MousePosLabel;/*��ʾ������ɫ�ı�ǩ*/
	
public:
	VideoCapture cap;/*��ȡ��Ƶ---�õ���OpenCV*/
	QImage resultImage;/*�����Ľ��ͼƬ*/
	QtGuiClass *directX11;/*directX�������*/
	float screen_color[4];/*������ɫֵRGBA*/
	QString file_name;/*����ͼ���·�����п�����*/
	QString filename_bg;/*����ͼ��·��*/
	bool composite;/*�ϳ�ǰ������*/
	bool video;/*�����Ƿ�Ϊ��Ƶ*/
	bool ifKinect;/*�����Ƿ�ΪKinect*/

	float ScreenBalanceValue;/*��Ļƽ��ֵ*/
	float despill_factor;/*�����ϵ��*/
	float despill_balance;/*�����ƽ��*/
	int preBlurValue;/*Ԥģ��ֵ*/
	float clip_black, clip_white;/* ǯ�ƺ�ɫ��ɫ */
	int edge_kernel_radius;/*��Ե���İ뾶*/
	float edge_kernel_tolerance;/*��Ե�����ݲ�*/
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
	float gamma;/*gammaУ������*///26���
	float brightness;/*����*///26���
	float contrast;/*�Աȶ�*///26���
	float sharp;/*��*///26���
	/****************28�޸Ŀ�ʼ****************/
	float binarization_threshold;/*��ֵ������*/
	bool isBinary;/*�Ƿ���ж�ֵ��*/
	/****************28�޸Ľ���****************/

	void DisplayImage(QString filename);/*�������ͼƬ��ʾ��QTǰ�˴���*/
	void DisplayVideo(QString filename);/*��ʾ��Ƶ��һ֡*/
	void setParameter(QtGuiClass * dX);/*��directX�������ò���*/
/*�źźͲ۵��Զ�������������Ҫʹ��connect����*/
public slots:
	void on_SelectImageButton_clicked();/*����ǰ��ͼ��*/
	void on_SelectBackgroundButton_clicked();/*���뱳��ͼ��*/
	void on_CompositeRadioButton_clicked();/*ǰ�������ϳɰ�ť*/
	void on_SelectKinectButton_clicked();/*���Kinect��ť*/
	void on_getOneFrameButton_clicked();/*���Kinect��ȡһ֡��ť*/
	
	void on_SelectVideoButton_clicked();/*����ǰ����Ƶ*/
	void on_KeyingMatteButton_clicked();/*����ɰ�--����ͨ��*/
	void on_KeyingImageButton_clicked();/*���ǰ��--��ͨ��*/
	void on_ScreenBalanceSlider_valueChanged(int value);/*������Ļƽ��ֵ*/
	void on_despillFactorSlider_valueChanged(int);/*���÷����ϵ��*/
	void on_despillBalanceSlider_valueChanged(int);/*���÷����ƽ��*/
	void on_PreBlurSlider_valueChanged(int);/*����Ԥģ��ֵ*/
	void on_gammaSlider_valueChanged(int);/*����gammaУ��ֵ*///26���
	void on_brightnessSlider_valueChanged(int);/*��������ֵ*///26���
	void on_contrastSlider_valueChanged(int);/*���öԱȶ�ֵ*///26���
	void on_sharpSlider_valueChanged(int);/*���öԱȶ�ֵ*///26���
	/****************28�޸Ŀ�ʼ****************/
	void on_binarizationSlider_valueChanged(int value);/*���ö�ֵ��*/
	void on_binaryCheckBox_clicked();/*�Ƿ���ж�ֵ����ť*/
	/****************28�޸Ľ���****************/

	/*����ǯ�ƺ�ɫ��ɫֵ*/
	void on_edgeKernelRadiusSlider_valueChanged(int);
	void on_edgeKernelToleranceSlider_valueChanged(int);
	void on_clipBlackSlider_valueChanged(int);
	void on_clipWhiteSlider_valueChanged(int);
	void on_PostBlurSlider_valueChanged(int value);/*���ú���ģ��ֵ*/
	void on_DilateErodeSlider_valueChanged(int value);/*�������͸�ʴ*/
	/*�������������*/
	void on_featherDistanceSlider_valueChanged(int value);
	void on_featherFalloffComboBox_currentIndexChanged(int);
	/*�������ֺͺ��������������*/
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
