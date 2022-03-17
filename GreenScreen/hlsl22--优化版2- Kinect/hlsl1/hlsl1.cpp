/**********
ʵ��QTǰ�˴���
***********/
#include "hlsl1.h"
#include<qDebug>/*�������������Ϣ������̨*/

hlsl1::hlsl1(QWidget *parent): QMainWindow(parent)
{
	initKinect();//��ʼ��Kinect��Ҫ�����ʼŶ



	ui.setupUi(this);

	directX11 = NULL;/*��ʼ��directX����ΪNULL*/

	//���ô���׷�����
	this->setMouseTracking(true);

	/*��������ؼ���������ʾ������ɫֵ��*/
	MousePosLabel = new QLabel;
	MousePosLabel->setText(tr(""));
	MousePosLabel->setFixedWidth(200);
	//��QMainWindow��״̬���м���ؼ�
	statusBar()->addPermanentWidget(MousePosLabel);

	this->file_name = "";/*��ʼ������ͼƬ��·��*/
	this->filename_bg = "";/*��ʼ������ͼ��·��*/
	this->composite = false;
	this->video = false;
	this->ifKinect = false;//�����Ƿ�ΪKinect

	/*������ɫֵ��ʼ��*/
	//this->screen_color[0] = 0.0;
	//this->screen_color[1] = 0.8;
	//this->screen_color[2] = 0.0;
	//this->screen_color[3] = 1.0;
	this->screen_color[0] = 98.0/255;
	this->screen_color[1] = 175.0/255;
	this->screen_color[2] = 136.0/255;
	this->screen_color[3] = 1.0;

	/*ֻ��������������������100��*/
	ScreenBalanceValue = 0.5;
	ui.ScreenBalanceSlider->setMinimum(0);//���û������ؼ�����Сֵ
	ui.ScreenBalanceSlider->setMaximum(100);//���û������ؼ������ֵ
	ui.ScreenBalanceSlider->setValue(50);//���û������ؼ���ֵ����ʾ���м�
	ui.ScreenBalanceSlider->setSingleStep(1);//���û����Ĳ���

	//26���gamma��ֵ����
	gamma = 2;/*ȡֵ��Χ0-10*///26���
	ui.gammaSlider->setMinimum(0);//���û������ؼ�����Сֵ
	ui.gammaSlider->setMaximum(100);//���û������ؼ������ֵ
	ui.gammaSlider->setValue(20);//���û������ؼ���ֵ����ʾ���м�
	ui.gammaSlider->setSingleStep(1);//���û����Ĳ���

	/****************28�޸Ŀ�ʼ****************/
	binarization_threshold = 0.5;/*ȡֵ��Χ0-1*/
	ui.binarizationSlider->setMinimum(0);//���û������ؼ�����Сֵ
	ui.binarizationSlider->setMaximum(10);//���û������ؼ������ֵ
	ui.binarizationSlider->setValue(5);//���û������ؼ���ֵ����ʾ���м�
	ui.binarizationSlider->setSingleStep(1);//���û����Ĳ���

	this->isBinary = false;
	/****************28�޸Ľ���****************/

	//26���
	sharp = 0.0;/*ȡֵ��Χ0-10*/
	ui.sharpSlider->setMinimum(0);//���û������ؼ�����Сֵ
	ui.sharpSlider->setMaximum(100);//���û������ؼ������ֵ
	ui.sharpSlider->setValue(sharp);//���û������ؼ���ֵ����ʾ���м�
	//26���
	ui.gammaSlider->setSingleStep(1);//���û����Ĳ���
	brightness = 0.0;/*ȡֵ��Χ0-10*/
	ui.brightnessSlider->setMinimum(0);//���û������ؼ�����Сֵ
	ui.brightnessSlider->setMaximum(100);//���û������ؼ������ֵ
	ui.brightnessSlider->setValue(brightness);//���û������ؼ���ֵ����ʾ���м�
	ui.brightnessSlider->setSingleStep(1);//���û����Ĳ���
	//26���
	contrast = 5.0;/*ȡֵ��Χ0-20*/
	ui.contrastSlider->setMinimum(0);//���û������ؼ�����Сֵ
	ui.contrastSlider->setMaximum(200);//���û������ؼ������ֵ
	ui.contrastSlider->setValue(50);//���û������ؼ���ֵ����ʾ���м�
	ui.contrastSlider->setSingleStep(1);//���û����Ĳ���

	despill_factor = 1.0;
	ui.despillFactorSlider->setMinimum(0);
	ui.despillFactorSlider->setMaximum(10);
	ui.despillFactorSlider->setValue(10);//���û������ؼ���ֵ����ʾ���м�
	ui.despillFactorSlider->setSingleStep(1);//���û����Ĳ���

	despill_balance = 0.5;
	ui.despillBalanceSlider->setMinimum(0);
	ui.despillBalanceSlider->setMaximum(10);
	ui.despillBalanceSlider->setValue(5);//���û������ؼ���ֵ����ʾ���м�
	ui.despillBalanceSlider->setSingleStep(1);//���û����Ĳ���

	preBlurValue = 0;
	ui.PreBlurSlider->setMinimum(0);//���û������ؼ�����Сֵ
	ui.PreBlurSlider->setMaximum(20);//���û������ؼ������ֵ
	ui.PreBlurSlider->setValue(0);//���û������ؼ���ֵ����ʾ���м�
	ui.PreBlurSlider->setSingleStep(1);//���û����Ĳ���

	edge_kernel_radius = 3;
	ui.edgeKernelRadiusSlider->setMinimum(0);
	ui.edgeKernelRadiusSlider->setMaximum(20);/*��ԭʼ��Χ��һ��*/
	ui.edgeKernelRadiusSlider->setValue(3);//���û������ؼ���ֵ����ʾ���м�
	ui.edgeKernelRadiusSlider->setSingleStep(1);//���û����Ĳ���

	edge_kernel_tolerance = 0.1;
	ui.edgeKernelToleranceSlider->setMinimum(0);
	ui.edgeKernelToleranceSlider->setMaximum(10);
	ui.edgeKernelToleranceSlider->setValue(1);
	ui.edgeKernelToleranceSlider->setSingleStep(1);//���û����Ĳ���

	//26�޸�
	clip_black = 0.2;//0
	ui.clipBlackSlider->setMinimum(0);
	ui.clipBlackSlider->setMaximum(100);
	ui.clipBlackSlider->setValue(20);//���û������ؼ���ֵ����ʾ���м�
	ui.clipBlackSlider->setSingleStep(1);//���û����Ĳ���
	//26�޸�
	clip_white = 0.9;//0
	ui.clipWhiteSlider->setMinimum(0);
	ui.clipWhiteSlider->setMaximum(100);
	ui.clipWhiteSlider->setValue(90);//���û������ؼ���ֵ����ʾ���м�
	ui.clipWhiteSlider->setSingleStep(1);//���û����Ĳ���

	blur_post = 0;
	ui.PostBlurSlider->setMinimum(0);
	ui.PostBlurSlider->setMaximum(20);
	ui.PostBlurSlider->setValue(0);//���û������ؼ���ֵ����ʾ���м�
	ui.PostBlurSlider->setSingleStep(1);//���û����Ĳ���

	distance = 0;
	ui.DilateErodeSlider->setMinimum(-20);
	ui.DilateErodeSlider->setMaximum(20);
	ui.DilateErodeSlider->setValue(0);//���û������ؼ���ֵ����ʾ���м�
	ui.DilateErodeSlider->setSingleStep(1);

	feather_distance = 0;
	ui.featherDistanceSlider->setMinimum(-20);
	ui.featherDistanceSlider->setMaximum(20);
	ui.featherDistanceSlider->setValue(0);//���û������ؼ���ֵ����ʾ���м�
	ui.featherDistanceSlider->setSingleStep(1);

	isGarbageMatte = false;
	ui.GarbageMatteCheckBox->setChecked(false);/*���ó�ѡ��*/
	isCoreMatte = false;
	ui.CoreMatteCheckBox->setChecked(false);

	isBoxMask_Garbage = true;
	//isEllipseMask = false;
	isBoxMask_Core = true;
	//isEllipseMask2 = false;

	maskType_Garbage = 0;
	maskType_Core = 0;
	/*������������*/
	x_Garbage = 0.5;
	y_Garbage = 0.5;
	width_Garbage = 0.1;
	height_Garbage = 0.1;
	rotation_Garbage = 0;
	transparent_Garbage = 1;
	ui.XDoubleSpinBox->setRange(-1, 2);  // ��Χ
	ui.XDoubleSpinBox->setDecimals(3);  // ����
	ui.XDoubleSpinBox->setSingleStep(0.01); // ����
	ui.XDoubleSpinBox->setValue(0.5);//���ó�ʼֵ

	ui.YDoubleSpinBox->setRange(-1, 2);  // ��Χ
	ui.YDoubleSpinBox->setDecimals(3);  // ����
	ui.YDoubleSpinBox->setSingleStep(0.01); // ����
	ui.YDoubleSpinBox->setValue(0.5);

	ui.WidthDoubleSpinBox->setRange(0, 2);
	ui.WidthDoubleSpinBox->setDecimals(3);  // ����
	ui.WidthDoubleSpinBox->setSingleStep(0.05); // ����
	ui.WidthDoubleSpinBox->setValue(0.1);

	ui.HeightDoubleSpinBox->setRange(0, 2);
	ui.HeightDoubleSpinBox->setDecimals(3);  // ����
	ui.HeightDoubleSpinBox->setSingleStep(0.05); // ����
	ui.HeightDoubleSpinBox->setValue(0.1);

	ui.RotationDoubleSpinBox->setRange(0, 1800);
	ui.RotationDoubleSpinBox->setDecimals(1);  // ����
	ui.RotationDoubleSpinBox->setSingleStep(0.1); // ����
	ui.RotationDoubleSpinBox->setValue(0);
	//ui.RotationDoubleSpinBox->setSuffix("��");  // ��׺--����

	ui.TransparentDoubleSpinBox->setRange(0, 1);
	ui.TransparentDoubleSpinBox->setDecimals(3);  // ����
	ui.TransparentDoubleSpinBox->setSingleStep(0.1); // ����
	ui.TransparentDoubleSpinBox->setValue(1);

	/*������������*/
	x_Core = 0.5;
	y_Core = 0.5;
	width_Core = 0.1;
	height_Core = 0.1;
	rotation_Core = 0;
	transparent_Core = 1;
	ui.XDoubleSpinBox_2->setRange(-1, 2);  // ��Χ
	ui.XDoubleSpinBox_2->setDecimals(3);  // ����
	ui.XDoubleSpinBox_2->setSingleStep(0.01); // ����
	ui.XDoubleSpinBox_2->setValue(0.5);//���ó�ʼֵ

	ui.YDoubleSpinBox_2->setRange(-1, 2);  // ��Χ
	ui.YDoubleSpinBox_2->setDecimals(3);  // ����
	ui.YDoubleSpinBox_2->setSingleStep(0.01); // ����
	ui.YDoubleSpinBox_2->setValue(0.5);

	ui.WidthDoubleSpinBox_2->setRange(0, 2);
	ui.WidthDoubleSpinBox_2->setDecimals(3);  // ����
	ui.WidthDoubleSpinBox_2->setSingleStep(0.05); // ����
	ui.WidthDoubleSpinBox_2->setValue(0.1);

	ui.HeightDoubleSpinBox_2->setRange(0, 2);
	ui.HeightDoubleSpinBox_2->setDecimals(3);  // ����
	ui.HeightDoubleSpinBox_2->setSingleStep(0.05); // ����
	ui.HeightDoubleSpinBox_2->setValue(0.1);

	ui.RotationDoubleSpinBox_2->setRange(0, 1800);
	ui.RotationDoubleSpinBox_2->setDecimals(1);  // ����
	ui.RotationDoubleSpinBox_2->setSingleStep(0.1); // ����
	ui.RotationDoubleSpinBox_2->setValue(0);
	//ui.RotationDoubleSpinBox_2->setSuffix("��");  // ��׺--����

	ui.TransparentDoubleSpinBox_2->setRange(0, 1);
	ui.TransparentDoubleSpinBox_2->setDecimals(3);  // ����
	ui.TransparentDoubleSpinBox_2->setSingleStep(0.1); // ����
	ui.TransparentDoubleSpinBox_2->setValue(1);
}
/*��������*/
hlsl1::~hlsl1() {
	if (directX11 != NULL) {
		directX11->~QtGuiClass();
		directX11 = NULL;
	}
}
//mousePressEvent()����Ϊ��갴���¼���Ӧ����--���ñ�����ɫֵ
void hlsl1::mousePressEvent(QMouseEvent *e)
{
	if (e->button() == Qt::LeftButton)
	{
		int x = QCursor::pos().x();/*�Ӵ���ϵͳ�в�ѯλ��*/
		int y = QCursor::pos().y();
		//grabWidow:����������ͨ��ץȡ��QRect��x��y��width��height�����Ƶĸ������ڵ����ݹ��������ͼ��
		QPixmap pixmap = QPixmap::grabWindow(QApplication::desktop()->winId(), x, y, 1, 1);/*ץȡ����ͼ,�����Qt4�н�ȡȫ���Ĵ���*/
																						   /*grabWindow������������Ļ��ץȡ���أ������ǴӴ���ץȡ���أ�
																						   �������һ�����ֻ���ȫ������ץȡ�Ĵ��ڵ���һ�����ڣ���Ҳ���Դ�����Ĵ��ڻ�ȡ���ء�
																						   ͨ������ץȡ����ꡣ*/
		if (!pixmap.isNull()) {/*�������ͼ��ΪNULL*/
			QImage image = pixmap.toImage();//������ͼת��ΪQImage
			if (!image.isNull()) {//���image��Ϊ��
				if (image.valid(0, 0)) {/*����λ����Ч*/
					QColor color = image.pixel(0, 0);
					float r = color.red();
					float g = color.green();
					float b = color.blue();
					QString text = QString("Screen Color: %1, %2, %3").arg(r).arg(g).arg(b);//mousedPressed_B
					MousePosLabel->setText(text);
					this->screen_color[0] = r / 255;
					this->screen_color[1] = g / 255;
					this->screen_color[2] = b / 255;
					this->screen_color[3] = 1.0;
				}
			}
		}
	}
}



