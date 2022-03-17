// Fill out your copyright notice in the Description page of Project Settings.
#include "TCPServerActor.h"
#include "Async/Async.h"
#include "TCPWrapperUtility.h"
#include "SocketSubsystem.h"
#include "Kismet/KismetSystemLibrary.h"
// Sets default values
ATCPServerActor::ATCPServerActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bShouldAutoListen = true;
	bReceiveDataOnGameThread = true;
	//bWantsInitializeComponent = true;
	//bAutoActivate = true;
	ListenPort = 1234;
	ListenSocketName = TEXT("ue4-tcp-server");
	bDisconnectOnFailedEmit = true;
	bShouldPing = false;
	PingInterval = 10.0f;
	PingMessage = TEXT("<Ping>");

	//BufferMaxSize = 2 * 1024 * 1024;	//default roughly 2mb
	BufferMaxSize = 4 * 1080 * 1920;
}

// Called when the game starts or when spawned
void ATCPServerActor::BeginPlay()
{
	Super::BeginPlay();

	if (bShouldAutoListen)
	{
		StartListenServer(ListenPort);
	}

	PingData.Append((uint8*)TCHAR_TO_UTF8(*PingMessage), PingMessage.Len());//from ATCPServerActor::InitializeComponent()

}

void ATCPServerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopListenServer();

	Super::EndPlay(EndPlayReason);
}

// Called every frame
void ATCPServerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ATCPServerActor::StartListenServer(const int32 InListenPort)
{
	FIPv4Address Address;
	FIPv4Address::Parse(TEXT("127.0.0.1"), Address);

	//Create Socket
	FIPv4Endpoint Endpoint(Address, InListenPort);

	ListenSocket = FTcpSocketBuilder(*ListenSocketName)
		//.AsNonBlocking()
		.AsReusable()
		.BoundToEndpoint(Endpoint)
		.WithReceiveBufferSize(BufferMaxSize);

	ListenSocket->SetReceiveBufferSize(BufferMaxSize, BufferMaxSize);
	ListenSocket->SetSendBufferSize(BufferMaxSize, BufferMaxSize);

	ListenSocket->Listen(8);

	OnListenBegin.Broadcast();
	bShouldListen = true;

	//Start a lambda thread to handle data
	ServerFinishedFuture = FTCPWrapperUtility::RunLambdaOnBackGroundThread([&]()
		{
			uint32 BufferSize;
			//uint32 BufferSize = 0;
			TArray<uint8> ReceiveBuffer;
			TArray<TSharedPtr<FTCPClient>> ClientsDisconnected;

			FDateTime LastPing = FDateTime::Now();

			while (bShouldListen)
			{
				//Do we have clients trying to connect? connect them
				bool bHasPendingConnection;
				ListenSocket->HasPendingConnection(bHasPendingConnection);
				if (bHasPendingConnection)
				{
					//GEngine->AddOnScreenDebugMessage(-1, .001f, FColor::Red, TEXT("Hello"));

					TSharedPtr<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
					FSocket* Client = ListenSocket->Accept(*Addr, TEXT("tcp-client"));

					const FString AddressString = Addr->ToString(true);

					TSharedPtr<FTCPClient> ClientItem = MakeShareable(new FTCPClient());
					ClientItem->Address = AddressString;
					ClientItem->Socket = Client;

					Clients.Add(AddressString, ClientItem);	//todo: balance this with remove when clients disconnect

					AsyncTask(ENamedThreads::GameThread, [&, AddressString]()
						{
							OnClientConnected.Broadcast(AddressString);
						});
				}

				//Check each endpoint for data
				for (auto ClientPair : Clients)
				{
					TSharedPtr<FTCPClient> Client = ClientPair.Value;

					//Did we disconnect? Note that this almost never changed from connected due to engine bug, instead it will be caught when trying to send data
					ESocketConnectionState ConnectionState = Client->Socket->GetConnectionState();
					if (ConnectionState != ESocketConnectionState::SCS_Connected)
					{
						ClientsDisconnected.Add(Client);
						continue;
					}

					if (Client->Socket->HasPendingData(BufferSize))
					{
						//这里要把字悬停在屏幕上，因为not reveived 居多。不然屏幕上会一直显示not receive
						GEngine->AddOnScreenDebugMessage(1, .1f, FColor::Red, TEXT("recvd:" + FString::FromInt(BufferSize)));

						ReceiveBuffer.SetNumUninitialized(BufferSize);
						int32 Read = 0;

						Client->Socket->Recv(ReceiveBuffer.GetData(), ReceiveBuffer.Num(), Read);//设置一下buffersize
						GEngine->AddOnScreenDebugMessage(2, .1f, FColor::Red, TEXT("Read:" + FString::FromInt(Read)));

						//将接收到的数据包拼成图片
						gottenLen += ReceiveBuffer.Num();
						if (gottenLen < BufferMaxSize + 1)//如果加了之后小于3*1080*1920，那么就加
						{
							ReceiveBufferGT.Append(ReceiveBuffer);
						}
						else//如果加了之后超过3*1080*1920，那么就重新开
						{
							GEngine->AddOnScreenDebugMessage(3, .1f, FColor::Red, TEXT("last one:" + FString::FromInt(gottenLen - ReceiveBuffer.Num())));

							gottenLen = ReceiveBuffer.Num();
							ReceiveBufferGT = {};
							ReceiveBufferGT.Append(ReceiveBuffer);
						}

						//下面注释的是原来的单纯接收数据不处理
						//if (bReceiveDataOnGameThread)
						//{
						//	//Copy buffer so it's still valid on game thread
						//	TArray<uint8> ReceiveBufferGT;
						//	ReceiveBufferGT.Append(ReceiveBuffer);
						//	//GEngine->AddOnScreenDebugMessage(-1, .1f, FColor::Red, TEXT("ReceiveBufferGT.Num:" + FString::FromInt(ReceiveBufferGT.Num())));

						//	//Pass the reference to be used on gamethread
						//	AsyncTask(ENamedThreads::GameThread, [&, ReceiveBufferGT]()
						//		{
						//			OnReceivedBytes.Broadcast(ReceiveBufferGT);
						//		});
						//}
						//else
						//{
						//	OnReceivedBytes.Broadcast(ReceiveBuffer);
						//}
					}
					else
					{
						GEngine->AddOnScreenDebugMessage(0, .001f, FColor::Red, TEXT("not recv:") + FString::FromInt(BufferSize));
					}

					//ping check

					if (bShouldPing)
					{
						FDateTime Now = FDateTime::Now();
						float TimeSinceLastPing = (Now - LastPing).GetTotalSeconds();

						if (TimeSinceLastPing > PingInterval)
						{
							LastPing = Now;
							int32 BytesSent = 0;
							bool Sent = Client->Socket->Send(PingData.GetData(), PingData.Num(), BytesSent);
							//UE_LOG(LogTemp, Log, TEXT("ping."));
							if (!Sent)
							{
								//UE_LOG(LogTemp, Log, TEXT("did not send."));
								Client->Socket->Close();
							}
						}
					}
				}

				//Handle disconnections
				if (ClientsDisconnected.Num() > 0)
				{
					for (TSharedPtr<FTCPClient> ClientToRemove : ClientsDisconnected)
					{
						const FString Address = ClientToRemove->Address;
						Clients.Remove(Address);
						AsyncTask(ENamedThreads::GameThread, [this, Address]()
							{
								OnClientDisconnected.Broadcast(Address);
							});
					}
					ClientsDisconnected.Empty();
				}

				//sleep for 100microns
				FPlatformProcess::Sleep(0.0001);
			}//end while

			for (auto ClientPair : Clients)
			{
				ClientPair.Value->Socket->Close();
			}
			Clients.Empty();

			//Server ended
			AsyncTask(ENamedThreads::GameThread, [&]()
				{
					Clients.Empty();
					OnListenEnd.Broadcast();
				});
		});
}

