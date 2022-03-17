/**********
实现QT前端代码
***********/
#include "hlsl1.h"
#include<qDebug>/*用于输出错误信息到控制台*/

hlsl1::hlsl1(QWidget *parent): QMainWindow(parent)
{
	initKinect();//初始化Kinect。要放在最开始哦



	ui.setupUi(this);

	directX11 = NULL;/*初始化directX程序为NULL*/

	//设置窗体追踪鼠标
	this->setMouseTracking(true);

	/*以下这个控件是用来显示背景颜色值的*/
	MousePosLabel = new QLabel;
	MousePosLabel->setText(tr(""));
	MousePosLabel->setFixedWidth(200);
	//在QMainWindow的状态栏中加入控件
	statusBar()->addPermanentWidget(MousePosLabel);

	this->file_name = "";/*初始化导入图片的路径*/
	this->filename_bg = "";/*初始化背景图像路径*/
	this->composite = false;
	this->video = false;
	this->ifKinect = false;//输入是否为Kinect

	/*背景颜色值初始化*/
	//this->screen_color[0] = 0.0;
	//this->screen_color[1] = 0.8;
	//this->screen_color[2] = 0.0;
	//this->screen_color[3] = 1.0;
	this->screen_color[0] = 98.0/255;
	this->screen_color[1] = 175.0/255;
	this->screen_color[2] = 136.0/255;
	this->screen_color[3] = 1.0;

	/*只可以是整数，所以扩大100倍*/
	ScreenBalanceValue = 0.5;
	ui.ScreenBalanceSlider->setMinimum(0);//设置滑动条控件的最小值
	ui.ScreenBalanceSlider->setMaximum(100);//设置滑动条控件的最大值
	ui.ScreenBalanceSlider->setValue(50);//设置滑动条控件的值，显示在中间
	ui.ScreenBalanceSlider->setSingleStep(1);//设置滑动的步长

	//26添加gamma初值设置
	gamma = 2;/*取值范围0-10*///26添加
	ui.gammaSlider->setMinimum(0);//设置滑动条控件的最小值
	ui.gammaSlider->setMaximum(100);//设置滑动条控件的最大值
	ui.gammaSlider->setValue(20);//设置滑动条控件的值，显示在中间
	ui.gammaSlider->setSingleStep(1);//设置滑动的步长

	/****************28修改开始****************/
	binarization_threshold = 0.5;/*取值范围0-1*/
	ui.binarizationSlider->setMinimum(0);//设置滑动条控件的最小值
	ui.binarizationSlider->setMaximum(10);//设置滑动条控件的最大值
	ui.binarizationSlider->setValue(5);//设置滑动条控件的值，显示在中间
	ui.binarizationSlider->setSingleStep(1);//设置滑动的步长

	this->isBinary = false;
	/****************28修改结束****************/

	//26添加
	sharp = 0.0;/*取值范围0-10*/
	ui.sharpSlider->setMinimum(0);//设置滑动条控件的最小值
	ui.sharpSlider->setMaximum(100);//设置滑动条控件的最大值
	ui.sharpSlider->setValue(sharp);//设置滑动条控件的值，显示在中间
	//26添加
	ui.gammaSlider->setSingleStep(1);//设置滑动的步长
	brightness = 0.0;/*取值范围0-10*/
	ui.brightnessSlider->setMinimum(0);//设置滑动条控件的最小值
	ui.brightnessSlider->setMaximum(100);//设置滑动条控件的最大值
	ui.brightnessSlider->setValue(brightness);//设置滑动条控件的值，显示在中间
	ui.brightnessSlider->setSingleStep(1);//设置滑动的步长
	//26添加
	contrast = 5.0;/*取值范围0-20*/
	ui.contrastSlider->setMinimum(0);//设置滑动条控件的最小值
	ui.contrastSlider->setMaximum(200);//设置滑动条控件的最大值
	ui.contrastSlider->setValue(50);//设置滑动条控件的值，显示在中间
	ui.contrastSlider->setSingleStep(1);//设置滑动的步长

	despill_factor = 1.0;
	ui.despillFactorSlider->setMinimum(0);
	ui.despillFactorSlider->setMaximum(10);
	ui.despillFactorSlider->setValue(10);//设置滑动条控件的值，显示在中间
	ui.despillFactorSlider->setSingleStep(1);//设置滑动的步长

	despill_balance = 0.5;
	ui.despillBalanceSlider->setMinimum(0);
	ui.despillBalanceSlider->setMaximum(10);
	ui.despillBalanceSlider->setValue(5);//设置滑动条控件的值，显示在中间
	ui.despillBalanceSlider->setSingleStep(1);//设置滑动的步长

	preBlurValue = 0;
	ui.PreBlurSlider->setMinimum(0);//设置滑动条控件的最小值
	ui.PreBlurSlider->setMaximum(20);//设置滑动条控件的最大值
	ui.PreBlurSlider->setValue(0);//设置滑动条控件的值，显示在中间
	ui.PreBlurSlider->setSingleStep(1);//设置滑动的步长

	edge_kernel_radius = 3;
	ui.edgeKernelRadiusSlider->setMinimum(0);
	ui.edgeKernelRadiusSlider->setMaximum(20);/*与原始范围不一致*/
	ui.edgeKernelRadiusSlider->setValue(3);//设置滑动条控件的值，显示在中间
	ui.edgeKernelRadiusSlider->setSingleStep(1);//设置滑动的步长

	edge_kernel_tolerance = 0.1;
	ui.edgeKernelToleranceSlider->setMinimum(0);
	ui.edgeKernelToleranceSlider->setMaximum(10);
	ui.edgeKernelToleranceSlider->setValue(1);
	ui.edgeKernelToleranceSlider->setSingleStep(1);//设置滑动的步长

	//26修改
	clip_black = 0.2;//0
	ui.clipBlackSlider->setMinimum(0);
	ui.clipBlackSlider->setMaximum(100);
	ui.clipBlackSlider->setValue(20);//设置滑动条控件的值，显示在中间
	ui.clipBlackSlider->setSingleStep(1);//设置滑动的步长
	//26修改
	clip_white = 0.9;//0
	ui.clipWhiteSlider->setMinimum(0);
	ui.clipWhiteSlider->setMaximum(100);
	ui.clipWhiteSlider->setValue(90);//设置滑动条控件的值，显示在中间
	ui.clipWhiteSlider->setSingleStep(1);//设置滑动的步长

	blur_post = 0;
	ui.PostBlurSlider->setMinimum(0);
	ui.PostBlurSlider->setMaximum(20);
	ui.PostBlurSlider->setValue(0);//设置滑动条控件的值，显示在中间
	ui.PostBlurSlider->setSingleStep(1);//设置滑动的步长

	distance = 0;
	ui.DilateErodeSlider->setMinimum(-20);
	ui.DilateErodeSlider->setMaximum(20);
	ui.DilateErodeSlider->setValue(0);//设置滑动条控件的值，显示在中间
	ui.DilateErodeSlider->setSingleStep(1);

	feather_distance = 0;
	ui.featherDistanceSlider->setMinimum(-20);
	ui.featherDistanceSlider->setMaximum(20);
	ui.featherDistanceSlider->setValue(0);//设置滑动条控件的值，显示在中间
	ui.featherDistanceSlider->setSingleStep(1);

	isGarbageMatte = false;
	ui.GarbageMatteCheckBox->setChecked(false);/*设置初选项*/
	isCoreMatte = false;
	ui.CoreMatteCheckBox->setChecked(false);

	isBoxMask_Garbage = true;
	//isEllipseMask = false;
	isBoxMask_Core = true;
	//isEllipseMask2 = false;

	maskType_Garbage = 0;
	maskType_Core = 0;
	/*垃圾遮罩所用*/
	x_Garbage = 0.5;
	y_Garbage = 0.5;
	width_Garbage = 0.1;
	height_Garbage = 0.1;
	rotation_Garbage = 0;
	transparent_Garbage = 1;
	ui.XDoubleSpinBox->setRange(-1, 2);  // 范围
	ui.XDoubleSpinBox->setDecimals(3);  // 精度
	ui.XDoubleSpinBox->setSingleStep(0.01); // 步长
	ui.XDoubleSpinBox->setValue(0.5);//设置初始值

	ui.YDoubleSpinBox->setRange(-1, 2);  // 范围
	ui.YDoubleSpinBox->setDecimals(3);  // 精度
	ui.YDoubleSpinBox->setSingleStep(0.01); // 步长
	ui.YDoubleSpinBox->setValue(0.5);

	ui.WidthDoubleSpinBox->setRange(0, 2);
	ui.WidthDoubleSpinBox->setDecimals(3);  // 精度
	ui.WidthDoubleSpinBox->setSingleStep(0.05); // 步长
	ui.WidthDoubleSpinBox->setValue(0.1);

	ui.HeightDoubleSpinBox->setRange(0, 2);
	ui.HeightDoubleSpinBox->setDecimals(3);  // 精度
	ui.HeightDoubleSpinBox->setSingleStep(0.05); // 步长
	ui.HeightDoubleSpinBox->setValue(0.1);

	ui.RotationDoubleSpinBox->setRange(0, 1800);
	ui.RotationDoubleSpinBox->setDecimals(1);  // 精度
	ui.RotationDoubleSpinBox->setSingleStep(0.1); // 步长
	ui.RotationDoubleSpinBox->setValue(0);
	//ui.RotationDoubleSpinBox->setSuffix("°");  // 后缀--乱码

	ui.TransparentDoubleSpinBox->setRange(0, 1);
	ui.TransparentDoubleSpinBox->setDecimals(3);  // 精度
	ui.TransparentDoubleSpinBox->setSingleStep(0.1); // 步长
	ui.TransparentDoubleSpinBox->setValue(1);

	/*核心遮罩所用*/
	x_Core = 0.5;
	y_Core = 0.5;
	width_Core = 0.1;
	height_Core = 0.1;
	rotation_Core = 0;
	transparent_Core = 1;
	ui.XDoubleSpinBox_2->setRange(-1, 2);  // 范围
	ui.XDoubleSpinBox_2->setDecimals(3);  // 精度
	ui.XDoubleSpinBox_2->setSingleStep(0.01); // 步长
	ui.XDoubleSpinBox_2->setValue(0.5);//设置初始值

	ui.YDoubleSpinBox_2->setRange(-1, 2);  // 范围
	ui.YDoubleSpinBox_2->setDecimals(3);  // 精度
	ui.YDoubleSpinBox_2->setSingleStep(0.01); // 步长
	ui.YDoubleSpinBox_2->setValue(0.5);

	ui.WidthDoubleSpinBox_2->setRange(0, 2);
	ui.WidthDoubleSpinBox_2->setDecimals(3);  // 精度
	ui.WidthDoubleSpinBox_2->setSingleStep(0.05); // 步长
	ui.WidthDoubleSpinBox_2->setValue(0.1);

	ui.HeightDoubleSpinBox_2->setRange(0, 2);
	ui.HeightDoubleSpinBox_2->setDecimals(3);  // 精度
	ui.HeightDoubleSpinBox_2->setSingleStep(0.05); // 步长
	ui.HeightDoubleSpinBox_2->setValue(0.1);

	ui.RotationDoubleSpinBox_2->setRange(0, 1800);
	ui.RotationDoubleSpinBox_2->setDecimals(1);  // 精度
	ui.RotationDoubleSpinBox_2->setSingleStep(0.1); // 步长
	ui.RotationDoubleSpinBox_2->setValue(0);
	//ui.RotationDoubleSpinBox_2->setSuffix("°");  // 后缀--乱码

	ui.TransparentDoubleSpinBox_2->setRange(0, 1);
	ui.TransparentDoubleSpinBox_2->setDecimals(3);  // 精度
	ui.TransparentDoubleSpinBox_2->setSingleStep(0.1); // 步长
	ui.TransparentDoubleSpinBox_2->setValue(1);
}
/*析构函数*/
hlsl1::~hlsl1() {
	if (directX11 != NULL) {
		directX11->~QtGuiClass();
		directX11 = NULL;
	}
}
//mousePressEvent()函数为鼠标按下事件响应函数--设置背景颜色值
void hlsl1::mousePressEvent(QMouseEvent *e)
{
	if (e->button() == Qt::LeftButton)
	{
		int x = QCursor::pos().x();/*从窗口系统中查询位置*/
		int y = QCursor::pos().y();
		//grabWidow:创建并返回通过抓取由QRect（x，y，width，height）限制的给定窗口的内容构造的像素图。
		QPixmap pixmap = QPixmap::grabWindow(QApplication::desktop()->winId(), x, y, 1, 1);/*抓取像素图,这个是Qt4中截取全屏的代码*/
																						   /*grabWindow（）函数从屏幕上抓取像素，而不是从窗口抓取像素，
																						   即如果有一个部分或完全超过您抓取的窗口的另一个窗口，您也可以从上面的窗口获取像素。
																						   通常不会抓取鼠标光标。*/
		if (!pixmap.isNull()) {/*如果像素图不为NULL*/
			QImage image = pixmap.toImage();//将像素图转换为QImage
			if (!image.isNull()) {//如果image不为空
				if (image.valid(0, 0)) {/*坐标位置有效*/
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



/*导入前景图像*/
void hlsl1::on_SelectImageButton_clicked() {
	file_name = QFileDialog::getOpenFileName(this, "Select Picture", ".", "*.jpg *.png");
	this->video = false;
	DisplayImage(file_name);
}
/*显示图像*///26修改
void hlsl1::DisplayImage(QString filename) {
	QImage * img = new QImage;
	if (filename.isEmpty()) {
		return;
	}
	/****************29修改开始****************/
	if (this->gamma > 0) {
		Mat mat = imread(filename.toStdString());
		int channels = mat.channels();/*获取当前图片的通道数*/
		if (channels == 3) {
			/*opencv读取的颜色值为BGR格式*/
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
			/*mat转换为QImage*/
			cvtColor(mat, mat, CV_BGR2RGB);//Qt中支持的是RGB图像, OpenCV中支持的是BGR
		}
		if (channels == 4) {
			/*opencv读取的颜色值为BGRA格式*/
			MatIterator_<Vec4b> it, end;
			for (it = mat.begin<Vec4b>(), end = mat.end<Vec4b>(); it != end; it++)
			{
				float tmp = (*it)[0] / 255.0;
				(*it)[0] = (tmp > 0.0f ? pow(tmp, gamma) : tmp)*255.0;
				tmp = (*it)[1] / 255.0;
				(*it)[1] = (tmp > 0.0f ? pow(tmp, gamma) : tmp)*255.0;
				tmp = (*it)[2] / 255.0;
				(*it)[2] = (tmp > 0.0f ? pow(tmp, gamma) : tmp)*255.0;
				/*(*it)[3]即alpha通道不变*/
			}
			/*mat转换为QImage*/
			cvtColor(mat, mat, CV_BGRA2RGB);//Qt中支持的是RGB图像, OpenCV中支持的是BGR
		}
		QImage image((const uchar *)mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888); //temp.setp()没有时，会导致有些图片在转换后倾斜 
		image.bits(); // enforce deep copy, see documentation  
					  // of QImage::QImage ( const uchar * data, int width, int height, Format format )
					  //QImage image = QImage((uchar*)(mat.data), mat.cols, mat.rows, QImage::Format_RGB888);
		QImage * img = &image;
		/****************29修改结束****************/

		/*图像缩放--增加图片质量的缩放*/
		QImage* imgScaled = new QImage;
		*imgScaled = img->scaled(ui.ImageDisplayLabel->width(), ui.ImageDisplayLabel->height(),
			Qt::IgnoreAspectRatio, Qt::SmoothTransformation);/*保持长宽比例*/
		ui.ImageDisplayLabel->setPixmap(QPixmap::fromImage(*imgScaled)); // 将图片显示到label上
		mat.release();
	}
	else {
		if (!(img->load(filename))) {
			QMessageBox::information(this, tr("打开图像失败"), tr("打开图像失败!"));
			delete img;
			return;
		}
		/*图像缩放--增加图片质量的缩放*/
		QImage* imgScaled = new QImage;
		*imgScaled = img->scaled(ui.ImageDisplayLabel->width(), ui.ImageDisplayLabel->height(),
			Qt::IgnoreAspectRatio, Qt::SmoothTransformation);/*保持长宽比例*/
		ui.ImageDisplayLabel->setPixmap(QPixmap::fromImage(*imgScaled)); // 将图片显示到label上
	}
}
///*显示图像*/
//void hlsl1::DisplayImage(QString filename) {
//	QImage * img = new QImage;
//	if (filename.isEmpty()) {
//		return;
//	}
//	else {
//		if (!(img->load(filename))) {
//			QMessageBox::information(this, tr("打开图像失败"), tr("打开图像失败!"));
//			delete img;
//			return;
//		}
//	}
//	/*图像缩放--增加图片质量的缩放*/
//	QImage* imgScaled = new QImage;
//	*imgScaled = img->scaled(ui.ImageDisplayLabel->width(), ui.ImageDisplayLabel->height(),
//		Qt::IgnoreAspectRatio, Qt::SmoothTransformation);/*保持长宽比例*/
//	ui.ImageDisplayLabel->setPixmap(QPixmap::fromImage(*imgScaled)); // 将图片显示到label上
//}

/*导入前景视频*/
void hlsl1::on_SelectVideoButton_clicked() {
	/*视频格式有哪些??这里可以再多加一些*/
	file_name = QFileDialog::getOpenFileName(this, "Select video", ".", "*.MOV");
	video = true;
	DisplayVideo(file_name);/*将视频显示在前端界面*/
}
/*显示原始视频第一帧*/
void hlsl1::DisplayVideo(QString filename) {
	/*输入的是视频*/
	cap.open(filename.toStdString()); //打开视频
	if (!cap.isOpened())//如果视频不能正常打开则返回
		return;
	Mat src;
	cap >> src;//等价于cap.read(frame);将视频中的帧放入mat中
	if (src.empty())//如果某帧为空则退出循环
		return;
	cvtColor(src, src, CV_BGR2RGB);//Qt中支持的是RGB图像, OpenCV中支持的是BGR
	QImage image = QImage((uchar*)(src.data), src.cols, src.rows, QImage::Format_RGB888);
	QImage * img = &image;
	/*图像缩放--增加图片质量的缩放*/
	QImage* imgScaled = new QImage;
	*imgScaled = img->scaled(ui.ImageDisplayLabel->width(), ui.ImageDisplayLabel->height(),
		Qt::IgnoreAspectRatio, Qt::SmoothTransformation);/*保持长宽比例*/
	ui.ImageDisplayLabel->setPixmap(QPixmap::fromImage(*imgScaled));
	src.release();/*释放src*/
}
/*导入背景图像*/
void hlsl1::on_SelectBackgroundButton_clicked() {
	filename_bg = QFileDialog::getOpenFileName(this, "SelectBackgroundPicture", ".", "*.jpg");
	if (video == false) {
		DisplayImage(filename_bg);
	}
}
/*合成按钮*/
void hlsl1::on_CompositeRadioButton_clicked() {
	if (composite == false) {
		composite = true;
	}
	else {
		composite = false;
		//DisplayImage(file_name);
	}
}
/*设置屏幕平衡值*/
void hlsl1::on_ScreenBalanceSlider_valueChanged(int value) {
	ScreenBalanceValue = value / 100.0;
	ui.ScreenBalanceValueLabel->setText(QString::number(ScreenBalanceValue));
}
/*设置非溢出系数*/
void hlsl1::on_despillFactorSlider_valueChanged(int value) {
	despill_factor = value / 10.0;
	ui.despillFactorValueLabel->setText(QString::number(despill_factor));
}
/*设置非溢出平衡*/
void hlsl1::on_despillBalanceSlider_valueChanged(int value) {
	despill_balance = value / 10.0;
	ui.despillBalanceValueLabel->setText(QString::number(despill_balance));
}
/*设置预模糊值*/
void hlsl1::on_PreBlurSlider_valueChanged(int value) {
	preBlurValue = value;
	ui.PreBlurValueLabel->setText(QString::number(preBlurValue));
}
/*设置gamma值*/
void hlsl1::on_gammaSlider_valueChanged(int value) {
	gamma = value / 10.0;
	ui.gammaValueLabel->setText(QString::number(gamma));
	if (video == false) {
		DisplayImage(file_name);/*重新显示图像*/
	}
	else {
		DisplayVideo(file_name);/*重新显示视频第一帧*/
	}
}
/*设置brightness值*///26添加
void hlsl1::on_brightnessSlider_valueChanged(int value) {
	brightness = value / 10.0;
	ui.brightnessValueLabel->setText(QString::number(brightness));
}

/****************28修改开始****************/
/*设置二值化*/
void hlsl1::on_binarizationSlider_valueChanged(int value) {
	binarization_threshold = value / 10.0;
	ui.binarizationValueLabel->setText(QString::number(binarization_threshold));
}
/*是否进行二值化按钮*/
void hlsl1::on_binaryCheckBox_clicked() {/*是否进行二值化按钮*/
	if (!ui.binaryCheckBox->checkState()) {
		isBinary = false;
		ui.binaryCheckBox->setChecked(false);
	}
	else {
		isBinary = true;
		ui.binaryCheckBox->setChecked(true);
	}
}
/****************28修改结束****************/

/*设置sharp值*///26添加
void hlsl1::on_sharpSlider_valueChanged(int value) {
	sharp = value / 10.0;
	ui.sharpValueLabel->setText(QString::number(sharp));
}
/*设置contrast值*///26添加
void hlsl1::on_contrastSlider_valueChanged(int value) {
	contrast = value / 10.0;
	ui.contrastValueLabel->setText(QString::number(contrast));
}
/*设置边缘核心半径*/
void hlsl1::on_edgeKernelRadiusSlider_valueChanged(int value) {
	edge_kernel_radius = value;
	ui.edgeKernelRadiusValueLabel->setText(QString::number(edge_kernel_radius));
}
/*设置边缘核心容差*/
void hlsl1::on_edgeKernelToleranceSlider_valueChanged(int value) {
	edge_kernel_tolerance = value / 10.0;
	ui.edgeKernelToleranceValueLabel->setText(QString::number(edge_kernel_tolerance));
}
/*设置钳制黑色*///26修改
void hlsl1::on_clipBlackSlider_valueChanged(int value) {
	clip_black = value / 100.0;
	ui.clipBlackValueLabel->setText(QString::number(clip_black));
}
/*设置钳制白色*///26修改
void hlsl1::on_clipWhiteSlider_valueChanged(int value) {
	clip_white = value / 100.0;
	ui.clipWhiteValueLabel->setText(QString::number(clip_white));
}
/*设置后期模糊*/
void hlsl1::on_PostBlurSlider_valueChanged(int value) {
	blur_post = value;
	ui.PostBlurValueLabel->setText(QString::number(blur_post));
}
/*设置膨胀腐蚀*/
void hlsl1::on_DilateErodeSlider_valueChanged(int value) {
	distance = value;
	ui.DilateErodeValueLabel->setText(QString::number(distance));
}
/*设置羽化距离*/
void hlsl1::on_featherDistanceSlider_valueChanged(int value) {
	feather_distance = value;
	ui.featherDistanceValueLabel->setText(QString::number(feather_distance));
}
/*设置羽化衰减*/
void hlsl1::on_featherFalloffComboBox_currentIndexChanged(int index)
{
	//将当前索引赋值给变量index，输出当前选项名
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
/*设置垃圾遮罩*/
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
//垃圾遮罩-若当前对象MaskComboBox值发生改变则触发此函数
void hlsl1::on_MaskComboBox_currentIndexChanged(int index)
{
	//将当前索引赋值给变量index，输出当前选项名
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
//垃圾遮罩的遮罩类型-若当前对象MaskTypeComboBox值发生改变则触发此函数
void hlsl1::on_MaskTypeComboBox_currentIndexChanged(int index)
{
	//将当前索引赋值给变量index，输出当前选项名
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
/*垃圾遮罩*/
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
/*设置核心遮罩*/
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
/*核心遮罩*/
void hlsl1::on_MaskComboBox_2_currentIndexChanged(int index)
{
	//将当前索引赋值给变量index，输出当前选项名
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
/*核心遮罩的遮罩类型-若当前对象MaskTypeComboBox值发生改变则触发此函数*/
void hlsl1::on_MaskTypeComboBox_2_currentIndexChanged(int index)
{
	//将当前索引赋值给变量index，输出当前选项名
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
/*核心遮罩*/
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
/*设置directX程序所需的参数*/
void hlsl1::setParameter(QtGuiClass * dX) {
	/*需要传入屏幕平衡值+屏幕背景颜色+主通道*/
	dX->ScreenBalanceValue = this->ScreenBalanceValue;//0.5
	dX->ScreenColorValue[0] = this->screen_color[0];//0.0
	dX->ScreenColorValue[1] = this->screen_color[1];//0.8
	dX->ScreenColorValue[2] = this->screen_color[2];//0.0
	dX->ScreenColorValue[3] = this->screen_color[3];//1.0
	/*去除绿色溢出所需数据*/
	dX->DespillFactor = this->despill_factor;//1.0
	dX->DespillBalance = this->despill_balance;//0.5
	/*图像预模糊处理所需数据*/
	dX->preBlurValue = this->preBlurValue;//0
	/*gamma校正*///26添加
	dX->gamma = this->gamma;
	/*亮度和对比度增强*///26添加
	dX->brightness = this->brightness;
	dX->contrast = this->contrast;
	dX->sharp_value = this->sharp;

	/****************28修改开始****************/
	/*二值化参数*/
	dX->binarization_threshold = this->binarization_threshold;
	dX->isBinary = this->isBinary;
	/****************28修改结束****************/

	/*钳制黑色白色所需数据*/
	dX->clipBlack = this->clip_black;
	dX->clipWhite = this->clip_white;
	dX->kernelRadius = this->edge_kernel_radius;
	dX->kernelTolerance = this->edge_kernel_tolerance;
	/*后期模糊所需数据*/
	dX->blur_post = this->blur_post;
	/*膨胀腐蚀所需数据*/
	dX->distance = this->distance;
	/*羽化所需参数*/
	dX->feather_distance = this->feather_distance;
	dX->feather_falloff = this->feather_falloff;
	/*垃圾遮罩所需参数*/
	dX->isGarbageMatte = this->isGarbageMatte;
	dX->isBoxMask_Garbage = this->isBoxMask_Garbage;
	dX->x_Garbage = this->x_Garbage;
	dX->y_Garbage = this->y_Garbage;
	dX->width_Garbage = this->width_Garbage;
	dX->height_Garbage = this->height_Garbage;
	dX->rotation_Garbage = this->rotation_Garbage;
	dX->transparent_Garbage = this->transparent_Garbage;
	dX->maskType_Garbage = this->maskType_Garbage;
	/*核心遮罩所需参数*/
	dX->isCoreMatte = this->isCoreMatte;
	dX->isBoxMask_Core = this->isBoxMask_Core;
	dX->x_Core = this->x_Core;
	dX->y_Core = this->y_Core;
	dX->width_Core = this->width_Core;
	dX->height_Core = this->height_Core;
	dX->rotation_Core = this->rotation_Core;
	dX->transparent_Core = this->transparent_Core;
	dX->maskType_Core = this->maskType_Core;
	/*合成*/
	/*dX->filename_bg = this->filename_bg.toStdString();
	dX->composite = this->composite;*/
	dX->setComposite(this->composite, this->filename_bg.toStdString());
}

/*点击Kinect获取一帧按钮*/
void hlsl1::on_getOneFrameButton_clicked() {
	/////////////////////////////////////////////////
	///////         从Kinect中获取一帧显示在ui.ImageDisplayLabel上
	///////      注意，开始运行程序后，得过个几秒钟，等Kinect接通后再点此按钮
	/////////////////////////////////////////////////

	unsigned char * colorFlow = (unsigned char*)malloc(sizeof(unsigned char) * colorwidth * colorheight * 3);
	unsigned short * depthFlow = (unsigned short *)malloc(sizeof(unsigned short)* colorwidth*colorheight);
	//cv::waitKey(30);
	getKinectData(colorFlow, depthFlow);
	Mat src = cv::Mat(colorheight, colorwidth, CV_8UC3, colorFlow);//彩色图赋给src，用于前端渲染

	/****************27修改开始****************/
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
	/*mat转换为QImage*/
	cvtColor(src, src, CV_BGR2RGB);//Qt中支持的是RGB图像, OpenCV中支持的是BGR
	QImage image((const uchar *)src.data, src.cols, src.rows, src.step, QImage::Format_RGB888); //temp.setp()没有时，会导致有些图片在转换后倾斜 
	image.bits(); // enforce deep copy, see documentation  
	/****************27修改结束****************/


	//cvtColor(src, src, CV_BGR2RGB);//Qt中支持的是RGB图像, OpenCV中支持的是BGR
	//QImage image = QImage((uchar*)(src.data), src.cols, src.rows, QImage::Format_RGB888);
	QImage * img = &image;
	/*图像缩放--增加图片质量的缩放*/
	QImage* imgScaled = new QImage;
	*imgScaled = img->scaled(ui.ImageDisplayLabel->width(), ui.ImageDisplayLabel->height(),
		Qt::IgnoreAspectRatio, Qt::SmoothTransformation);/*保持长宽比例*/
	ui.ImageDisplayLabel->setPixmap(QPixmap::fromImage(*imgScaled));
	src.release();/*释放src*/

}
/*点击Kinect按钮*/
void hlsl1::on_SelectKinectButton_clicked() {
	/*清除上一次操作的所有数据*/
	if (directX11 != NULL) {
		directX11->~QtGuiClass();
		directX11 = NULL;
	}
	this->video = false;
	this->ifKinect = true;

	directX11 = new QtGuiClass(false, true);
	//directX11->flag = 0;/*输出蒙版*/
	directX11->flag = 1;/*输出前景*/
	this->setParameter(directX11);
	directX11->setConstantBuffers();/*设置常量缓冲区*/
	directX11->show();/*只是显示*/
	
}

/*点击“蒙版按钮”，输出蒙版--不带通道*/
void hlsl1::on_KeyingMatteButton_clicked() {
	/*清除上一次操作的所有数据*/
	if (directX11 != NULL) {
		directX11->~QtGuiClass();
		directX11 = NULL;
	}
	//this->video = false;
	/*对单张图片进行处理*/
	if (file_name != NULL&&file_name != ""&&this->video == false) {/*对单张图片进行处理*/
		string fileName = this->file_name.toStdString();/*QString和String的转换*/
		qDebug() << "--------new start" << endl;
		directX11 = new QtGuiClass(fileName,false);
		qDebug() << "--------new end" << endl;
		directX11->flag = 0;/*输出蒙版*/
		this->setParameter(directX11);
		directX11->setConstantBuffers();
		qDebug() << "--------show() start" << endl;
		directX11->show();/*只是显示*/
		qDebug() << "--------show() end" << endl;

		//resultImage = QImage(directX11->imgData, directX11->width, directX11->height, QImage::Format_RGBA8888);
		//resultImage = resultImage.mirrored(false, true);/*将图片垂直翻转*/
		//								
		///*图像缩放--增加图片质量的缩放*/
		//QImage* imgScaled = new QImage;
		//*imgScaled = resultImage.scaled(ui.ResultDisplayLabel->width(), ui.ResultDisplayLabel->height(),
		//	Qt::IgnoreAspectRatio, Qt::SmoothTransformation);/*保持长宽比例*/
		//ui.ResultDisplayLabel_2->setPixmap(QPixmap::fromImage(*imgScaled)); // 将图片显示到label上

		//free(directX11->imgData);
	}
	/*对视频进行处理*/
	if (file_name != NULL&&file_name != ""&&this->video==true) {
		string fileName = this->file_name.toStdString();/*QString和String的转换*/
		directX11 = new QtGuiClass(fileName, true);
		directX11->flag = 0;/*输出蒙版*/
		this->setParameter(directX11);
		directX11->setConstantBuffers();/*设置常量缓冲区*/
		directX11->show();/*只是显示*/
	}
}
/*输出前景图--带通道*/
void hlsl1::on_KeyingImageButton_clicked() {
	/*清除上一次操作的所有数据*/
	if (directX11 != NULL) {
		directX11->~QtGuiClass();
		directX11 = NULL;
	}
	/*对单张图片进行处理*/
	if (file_name != NULL&&file_name != ""&&this->video == false) {/*对单张图片进行处理*/
		string fileName = this->file_name.toStdString();/*QString和String的转换*/
		directX11 = new QtGuiClass(fileName,false);
		directX11->flag = 1;/*输出前景*/
		this->setParameter(directX11);
		directX11->setConstantBuffers();
		directX11->show();

		//resultImage = QImage(directX11->imgData, directX11->width, directX11->height, QImage::Format_RGBA8888);
		//resultImage = resultImage.mirrored(false, true);/*将图片垂直翻转*/

		///*图像缩放--增加图片质量的缩放*/
		//QImage* imgScaled = new QImage;
		//*imgScaled = resultImage.scaled(ui.ResultDisplayLabel->width(), ui.ResultDisplayLabel->height(),
		//	Qt::IgnoreAspectRatio, Qt::SmoothTransformation);/*保持长宽比例*/
		//ui.ResultDisplayLabel_2->setPixmap(QPixmap::fromImage(*imgScaled)); // 将图片显示到label上

		//free(directX11->imgData);
	}
	/*对视频进行处理*/
	if (file_name != NULL&&file_name != ""&&this->video == true) {
		string fileName = this->file_name.toStdString();/*QString和String的转换*/
		directX11 = new QtGuiClass(fileName, true);
		directX11->flag = 1;/*输出蒙版*/
		this->setParameter(directX11);
		directX11->setConstantBuffers();
		directX11->show();/*只是显示*/
	}
}