/*����ǰ��ͼ��*/
void hlsl1::on_SelectImageButton_clicked() {
	file_name = QFileDialog::getOpenFileName(this, "Select Picture", ".", "*.jpg *.png");
	this->video = false;
	DisplayImage(file_name);
}
/*��ʾͼ��*///26�޸�
void hlsl1::DisplayImage(QString filename) {
	QImage * img = new QImage;
	if (filename.isEmpty()) {
		return;
	}
	/****************29�޸Ŀ�ʼ****************/
	if (this->gamma > 0) {
		Mat mat = imread(filename.toStdString());
		int channels = mat.channels();/*��ȡ��ǰͼƬ��ͨ����*/
		if (channels == 3) {
			/*opencv��ȡ����ɫֵΪBGR��ʽ*/
			MatIterator_<Vec3b> it, end;
			for (it = mat.begin<Vec3b>(), end = mat.end<Vec3b>(); it != end; it++)
			{
				float tmp = (*it)[0] / 255.0;
				(*it)[0] = (tmp > 0.0f ? pow(tmp, gamma) : tmp)*255.0;
				tmp = (*it)[1] / 255.0;
				(*it)[1] = (tmp > 0.0f ? pow(tmp, gamma) : tmp)*255.0;
				tmp = (*it)[2] / 255.0;
				(*it)[2] = (tmp > 0.0f ? pow(tmp, gamma) : tmp)*255.0;
			}
			/*matת��ΪQImage*/
			cvtColor(mat, mat, CV_BGR2RGB);//Qt��֧�ֵ���RGBͼ��, OpenCV��֧�ֵ���BGR
		}
		if (channels == 4) {
			/*opencv��ȡ����ɫֵΪBGRA��ʽ*/
			MatIterator_<Vec4b> it, end;
			for (it = mat.begin<Vec4b>(), end = mat.end<Vec4b>(); it != end; it++)
			{
				float tmp = (*it)[0] / 255.0;
				(*it)[0] = (tmp > 0.0f ? pow(tmp, gamma) : tmp)*255.0;
				tmp = (*it)[1] / 255.0;
				(*it)[1] = (tmp > 0.0f ? pow(tmp, gamma) : tmp)*255.0;
				tmp = (*it)[2] / 255.0;
				(*it)[2] = (tmp > 0.0f ? pow(tmp, gamma) : tmp)*255.0;
				/*(*it)[3]��alphaͨ������*/
			}
			/*matת��ΪQImage*/
			cvtColor(mat, mat, CV_BGRA2RGB);//Qt��֧�ֵ���RGBͼ��, OpenCV��֧�ֵ���BGR
		}
		QImage image((const uchar *)mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888); //temp.setp()û��ʱ���ᵼ����ЩͼƬ��ת������б 
		image.bits(); // enforce deep copy, see documentation  
					  // of QImage::QImage ( const uchar * data, int width, int height, Format format )
					  //QImage image = QImage((uchar*)(mat.data), mat.cols, mat.rows, QImage::Format_RGB888);
		QImage * img = &image;
		/****************29�޸Ľ���****************/

		/*ͼ������--����ͼƬ����������*/
		QImage* imgScaled = new QImage;
		*imgScaled = img->scaled(ui.ImageDisplayLabel->width(), ui.ImageDisplayLabel->height(),
			Qt::IgnoreAspectRatio, Qt::SmoothTransformation);/*���ֳ������*/
		ui.ImageDisplayLabel->setPixmap(QPixmap::fromImage(*imgScaled)); // ��ͼƬ��ʾ��label��
		mat.release();
	}
	else {
		if (!(img->load(filename))) {
			QMessageBox::information(this, tr("��ͼ��ʧ��"), tr("��ͼ��ʧ��!"));
			delete img;
			return;
		}
		/*ͼ������--����ͼƬ����������*/
		QImage* imgScaled = new QImage;
		*imgScaled = img->scaled(ui.ImageDisplayLabel->width(), ui.ImageDisplayLabel->height(),
			Qt::IgnoreAspectRatio, Qt::SmoothTransformation);/*���ֳ������*/
		ui.ImageDisplayLabel->setPixmap(QPixmap::fromImage(*imgScaled)); // ��ͼƬ��ʾ��label��
	}
}
///*��ʾͼ��*/
//void hlsl1::DisplayImage(QString filename) {
//	QImage * img = new QImage;
//	if (filename.isEmpty()) {
//		return;
//	}
//	else {
//		if (!(img->load(filename))) {
//			QMessageBox::information(this, tr("��ͼ��ʧ��"), tr("��ͼ��ʧ��!"));
//			delete img;
//			return;
//		}
//	}
//	/*ͼ������--����ͼƬ����������*/
//	QImage* imgScaled = new QImage;
//	*imgScaled = img->scaled(ui.ImageDisplayLabel->width(), ui.ImageDisplayLabel->height(),
//		Qt::IgnoreAspectRatio, Qt::SmoothTransformation);/*���ֳ������*/
//	ui.ImageDisplayLabel->setPixmap(QPixmap::fromImage(*imgScaled)); // ��ͼƬ��ʾ��label��
//}