void ATCPServerActor::StopListenServer()
{
	if (ListenSocket)
	{
		bShouldListen = false;
		ServerFinishedFuture.Get();

		ListenSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenSocket);
		ListenSocket = nullptr;

		OnListenEnd.Broadcast();
	}
}

bool  ATCPServerActor::Emit(const TArray<uint8>& Bytes, const FString& ToClient)
{
	if (Clients.Num() > 0)
	{
		int32 BytesSent = 0;
		//simple multi-cast
		if (ToClient == TEXT("All"))
		{
			//Success is all of the messages emitted successfully
			bool Success = true;
			TArray<TSharedPtr<FTCPClient>> AllClients;

			Clients.GenerateValueArray(AllClients);
			for (TSharedPtr<FTCPClient>& Client : AllClients)
			{
				if (Client.IsValid())
				{
					bool Sent = Client->Socket->Send(Bytes.GetData(), Bytes.Num(), BytesSent);
					if (!Sent && bDisconnectOnFailedEmit)
					{
						Client->Socket->Close();
					}
					Success = Sent && Success;
				}
			}
			return Success;
		}
		//match client address and port
		else
		{
			TSharedPtr<FTCPClient> Client = Clients[ToClient];

			if (Client.IsValid())
			{
				bool Sent = Client->Socket->Send(Bytes.GetData(), Bytes.Num(), BytesSent);
				if (!Sent && bDisconnectOnFailedEmit)
				{
					Client->Socket->Close();
				}
				return Sent;
			}
		}
	}
	return false;
}

void ATCPServerActor::DisconnectClient(FString ClientAddress /*= TEXT("All")*/, bool bDisconnectNextTick/*=false*/)
{
	TFunction<void()> DisconnectFunction = [this, ClientAddress]
	{
		bool bDisconnectAll = ClientAddress == TEXT("All");

		if (!bDisconnectAll)
		{
			TSharedPtr<FTCPClient> Client = Clients[ClientAddress];

			if (Client.IsValid())
			{
				Client->Socket->Close();
				Clients.Remove(Client->Address);
				OnClientDisconnected.Broadcast(ClientAddress);
			}
		}
		else
		{
			for (auto ClientPair : Clients)
			{
				TSharedPtr<FTCPClient> Client = ClientPair.Value;
				Client->Socket->Close();
				Clients.Remove(Client->Address);
				OnClientDisconnected.Broadcast(ClientAddress);
			}
		}
	};

	if (bDisconnectNextTick)
	{
		//disconnect on next tick
		AsyncTask(ENamedThreads::GameThread, DisconnectFunction);
	}
	else
	{
		DisconnectFunction();
	}
}
