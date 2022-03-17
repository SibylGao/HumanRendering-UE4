#include "MyActor.h"

//#define RGBorRGBA 1     //0表示接收rgb， 1表示接收RGBA
#define SIZE_RGB 1080 * 1920 * 3 //1080 * 1920 * 3 or 1080 * 1920 * 4 分别表示rgb和rgba
#define SIZE_RGBA 1080 * 1920 * 4

int trim = 300; //左右两侧各裁剪掉400个像素
int ccount = 20;
// Sets default values
AMyActor::AMyActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TotalDamage = 200;
	DamageTimeInSeconds = 1.0f;

	//创建组件实例
	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	PointCloudRendererComponent = CreateDefaultSubobject<UGPUPointCloudRendererComponent>("PointCloudRendererComponent");
	//附加到根组件
	PointCloudRendererComponent->SetupAttachment(SceneComponent);
}
void AMyActor::InitPose()
{
	//相机外参
	double data[7] = {0};
	data[6] = 1;
	Eigen::Quaterniond q(data[6], data[3], data[4], data[5]);
	Eigen::Isometry3d T(q);
	T.pretranslate(Eigen::Vector3d(data[0], data[1], data[2]));
	poses.push_back(T);
}

//生成点云
void AMyActor::GenerateCloud()
{
	pointCloud.Empty(); //一定要清空。

	vector<cv::Mat> colorImgs, depthImgs; // 彩色图和深度图

	colorImgs.clear();
	depthImgs.clear();
	for (int i = 0; i < 1; i++)
	{
		colorImgs.push_back(receivedImg_rgb);
		depthImgs.push_back(receivedImg_depth); // 使用-1读取原始图像
	}
	//cv::imshow("wwe", receivedImg_rgb);
	//cv::waitKey(30);

	double depthScale = 10.0;

	double cx = 973.315007239157;
	double cy = 516.520757482246;
	double fx = 1049.72332533394;
	double fy = 1047.55943288646;

	UE_LOG(LogTemp, Log, TEXT("正在将图像转换为点云..."));

	//用来存放颜色和位置的数组
	TArray<FVector> point_positions;
	TArray<FColor> point_colors;

	//////////////////////////////////////////////////////////////////这里耗时
	// 新建一个点云
	for (int i = 0; i < colorImgs.size(); i++)
	{
		cout << "转换图像中: " << i + 1 << endl;
		cv::Mat color = receivedImg_rgb;
		cv::Mat depth = receivedImg_depth;
		Eigen::Isometry3d T = poses[i];
		//for (int v = 0; v < color.rows; v++)
		//	for (int u = 0; u < color.cols; u++)
		//我猜想如果每帧实时渲染1920*1080个点，也就是200万个点，可能难免卡顿，于是缩小到50万。速度有明显提升，但是依然会出现频闪的情况。
		//不用担心点云数量，因为将来只渲染人，点云量很少。
		//频闪情况可能来源于：1、每帧的渲染时间未调好（尝试保持上一帧，解决） （已修改） 2、depth与rgb传输未同步（孔洞？）
		//	for (int v = 0; v < color.rows; v+=2)
		//		for (int u = 0; u < color.cols; u+=2)
		//		{
		//			unsigned int d = depth.ptr<unsigned short>(v)[u]; // 深度值
		//			if (d == 0) continue; // 为0表示没有测量到
		//			if (color.at<cv::Vec3b>(v, u)[0] == 0 && color.at<cv::Vec3b>(v, u)[1] == 0 && color.at<cv::Vec3b>(v, u)[2] == 0)
		//				continue;

		//			Eigen::Vector3d point;
		//			point[2] = double(d) / depthScale;
		//			point[0] = (u - cx) * point[2] / fx;
		//			point[1] = (v - cy) * point[2] / fy;

		//			//ue4坐标系是左手坐标系。Kinect相机坐标系是右手系。
		//			point[1] *= -1;
		//			Eigen::Vector3d pointWorld = T * point;

		//			//注意坐标系的差异。
		//			FVector point_xyz = FVector(pointWorld[2], pointWorld[0], pointWorld[1]);
		//			FColor point_color = FColor(color.data[v * color.step + u * color.channels() + 0], color.data[v * color.step + u * color.channels() + 1], color.data[v * color.step + u * color.channels() + 2]);

		//			PointT p(point_xyz, point_color, true, 1);
		//			pointCloud.Emplace(p);
		//		}
		//}

		//根据RGBD
		for (int v = 0; v < color.rows; v += 2)
			for (int u = 0 + trim; u < color.cols - trim; u += 2)
			{
				unsigned int d = depth.ptr<unsigned short>(v)[u]; // 深度值
				if (d == 0)
					continue;						   // 为0表示没有测量到
				if (color.at<cv::Vec4b>(v, u)[3] < 10) //如果alpha小于某阈值，则认为是背景，将其过滤掉。注意这里得改成Vec4b哦
					continue;

				Eigen::Vector3d point;
				point[2] = double(d) / depthScale;
				point[0] = (u - cx) * point[2] / fx;
				point[1] = (v - cy) * point[2] / fy;

				//ue4坐标系是左手坐标系。Kinect相机坐标系是右手系。
				point[1] *= -1;
				Eigen::Vector3d pointWorld = T * point;

				//注意坐标系的差异。
				FVector point_xyz = FVector(pointWorld[2], pointWorld[0], pointWorld[1]);
				FColor point_color = FColor(color.data[v * color.step + u * color.channels() + 0], color.data[v * color.step + u * color.channels() + 1], color.data[v * color.step + u * color.channels() + 2]);

				// //gaoshiyu added
				// gao_points_coulor.Emplace(point_color);
				// gao_points_xyz.Emplace(point_xyz);
				// bool gao_flag = myPointCloudRendering->SetInput(gao_points_xyz, gao_points_coulor);

				//加入数组
				point_colors.Add(point_color);
				point_positions.Add(point_xyz);

				/*PointT p(point_xyz, point_color, true, 1);
				pointCloud.Emplace(p);*/
			}

		//TODO:设置属性，具体参数看插件说明
		//PointCloudRendererComponent->SetDynamicProperties();

		//插件输入点的位置和颜色
		PointCloudRendererComponent->SetInputAndConvert2(point_positions, point_colors);
	}

	// Prepare async parameteres
	//FLidarPointCloudAsyncParameters AsyncParameters_Kinect(true,
	//	[](float Progress)
	//	{
	//		// Lambda call, executed every ~1% of progress
	//		// Progress parameter is a float in 0 - 1 range
	//	},
	//	[](bool bSuccess)
	//	{
	//		// Lambda call, executed, once the process completes
	//	});

	//////////////////////////////////////////////////////////////////这里耗时

	MyPointCloud_fromKinect->SetData(pointCloud);

	MyPointCloudActor_fromKinect->SetPointCloud(MyPointCloud_fromKinect);
}
//加载一个深度图和一个rgb
//void AMyActor :: GenerateCloud()
//{
//	vector<cv::Mat> colorImgs, depthImgs;    // 彩色图和深度图
//	vector<Eigen::Isometry3d, Eigen::aligned_allocator<Eigen::Isometry3d>> poses;         // 相机位姿
//
//	ifstream fin("C:\\Users\\Administrator\\Desktop\\slambook-master\\ch5\\joinMap\\mytestData\\pose.txt");
//	if (!fin)
//	{
//		//cerr << "请在有pose.txt的目录下运行此程序" << endl;
//		UE_LOG(LogTemp, Log, TEXT("can't find pose.txt"));
//		return ;
//	}
//	for (int i = 0; i < 1; i++)
//	{
//		//        boost::format fmt( "../../%s/%d.%s" ); //图像文件格式
//		boost::format fmt("C:\\Users\\Administrator\\Desktop\\slambook-master\\ch5\\joinMap\\mytestData/%s/%d.%s"); //图像文件格式
//		colorImgs.push_back(cv::imread((fmt % "color" % (i + 1) % "png").str()));
//		depthImgs.push_back(cv::imread((fmt % "depth" % (i + 1) % "pgm").str(), -1)); // 使用-1读取原始图像
//
//		//UE_LOG(LogTemp, Log, TEXT("##################################################################"));
//		//string log = (fmt % "color" % (i + 1) % "png").str();
//		//UE_LOG(LogTemp, Log, TEXT("%s"), *FString(log.c_str()));
//
//		double data[7] = { 0 };
//		for (auto& d : data)
//			fin >> d;
//		Eigen::Quaterniond q(data[6], data[3], data[4], data[5]);
//		Eigen::Isometry3d T(q);
//		T.pretranslate(Eigen::Vector3d(data[0], data[1], data[2]));
//		poses.push_back(T);
//	}
//	double depthScale = 10.0;
//
//	double cx = 973.315007239157;
//	double cy = 516.520757482246;
//	double fx = 1049.72332533394;
//	double fy = 1047.55943288646;
//
//	UE_LOG(LogTemp, Log, TEXT("正在将图像转换为点云..."));
//
//
//	// 新建一个点云
//	for (int i = 0; i < colorImgs.size(); i++)
//	{
//		cout << "转换图像中: " << i + 1 << endl;
//		cv::Mat color = colorImgs[i];
//		cv::Mat depth = depthImgs[i];
//		Eigen::Isometry3d T = poses[i];
//		for (int v = 0; v < color.rows; v++)
//			for (int u = 0; u < color.cols; u++)
//			{
//				unsigned int d = depth.ptr<unsigned short>(v)[u]; // 深度值
//				if (d == 0) continue; // 为0表示没有测量到
//				if (d == 0) continue; // 为0表示没有测量到
//				Eigen::Vector3d point;
//				point[2] = double(d) / depthScale;
//				point[0] = (u - cx) * point[2] / fx;
//				point[1] = (v - cy) * point[2] / fy;
//
//				//ue4坐标系是左手坐标系。Kinect相机坐标系是右手系。
//				point[1] *= -1;
//				Eigen::Vector3d pointWorld = T * point;
//
//				//注意坐标系的差异。
//				FVector point_xyz = FVector(pointWorld[2], pointWorld[0], pointWorld[1]);
//				FColor point_color = FColor(color.data[v * color.step + u * color.channels() + 2], color.data[v * color.step + u * color.channels() + 1], color.data[v * color.step + u * color.channels()]);
//
//				PointT p(point_xyz, point_color, true, 1);
//				pointCloud.Emplace(p);
//			}
//	}
//
//	// Prepare async parameteres
//	FLidarPointCloudAsyncParameters AsyncParameters_Kinect(true,
//		[](float Progress)
//		{
//			// Lambda call, executed every ~1% of progress
//			// Progress parameter is a float in 0 - 1 range
//		},
//		[](bool bSuccess)
//		{
//			// Lambda call, executed, once the process completes
//		});
//
//	// Usage 1: Construct a Point Cloud asset
//	MyPointCloud_fromKinect = ULidarPointCloud::CreateFromData(pointCloud, AsyncParameters_Kinect);
//	MyPointCloudActor_fromKinect = GetWorld()->SpawnActor<ALidarPointCloudActor>();
//	MyPointCloudActor_fromKinect->SetPointCloud(MyPointCloud_fromKinect);
//
//
//}
// Called when the game starts or when spawned
void AMyActor::BeginPlay()
{
	Super::BeginPlay();

	//Depth
	TArray<AActor *> Actors_depth;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), TEXT("tag_tcp_depth"), Actors_depth);
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Actors.Num:" + FString::FromInt(Actors.Num())));

	for (AActor *Actor : Actors_depth)
	{
		//ReceiveBufferGT = Actor->ReceiveBufferGT;
		TCP_depth = dynamic_cast<ATCPServerActor_depth *>(Actor);
	}

	//RGB
	TArray<AActor *> Actors_rgb;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), TEXT("tag_tcp_rgb"), Actors_rgb);

	//UMaterialInterface *DistortionDisplacementMapMaterialParent = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/mymaterialInstance"));
	//mymaterialpoint = UMaterialInstanceDynamic::Create(DistortionDisplacementMapMaterialParent, nullptr);
	//myPointCloudRendering->UpdateDynamicMaterialForStreaming(mymaterialpoint); //更新渲染的指针

	for (AActor *Actor : Actors_rgb)
	{
		//ReceiveBufferGT = Actor->ReceiveBufferGT;
		TCP_rgb = dynamic_cast<ATCPServerActor_rgb *>(Actor);
	}

	//新建点云actor
	//MyPointCloudActor_fromKinect = GetWorld()->SpawnActor<ALidarPointCloudActor>();
	//原来用的是动态生成的，后来为了调材质，直接先在场景中建好了“KinectPointCloud”，再修改
	TArray<AActor *> Actors_kinect;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), TEXT("kinect_"), Actors_kinect);
	for (AActor *Actor : Actors_kinect)
	{
		//ReceiveBufferGT = Actor->ReceiveBufferGT;
		MyPointCloudActor_fromKinect = dynamic_cast<ALidarPointCloudActor *>(Actor);
	}

	MyPointCloud_fromKinect = ULidarPointCloud::CreateFromData(pointCloud, false);

	//初始化相机外参
	InitPose();

	time_stt = clock(); // 计时

	//////////////////////////////////
	//         来自kinect的数据生成点云
	//////////////////////////////////
	//GenerateCloud();

	//
	////////////////////////////////////
	////         随机生成点云
	////////////////////////////////////
	//	// Populate the MyPoints array with 1000 randomly positioned, red points
	//	for (int32 i = 0; i < 1000; i++)
	//	{
	//		// See FPointCloudPoint for more constructor versions
	//		MyPoints.Emplace(FLidarPointCloudPoint(FMath::VRand() * 100, FColor::Red, true, 1));
	//	}
	//
	//	// Prepare async parameteres
	//	FLidarPointCloudAsyncParameters AsyncParameters(true,
	//		[](float Progress)
	//		{
	//			// Lambda call, executed every ~1% of progress
	//			// Progress parameter is a float in 0 - 1 range
	//		},
	//		[](bool bSuccess)
	//		{
	//			// Lambda call, executed, once the process completes
	//		});
	//
	//	// Usage 1: Construct a Point Cloud asset
	//	MyPointCloud = ULidarPointCloud::CreateFromData(MyPoints, AsyncParameters);
	//
	//
	//
	////////////////////////////////////
	////         从文件中加载点云
	////////////////////////////////////
	//
	//	// This will determine normalization and clipping range for color values
	//// If set to FVector2D::ZeroVector or (0, 0), the best match will be automatically
	//// determined based on the data contained within the file.
	//	FVector2D RGBRange = FVector2D::ZeroVector;
	//
	//	// This object determines, which columns to use as which data sources
	//	// Use 0-based indexing when specifying the column ID
	//	FLidarPointCloudImportSettings_ASCII_Columns SourceColumns;
	//	SourceColumns.LocationX = 0;	// Use first column in the file as X location
	//	SourceColumns.LocationY = 1;	// Use second column in the file as Y location
	//	SourceColumns.LocationZ = 2;	// Use third column in the file as Z location
	//	SourceColumns.Intensity = 6;	// Use seventh column in the file as Intensity
	//
	//	// Do not use Red, Green nor Blue channels
	//	// This will result in Cloud importing as Intensity data only
	//	SourceColumns.Red = -1;
	//	SourceColumns.Green = -1;
	//	SourceColumns.Blue = -1;
	//
	//	FLidarPointCloudAsyncParameters AsyncParameters_fromFile(true,
	//		[](float Progress)
	//		{
	//			// Lambda call, executed every ~1% of progress
	//			// Progress parameter is a float in 0 - 1 range
	//		},
	//		[](bool bSuccess)
	//		{
	//			// Lambda call, executed, once the process completes
	//		});
	//
	//	// Load the Cloud from ASCII file
	//	MyPointCloud_fromFile = ULidarPointCloudFileIO_ASCII::CreatePointCloudFromFile("C:\\Users\\Administrator\\Desktop\\part_1.pts", AsyncParameters_fromFile, RGBRange, SourceColumns);
	//
	//	//////////////////////////////////
	//	//         新加actor显示
	//	//////////////////////////////////
	//
	//	//// Create new Point Cloud Actor
	//	MyPointCloudActor = GetWorld()->SpawnActor<ALidarPointCloudActor>();
	//
	//	time_stt = clock(); // 计时
}