/*����ǰ����Ƶ*/
void hlsl1::on_SelectVideoButton_clicked() {
	/*��Ƶ��ʽ����Щ??��������ٶ��һЩ*/
	file_name = QFileDialog::getOpenFileName(this, "Select video", ".", "*.MOV");
	video = true;
	DisplayVideo(file_name);/*����Ƶ��ʾ��ǰ�˽���*/
}
/*��ʾԭʼ��Ƶ��һ֡*/
void hlsl1::DisplayVideo(QString filename) {
	/*���������Ƶ*/
	cap.open(filename.toStdString()); //����Ƶ
	if (!cap.isOpened())//�����Ƶ�����������򷵻�
		return;
	Mat src;
	cap >> src;//�ȼ���cap.read(frame);����Ƶ�е�֡����mat��
	if (src.empty())//���ĳ֡Ϊ�����˳�ѭ��
		return;
	cvtColor(src, src, CV_BGR2RGB);//Qt��֧�ֵ���RGBͼ��, OpenCV��֧�ֵ���BGR
	QImage image = QImage((uchar*)(src.data), src.cols, src.rows, QImage::Format_RGB888);
	QImage * img = &image;
	/*ͼ������--����ͼƬ����������*/
	QImage* imgScaled = new QImage;
	*imgScaled = img->scaled(ui.ImageDisplayLabel->width(), ui.ImageDisplayLabel->height(),
		Qt::IgnoreAspectRatio, Qt::SmoothTransformation);/*���ֳ������*/
	ui.ImageDisplayLabel->setPixmap(QPixmap::fromImage(*imgScaled));
	src.release();/*�ͷ�src*/
}
/*���뱳��ͼ��*/
void hlsl1::on_SelectBackgroundButton_clicked() {
	filename_bg = QFileDialog::getOpenFileName(this, "SelectBackgroundPicture", ".", "*.jpg");
	if (video == false) {
		DisplayImage(filename_bg);
	}
}
/*�ϳɰ�ť*/
void hlsl1::on_CompositeRadioButton_clicked() {
	if (composite == false) {
		composite = true;
	}
	else {
		composite = false;
		//DisplayImage(file_name);
	}
}
/*������Ļƽ��ֵ*/
void hlsl1::on_ScreenBalanceSlider_valueChanged(int value) {
	ScreenBalanceValue = value / 100.0;
	ui.ScreenBalanceValueLabel->setText(QString::number(ScreenBalanceValue));
}
/*���÷����ϵ��*/
void hlsl1::on_despillFactorSlider_valueChanged(int value) {
	despill_factor = value / 10.0;
	ui.despillFactorValueLabel->setText(QString::number(despill_factor));
}
/*���÷����ƽ��*/
void hlsl1::on_despillBalanceSlider_valueChanged(int value) {
	despill_balance = value / 10.0;
	ui.despillBalanceValueLabel->setText(QString::number(despill_balance));
}
/*����Ԥģ��ֵ*/
void hlsl1::on_PreBlurSlider_valueChanged(int value) {
	preBlurValue = value;
	ui.PreBlurValueLabel->setText(QString::number(preBlurValue));
}
/*����gammaֵ*/
void hlsl1::on_gammaSlider_valueChanged(int value) {
	gamma = value / 10.0;
	ui.gammaValueLabel->setText(QString::number(gamma));
	if (video == false) {
		DisplayImage(file_name);/*������ʾͼ��*/
	}
	else {
		DisplayVideo(file_name);/*������ʾ��Ƶ��һ֡*/
	}
}
/*����brightnessֵ*///26���
void hlsl1::on_brightnessSlider_valueChanged(int value) {
	brightness = value / 10.0;
	ui.brightnessValueLabel->setText(QString::number(brightness));
}

