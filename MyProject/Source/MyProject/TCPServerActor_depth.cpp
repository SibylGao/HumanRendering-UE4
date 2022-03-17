// Fill out your copyright notice in the Description page of Project Settings.


#include "TCPServerActor_depth.h"

ATCPServerActor_depth::ATCPServerActor_depth()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bShouldAutoListen = true;
	bReceiveDataOnGameThread = true;
	//bWantsInitializeComponent = true;
	//bAutoActivate = true;
	ListenPort = 1235;
	ListenSocketName = TEXT("ue4-tcp-server");
	bDisconnectOnFailedEmit = true;
	bShouldPing = false;
	PingInterval = 10.0f;
	PingMessage = TEXT("<Ping>");

	//BufferMaxSize = 2 * 1024 * 1024;	//default roughly 2mb
	BufferMaxSize = 2 * 1080 * 1920;
}