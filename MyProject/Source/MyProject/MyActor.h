// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "TCPServerActor_depth.h"
#include "TCPServerActor_rgb.h"

#include <ctime>
#include <iostream>
#include <fstream>
using namespace std;

//#include <Kinect.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <Eigen/Geometry>

THIRD_PARTY_INCLUDES_START
#define BOOST_DISABLE_ABI_HEADERS
#include <boost/format.hpp> // for formating strings
THIRD_PARTY_INCLUDES_END

namespace boost
{
#ifdef BOOST_NO_EXCEPTIONS

	void throw_exception(std::exception const &e)
	{
	} // user defined

#else

#endif

} // namespace boost、

#include "LidarPointCloud.h"
#include "LidarPointCloudShared.h"
#include "LidarPointCloudActor.h"
#include "IO/LidarPointCloudFileIO_ASCII.h"

#include "CoreMinimal.h"
#include "Kismet/GameplayStatics.h"

#include "IGPUPointCloudRenderer.h"
#include "PointCloudStreamingCore.h"
#include "GPUPointCloudRendererComponent.h"

#include "GameFramework/Actor.h"
#include "MyActor.generated.h"

UCLASS()
class MYPROJECT_API AMyActor : public AActor
{
	GENERATED_BODY()

public:
	//添加一个根组件，不然没法移动
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USceneComponent *SceneComponent;
	//点云组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UGPUPointCloudRendererComponent *PointCloudRendererComponent;

	///////////////////////////
	//       Kinect
	///////////////////////////

	//const int width = 512;
	//const int height = 424;
	//const int colorwidth = 1920;
	//const int colorheight = 1080;

	//void drawKinectData();

	///////////////////////////
	//       ue4
	///////////////////////////
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	int32 TotalDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float DamageTimeInSeconds;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Damage")
	float DamagePerSecond;

	// Sets default values for this actor's properties
	AMyActor();

	void PostInitProperties();
	UFUNCTION(BlueprintCallable, Category = "Damage")
	void CalculateValues();
	void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent);
	UFUNCTION(BlueprintNativeEvent, Category = "Damage")
	void CalledFromCpp();

	//初始化相机外参
	vector<Eigen::Isometry3d, Eigen::aligned_allocator<Eigen::Isometry3d>> poses; // 相机位姿
	void InitPose();

	//用Kinect采集的数据生成点云
	void GenerateCloud();

	// Create an empty array to hold the points
	TArray<FLidarPointCloudPoint> MyPoints;
	ULidarPointCloud *MyPointCloud;
	ULidarPointCloud *MyPointCloud_fromFile;
	ULidarPointCloud *MyPointCloud_fromKinect;
	ALidarPointCloudActor *MyPointCloudActor;
	ALidarPointCloudActor *MyPointCloudActor_fromKinect;

	clock_t time_stt; // 计时

	//来自kinect的数据用到的
	typedef FLidarPointCloudPoint PointT;
	typedef TArray<FLidarPointCloudPoint> PointCloud;
	PointCloud pointCloud;

	//渲染插件用的成员变量 gaoshiyu修改
	/*UMaterialInstanceDynamic *mymaterialpoint = nullptr;
	FPointCloudStreamingCore *myPointCloudRendering = new FPointCloudStreamingCore(mymaterialpoint);
	TArray<FVector> gao_points_xyz;
	TArray<FColor> gao_points_coulor;*/
	//static UMaterialInstanceDynamic* my_materialInstanceDynamicPoint;
	struct PlayerController_eventClimpClimp_Parms
	{
		TArray<FVector> points_xyz;
		TArray<FColor> points_coulor;
	};

	///////////////////////////
	//      socket
	///////////////////////////
	ATCPServerActor_depth *TCP_depth;
	ATCPServerActor_rgb *TCP_rgb;
	//cv::Mat receivedImg_depth = cv::Mat(1080, 1920, CV_16UC1, cv::Scalar(600));
	//cv::Mat receivedImg_rgb = cv::Mat(1080, 1920, CV_8UC3, cv::Scalar(0, 255, 0));
	cv::Mat receivedImg_depth;
	cv::Mat receivedImg_rgb;
	//cv::Mat receivedImg_rgba = cv::Mat(1080, 1920, CV_8UC4, cv::Scalar(0, 255, 0, 255));

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