/****************28�޸Ŀ�ʼ****************/
/*���ö�ֵ��*/
void hlsl1::on_binarizationSlider_valueChanged(int value) {
	binarization_threshold = value / 10.0;
	ui.binarizationValueLabel->setText(QString::number(binarization_threshold));
}
/*�Ƿ���ж�ֵ����ť*/
void hlsl1::on_binaryCheckBox_clicked() {/*�Ƿ���ж�ֵ����ť*/
	if (!ui.binaryCheckBox->checkState()) {
		isBinary = false;
		ui.binaryCheckBox->setChecked(false);
	}
	else {
		isBinary = true;
		ui.binaryCheckBox->setChecked(true);
	}
}
/****************28�޸Ľ���****************/

/*����sharpֵ*///26���
void hlsl1::on_sharpSlider_valueChanged(int value) {
	sharp = value / 10.0;
	ui.sharpValueLabel->setText(QString::number(sharp));
}
/*����contrastֵ*///26���
void hlsl1::on_contrastSlider_valueChanged(int value) {
	contrast = value / 10.0;
	ui.contrastValueLabel->setText(QString::number(contrast));
}
/*���ñ�Ե���İ뾶*/
void hlsl1::on_edgeKernelRadiusSlider_valueChanged(int value) {
	edge_kernel_radius = value;
	ui.edgeKernelRadiusValueLabel->setText(QString::number(edge_kernel_radius));
}
/*���ñ�Ե�����ݲ�*/
void hlsl1::on_edgeKernelToleranceSlider_valueChanged(int value) {
	edge_kernel_tolerance = value / 10.0;
	ui.edgeKernelToleranceValueLabel->setText(QString::number(edge_kernel_tolerance));
}
/*����ǯ�ƺ�ɫ*///26�޸�
void hlsl1::on_clipBlackSlider_valueChanged(int value) {
	clip_black = value / 100.0;
	ui.clipBlackValueLabel->setText(QString::number(clip_black));
}
/*����ǯ�ư�ɫ*///26�޸�
void hlsl1::on_clipWhiteSlider_valueChanged(int value) {
	clip_white = value / 100.0;
	ui.clipWhiteValueLabel->setText(QString::number(clip_white));
}
/*���ú���ģ��*/
void hlsl1::on_PostBlurSlider_valueChanged(int value) {
	blur_post = value;
	ui.PostBlurValueLabel->setText(QString::number(blur_post));
}
/*�������͸�ʴ*/
void hlsl1::on_DilateErodeSlider_valueChanged(int value) {
	distance = value;
	ui.DilateErodeValueLabel->setText(QString::number(distance));
}
/*�����𻯾���*/
void hlsl1::on_featherDistanceSlider_valueChanged(int value) {
	feather_distance = value;
	ui.featherDistanceValueLabel->setText(QString::number(feather_distance));
}
/*������˥��*/
void hlsl1::on_featherFalloffComboBox_currentIndexChanged(int index)
{
	//����ǰ������ֵ������index�������ǰѡ����
	index = ui.featherFalloffComboBox->currentIndex();
	switch (index)
	{
	case 0:
		feather_falloff = 0;
		break;
	case 1:
		feather_falloff = 1;
		break;
	case 2:
		feather_falloff = 2;
		break;
	case 3:
		feather_falloff = 7;
		break;
	case 4:
		feather_falloff = 3;
		break;
	case 5:
		feather_falloff = 4;
		break;
	}
	ui.featherFalloffValueLabel->setText(QString::number(feather_falloff));
}
/*������������*/
void hlsl1::on_GarbageMatteCheckBox_clicked() {
	if (!ui.GarbageMatteCheckBox->checkState()) {
		isGarbageMatte = false;
		ui.GarbageMatteCheckBox->setChecked(false);
	}
	else {
		isGarbageMatte = true;
		ui.GarbageMatteCheckBox->setChecked(true);
	}
}
//��������-����ǰ����MaskComboBoxֵ�����ı��򴥷��˺���
void hlsl1::on_MaskComboBox_currentIndexChanged(int index)
{
	//����ǰ������ֵ������index�������ǰѡ����
	index = ui.MaskComboBox->currentIndex();
	switch (index) {
	case 0:
		isBoxMask_Garbage = true;
		//isEllipseMask_Garbage = false;
		break;
	case 1:
		isBoxMask_Garbage = false;
		//isEllipseMask_Garbage = true;
		break;
	}
}
//�������ֵ���������-����ǰ����MaskTypeComboBoxֵ�����ı��򴥷��˺���
void hlsl1::on_MaskTypeComboBox_currentIndexChanged(int index)
{
	//����ǰ������ֵ������index�������ǰѡ����
	index = ui.MaskTypeComboBox->currentIndex();
	switch (index) {
	case 0:
		maskType_Garbage = 0;
		break;
	case 1:
		maskType_Garbage = 1;
		break;
	case 2:
		maskType_Garbage = 2;
		break;
	case 3:
		maskType_Garbage = 3;
		break;
	}
}
/*��������*/
void hlsl1::on_XDoubleSpinBox_valueChanged(double value) {
	x_Garbage = value;
}
void hlsl1::on_YDoubleSpinBox_valueChanged(double value) {
	y_Garbage = value;
}
void hlsl1::on_WidthDoubleSpinBox_valueChanged(double value) {
	width_Garbage = value;
}
void hlsl1::on_HeightDoubleSpinBox_valueChanged(double value) {
	height_Garbage = value;
}
void hlsl1::on_RotationDoubleSpinBox_valueChanged(double value) {
	rotation_Garbage = value;
}
void hlsl1::on_TransparentDoubleSpinBox_valueChanged(double value) {
	transparent_Garbage = value;
}
/*���ú�������*/
void hlsl1::on_CoreMatteCheckBox_clicked() {
	if (!ui.CoreMatteCheckBox->checkState()) {
		isCoreMatte = false;
		ui.CoreMatteCheckBox->setChecked(false);
	}
	else {
		isCoreMatte = true;
		ui.CoreMatteCheckBox->setChecked(true);
	}
}
/*��������*/
void hlsl1::on_MaskComboBox_2_currentIndexChanged(int index)
{
	//����ǰ������ֵ������index�������ǰѡ����
	index = ui.MaskComboBox_2->currentIndex();
	switch (index) {
	case 0:
		isBoxMask_Core = true;
		//isEllipseMask2 = false;
		break;
	case 1:
		isBoxMask_Core = false;
		//isEllipseMask2 = true;
		break;
	}
}
/*�������ֵ���������-����ǰ����MaskTypeComboBoxֵ�����ı��򴥷��˺���*/
void hlsl1::on_MaskTypeComboBox_2_currentIndexChanged(int index)
{
	//����ǰ������ֵ������index�������ǰѡ����
	index = ui.MaskTypeComboBox_2->currentIndex();
	switch (index) {
	case 0:
		maskType_Core = 0;
		break;
	case 1:
		maskType_Core = 1;
		break;
	case 2:
		maskType_Core = 2;
		break;
	case 3:
		maskType_Core = 3;
		break;
	}
}
/*��������*/
void hlsl1::on_XDoubleSpinBox_2_valueChanged(double value) {
	x_Core = value;
}
void hlsl1::on_YDoubleSpinBox_2_valueChanged(double value) {
	y_Core = value;
}
void hlsl1::on_WidthDoubleSpinBox_2_valueChanged(double value) {
	width_Core = value;
}
void hlsl1::on_HeightDoubleSpinBox_2_valueChanged(double value) {
	height_Core = value;
}
void hlsl1::on_RotationDoubleSpinBox_2_valueChanged(double value) {
	rotation_Core = value;
}
void hlsl1::on_TransparentDoubleSpinBox_2_valueChanged(double value) {
	transparent_Core = value;
}
/*����directX��������Ĳ���*/
void hlsl1::setParameter(QtGuiClass * dX) {
	/*��Ҫ������Ļƽ��ֵ+��Ļ������ɫ+��ͨ��*/
	dX->ScreenBalanceValue = this->ScreenBalanceValue;//0.5
	dX->ScreenColorValue[0] = this->screen_color[0];//0.0
	dX->ScreenColorValue[1] = this->screen_color[1];//0.8
	dX->ScreenColorValue[2] = this->screen_color[2];//0.0
	dX->ScreenColorValue[3] = this->screen_color[3];//1.0
	/*ȥ����ɫ�����������*/
	dX->DespillFactor = this->despill_factor;//1.0
	dX->DespillBalance = this->despill_balance;//0.5
	/*ͼ��Ԥģ��������������*/
	dX->preBlurValue = this->preBlurValue;//0
	/*gammaУ��*///26���
	dX->gamma = this->gamma;
	/*���ȺͶԱȶ���ǿ*///26���
	dX->brightness = this->brightness;
	dX->contrast = this->contrast;
	dX->sharp_value = this->sharp;

	/****************28�޸Ŀ�ʼ****************/
	/*��ֵ������*/
	dX->binarization_threshold = this->binarization_threshold;
	dX->isBinary = this->isBinary;
	/****************28�޸Ľ���****************/

	/*ǯ�ƺ�ɫ��ɫ��������*/
	dX->clipBlack = this->clip_black;
	dX->clipWhite = this->clip_white;
	dX->kernelRadius = this->edge_kernel_radius;
	dX->kernelTolerance = this->edge_kernel_tolerance;
	/*����ģ����������*/
	dX->blur_post = this->blur_post;
	/*���͸�ʴ��������*/
	dX->distance = this->distance;
	/*���������*/
	dX->feather_distance = this->feather_distance;
	dX->feather_falloff = this->feather_falloff;
	/*���������������*/
	dX->isGarbageMatte = this->isGarbageMatte;
	dX->isBoxMask_Garbage = this->isBoxMask_Garbage;
	dX->x_Garbage = this->x_Garbage;
	dX->y_Garbage = this->y_Garbage;
	dX->width_Garbage = this->width_Garbage;
	dX->height_Garbage = this->height_Garbage;
	dX->rotation_Garbage = this->rotation_Garbage;
	dX->transparent_Garbage = this->transparent_Garbage;
	dX->maskType_Garbage = this->maskType_Garbage;
	/*���������������*/
	dX->isCoreMatte = this->isCoreMatte;
	dX->isBoxMask_Core = this->isBoxMask_Core;
	dX->x_Core = this->x_Core;
	dX->y_Core = this->y_Core;
	dX->width_Core = this->width_Core;
	dX->height_Core = this->height_Core;
	dX->rotation_Core = this->rotation_Core;
	dX->transparent_Core = this->transparent_Core;
	dX->maskType_Core = this->maskType_Core;
	/*�ϳ�*/
	/*dX->filename_bg = this->filename_bg.toStdString();
	dX->composite = this->composite;*/
	dX->setComposite(this->composite, this->filename_bg.toStdString());
}