// Called every frame
void AMyActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//if ((clock() - time_stt) / (double)CLOCKS_PER_SEC > 2) {
	//	//显示零件
	//	MyPointCloudActor->SetPointCloud(MyPointCloud_fromFile);

	//	if ((clock() - time_stt) / (double)CLOCKS_PER_SEC > 4) {
	//		//显示球
	//		MyPointCloudActor->SetPointCloud(MyPointCloud);
	//		time_stt = clock(); // 计时
	//	}
	//}

	//原版从TCP中读取数据
	////depth
	//if (TCP_depth->ReceiveBufferGT.Num() == 1080 * 1920 * 2)
	//{
	//	receivedImg_depth = cv::Mat(1080, 1920, CV_16UC1, reinterpret_cast<char*>(TCP_depth->ReceiveBufferGT.GetData()));
	//	//cv::imshow("asdf", receivedImg);
	//	//cv::waitKey(30);
	//}

	//////RGB
	////if (TCP_rgb->ReceiveBufferGT.Num() == SIZE_RGB)
	////{
	////	receivedImg_rgb = cv::Mat(1080, 1920, CV_8UC3, reinterpret_cast<char*>(TCP_rgb->ReceiveBufferGT.GetData()));
	////	//cv::imshow("", receivedImg);
	////	//cv::waitKey(30);
	////}
	//
	////RGBA
	//if (TCP_rgb->ReceiveBufferGT.Num() == SIZE_RGBA)
	//{
	//	//原版从TCP中读取数据
	//	receivedImg_rgb = cv::Mat(1080, 1920, CV_8UC4, reinterpret_cast<char*>(TCP_rgb->ReceiveBufferGT.GetData()));

	//	/*receivedImg_rgb = cv::imread("C:\\Users\\DELL\\Desktop\\yjf\\MyProject\\depth_saved\\color.png");*/
	//	//cv::imshow("", receivedImg);
	//	//cv::waitKey(30);
	//}

	receivedImg_rgb = cv::imread("C:\\Users\\gaoshiyu\\Desktop\\yjf\\MyProject\\depths_saved\\color.png");
	receivedImg_depth = cv::imread("C:\\Users\\gaoshiyu\\Desktop\\yjf\\MyProject\\depths_saved\\depth.png");

	//////////////////////////////////
	//         来自kinect的数据生成点云
	//////////////////////////////////
	//该值稳定，传输就稳定
	GEngine->AddOnScreenDebugMessage(5, 5.f, FColor::Red, TEXT("rgbdSize:" + FString::FromInt(TCP_rgb->ReceiveBufferGT.Num())));

	//gao mask
	//if (TCP_depth->ReceiveBufferGT.Num() == 1080 * 1920 * 2 && TCP_rgb->ReceiveBufferGT.Num() == SIZE_RGBA)
	//{
	//	//time_stt = clock();
	//	//每5帧渲染一次。保持点云渲染的视觉连续性
	//	if (ccount % 20 == 0) {
	//		GenerateCloud();
	//		ccount = 1;
	//	}
	//}
	//读取固定图像
	if (receivedImg_rgb.data && receivedImg_depth.data)
	{
		if (ccount % 5 == 0)
		{
			GenerateCloud();
			ccount = 1;
		}
	}

	ccount++;

	//if (TCP_depth->ReceiveBufferGT.Num() == 1080 * 1920 * 2 && TCP_rgb->ReceiveBufferGT.Num() == SIZE_RGB)
	//{
	//	//time_stt = clock();
	//	//每5帧渲染一次。保持点云渲染的视觉连续性
	//	if (ccount % 3 == 0) {
	//		GenerateCloud();
	//		ccount = 0;
	//	}
	//	ccount++;
	//}
}

void AMyActor::PostInitProperties()
{
	Super::PostInitProperties();
	CalculateValues();
}

void AMyActor::CalculateValues()
{
	DamagePerSecond = TotalDamage / DamageTimeInSeconds;
}

//在编辑器中发生变化时
#if WITH_EDITOR
void AMyActor::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent)
{
	CalculateValues();

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void AMyActor::CalledFromCpp_Implementation() //在vs中该行会显示错误，但是这个命名就是没错的。还是会成功地生成解决方案。
{
	// 这里可以添加些有趣的代码
}