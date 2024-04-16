// Fill out your copyright notice in the Description page of Project Settings.


#include "LBServerSocket.h"
#include "Sockets.h"
#include "Common/TcpSocketBuilder.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "LBServerObject.h"
#include "Containers/Map.h"
#include "LBServerObject.h"

static TMap<uint64, TSharedPtr<FLBServerSocket>> PeersConnectedMap;

FLBServerSocket::FLBServerSocket() :
    FLBSocketAsync(TEXT("ServerSock Created."))
{
	HandleSendCompletedCallback = nullptr;
	HandleSendFailedCallback    = nullptr;
	HandleRecvCompletedCallback = nullptr;
	HandleRecvFailedCallback    = nullptr;
}

FLBServerSocket::FLBServerSocket(FSocket* NewSock) :
    FLBSocketAsync(NewSock)
{
    HandleSendCompletedCallback = nullptr;
	HandleSendFailedCallback    = nullptr;
	HandleRecvCompletedCallback = nullptr;
	HandleRecvFailedCallback    = nullptr;
}

FLBServerSocket::~FLBServerSocket()
{
}

void FLBServerSocket::Initialize(ULBServerObject* ServerObject)
{
    Server = ServerObject;
    _Sock->SetNonBlocking(false);
}

void FLBServerSocket::ServerListen()
{
    FString address = TEXT("127.0.0.1");
    FIPv4Address ip;
    FIPv4Address::Parse(address, ip);
    TSharedRef<FInternetAddr> addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
    addr->SetIp(ip.Value);
    addr->SetPort(7788);
    bool success = _Sock->Bind(*addr);
    if (!success)
    {
        UE_LOG(LogTemp, Log, TEXT("ServerSock Bind Failed."));
        return;
    }

    success = _Sock->Listen(100);

    if (!success)
    {
        UE_LOG(LogTemp, Log, TEXT("ServerSock Listen Failed."));
        return;
    }

    AcceptConnection();
}

void FLBServerSocket::AcceptConnection()
{
    AsyncTask(ENamedThreads::AnyThread, [this]()
    {
        while (!Stopping)
        {
           TSharedRef<FInternetAddr> addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
           FSocket *sock = _Sock->Accept(*addr, TEXT("ClientSock"));
           if (sock == nullptr)
           {
               UE_LOG(LogTemp, Log, TEXT("ServerSock Listen Failed."));
           }
           else
           {
               TSharedPtr<FLBServerSocket> PeerSock = MakeShareable(new FLBServerSocket(sock));
               PeerSock->ThisRef = PeerSock;
            //    PeersConnectedMap.Add(reinterpret_cast<uint64>(sock), PeerSock);
               PeerSock->Initialize(Server);
               PeerSock->RecvAsync();
           }
        }
    });
}


void FLBServerSocket::Finish()
{
    Stopping = true;
    if (ThisRef.IsValid())
    {
        // PeersConnectedMap.Remove(reinterpret_cast<uint64>(_Sock));
        ThisRef.Reset();
    }

    Disconnect();
}

void FLBServerSocket::RegistOnSendCompletedCallback(OnSendCompletedCallback cb)
{
    HandleSendCompletedCallback = cb;
}

void FLBServerSocket::RegistOnSendFailedCallback(OnSendFailedCallback cb)
{
    HandleSendFailedCallback = cb;
}

void FLBServerSocket::RegistOnRecvCompletedCallback(OnRecvCompletedCallback cb)
{
    HandleRecvCompletedCallback = cb;
}

void FLBServerSocket::RegistOnRecvFailedCallback(OnRecvFailedCallback cb)
{
    HandleRecvFailedCallback = cb;
}

void FLBServerSocket::OnSendCompleted()
{
    if (HandleSendCompletedCallback != nullptr)
    {
        // Callback must be called in GameThread
        AsyncTask(ENamedThreads::GameThread, [this](){
            HandleSendCompletedCallback(this);
        });
    }
}

void FLBServerSocket::OnSendFailed()
{
    if (HandleSendFailedCallback != nullptr)
    {
        // Callback must be called in GameThread
        AsyncTask(ENamedThreads::GameThread, [this](){
            HandleSendFailedCallback(this);
        });
    }
}

void FLBServerSocket::OnRecvCompleted(uint32 MessageId, const TArray<uint8>& InData)
{
    FLBMessage Message(MessageId, InData.Num());
    Message.SetData(InData);
    FLBMessageToken Token = Message.MessageToken();

    switch (ELBMessageType(MessageId))
    {
        case ELBMessageType::ELBMessage_MatchGameServer:
        {
            // Request to find game server to match
            JobContextType context;
            context.Customer = ThisRef;
            context.Token    = Token;
            context.MessageType = ELBMessageType::ELBMessage_MatchGameServer;
            PushJob(&context);
        }
        break;
        case ELBMessageType::ELBMessage_IncreaseGamePlayer:
        {
            JobContextType context;
            context.Customer = ThisRef;
            context.Token    = Token;
            context.MessageType = ELBMessageType::ELBMessage_IncreaseGamePlayer;
            int success = Message.GetIncreaseGamePlayerMessage(context.GameId);
            PushJob(&context);
        }
        break;
        case ELBMessageType::ELBMessage_DecreaseGamePlayer:
        {
            JobContextType context;
            context.Customer = ThisRef;
            context.Token    = Token;
            context.MessageType = ELBMessageType::ELBMessage_DecreaseGamePlayer;
            int success = Message.GetDecreaseGamePlayerMessage(context.GameId);
            PushJob(&context);
        }
        break;
        default:
        break;
    }

    if (HandleRecvCompletedCallback != nullptr)
    {
        // Callback must be called in GameThread
        AsyncTask(ENamedThreads::GameThread, [this,MessageId, InData](){
            HandleRecvCompletedCallback(this, MessageId, InData);
        });
    }
}

void FLBServerSocket::OnRecvFailed()
{
    if (HandleRecvFailedCallback != nullptr)
    {
        // Callback must be called in GameThread
        AsyncTask(ENamedThreads::GameThread, [this](){
            HandleRecvFailedCallback(this);
        });
    }
}

void FLBServerSocket::PushJob(const JobContextType* JobContext)
{
   JobContextType context(*JobContext);
   AsyncTask(ENamedThreads::GameThread, [this,context](){
       TSharedPtr<FLBServerJob> job = MakeShareable(new FLBServerJob(context));
       Server->RequestServerJob(job);
   });
}