/*���Kinect��ȡһ֡��ť*/
void hlsl1::on_getOneFrameButton_clicked() {
	/////////////////////////////////////////////////
	///////         ��Kinect�л�ȡһ֡��ʾ��ui.ImageDisplayLabel��
	///////      ע�⣬��ʼ���г���󣬵ù��������ӣ���Kinect��ͨ���ٵ�˰�ť
	/////////////////////////////////////////////////

	unsigned char * colorFlow = (unsigned char*)malloc(sizeof(unsigned char) * colorwidth * colorheight * 3);
	unsigned short * depthFlow = (unsigned short *)malloc(sizeof(unsigned short)* colorwidth*colorheight);
	//cv::waitKey(30);
	getKinectData(colorFlow, depthFlow);
	Mat src = cv::Mat(colorheight, colorwidth, CV_8UC3, colorFlow);//��ɫͼ����src������ǰ����Ⱦ

	/****************27�޸Ŀ�ʼ****************/
	if (this->gamma > 0) {
		MatIterator_<Vec3b> it, end;
		for (it = src.begin<Vec3b>(), end = src.end<Vec3b>(); it != end; it++)
		{
			float tmp = (*it)[0] / 255.0;
			(*it)[0] = (tmp > 0.0f ? pow(tmp, gamma) : tmp)*255.0;
			tmp = (*it)[1] / 255.0;
			(*it)[1] = (tmp > 0.0f ? pow(tmp, gamma) : tmp)*255.0;
			tmp = (*it)[2] / 255.0;
			(*it)[2] = (tmp > 0.0f ? pow(tmp, gamma) : tmp)*255.0;
		}
	}
	/*matת��ΪQImage*/
	cvtColor(src, src, CV_BGR2RGB);//Qt��֧�ֵ���RGBͼ��, OpenCV��֧�ֵ���BGR
	QImage image((const uchar *)src.data, src.cols, src.rows, src.step, QImage::Format_RGB888); //temp.setp()û��ʱ���ᵼ����ЩͼƬ��ת������б 
	image.bits(); // enforce deep copy, see documentation  
	/****************27�޸Ľ���****************/


	//cvtColor(src, src, CV_BGR2RGB);//Qt��֧�ֵ���RGBͼ��, OpenCV��֧�ֵ���BGR
	//QImage image = QImage((uchar*)(src.data), src.cols, src.rows, QImage::Format_RGB888);
	QImage * img = &image;
	/*ͼ������--����ͼƬ����������*/
	QImage* imgScaled = new QImage;
	*imgScaled = img->scaled(ui.ImageDisplayLabel->width(), ui.ImageDisplayLabel->height(),
		Qt::IgnoreAspectRatio, Qt::SmoothTransformation);/*���ֳ������*/
	ui.ImageDisplayLabel->setPixmap(QPixmap::fromImage(*imgScaled));
	src.release();/*�ͷ�src*/

}
/*���Kinect��ť*/
void hlsl1::on_SelectKinectButton_clicked() {
	/*�����һ�β�������������*/
	if (directX11 != NULL) {
		directX11->~QtGuiClass();
		directX11 = NULL;
	}
	this->video = false;
	this->ifKinect = true;

	directX11 = new QtGuiClass(false, true);
	//directX11->flag = 0;/*����ɰ�*/
	directX11->flag = 1;/*���ǰ��*/
	this->setParameter(directX11);
	directX11->setConstantBuffers();/*���ó���������*/
	directX11->show();/*ֻ����ʾ*/
	
}

/*������ɰ水ť��������ɰ�--����ͨ��*/
void hlsl1::on_KeyingMatteButton_clicked() {
	/*�����һ�β�������������*/
	if (directX11 != NULL) {
		directX11->~QtGuiClass();
		directX11 = NULL;
	}
	//this->video = false;
	/*�Ե���ͼƬ���д���*/
	if (file_name != NULL&&file_name != ""&&this->video == false) {/*�Ե���ͼƬ���д���*/
		string fileName = this->file_name.toStdString();/*QString��String��ת��*/
		qDebug() << "--------new start" << endl;
		directX11 = new QtGuiClass(fileName,false);
		qDebug() << "--------new end" << endl;
		directX11->flag = 0;/*����ɰ�*/
		this->setParameter(directX11);
		directX11->setConstantBuffers();
		qDebug() << "--------show() start" << endl;
		directX11->show();/*ֻ����ʾ*/
		qDebug() << "--------show() end" << endl;

		//resultImage = QImage(directX11->imgData, directX11->width, directX11->height, QImage::Format_RGBA8888);
		//resultImage = resultImage.mirrored(false, true);/*��ͼƬ��ֱ��ת*/
		//								
		///*ͼ������--����ͼƬ����������*/
		//QImage* imgScaled = new QImage;
		//*imgScaled = resultImage.scaled(ui.ResultDisplayLabel->width(), ui.ResultDisplayLabel->height(),
		//	Qt::IgnoreAspectRatio, Qt::SmoothTransformation);/*���ֳ������*/
		//ui.ResultDisplayLabel_2->setPixmap(QPixmap::fromImage(*imgScaled)); // ��ͼƬ��ʾ��label��

		//free(directX11->imgData);
	}
	/*����Ƶ���д���*/
	if (file_name != NULL&&file_name != ""&&this->video==true) {
		string fileName = this->file_name.toStdString();/*QString��String��ת��*/
		directX11 = new QtGuiClass(fileName, true);
		directX11->flag = 0;/*����ɰ�*/
		this->setParameter(directX11);
		directX11->setConstantBuffers();/*���ó���������*/
		directX11->show();/*ֻ����ʾ*/
	}
}
/*���ǰ��ͼ--��ͨ��*/
void hlsl1::on_KeyingImageButton_clicked() {
	/*�����һ�β�������������*/
	if (directX11 != NULL) {
		directX11->~QtGuiClass();
		directX11 = NULL;
	}
	/*�Ե���ͼƬ���д���*/
	if (file_name != NULL&&file_name != ""&&this->video == false) {/*�Ե���ͼƬ���д���*/
		string fileName = this->file_name.toStdString();/*QString��String��ת��*/
		directX11 = new QtGuiClass(fileName,false);
		directX11->flag = 1;/*���ǰ��*/
		this->setParameter(directX11);
		directX11->setConstantBuffers();
		directX11->show();

		//resultImage = QImage(directX11->imgData, directX11->width, directX11->height, QImage::Format_RGBA8888);
		//resultImage = resultImage.mirrored(false, true);/*��ͼƬ��ֱ��ת*/

		///*ͼ������--����ͼƬ����������*/
		//QImage* imgScaled = new QImage;
		//*imgScaled = resultImage.scaled(ui.ResultDisplayLabel->width(), ui.ResultDisplayLabel->height(),
		//	Qt::IgnoreAspectRatio, Qt::SmoothTransformation);/*���ֳ������*/
		//ui.ResultDisplayLabel_2->setPixmap(QPixmap::fromImage(*imgScaled)); // ��ͼƬ��ʾ��label��

		//free(directX11->imgData);
	}
	/*����Ƶ���д���*/
	if (file_name != NULL&&file_name != ""&&this->video == true) {
		string fileName = this->file_name.toStdString();/*QString��String��ת��*/
		directX11 = new QtGuiClass(fileName, true);
		directX11->flag = 1;/*����ɰ�*/
		this->setParameter(directX11);
		directX11->setConstantBuffers();
		directX11->show();/*ֻ����ʾ*/
	}
}
