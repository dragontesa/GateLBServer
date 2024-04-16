// Fill out your copyright notice in the Description page of Project Settings.


#include "LBSocketAsync.h"
#include "Sockets.h"
#include "Common/TcpSocketBuilder.h"
#include "Serialization/ArrayWriter.h"
#include "SocketSubsystem.h"
#include "Networking.h"
#include "SocketSubsystemModule.h"
#include "LBMessage.h"


FLBSocketAsync::FLBSocketAsync(const FString& description)
{
	_Sock = FTcpSocketBuilder(description);
}

FLBSocketAsync::FLBSocketAsync(FSocket* NewSock)
{
	_Sock = NewSock;
}

FLBSocketAsync::~FLBSocketAsync()
{
	Disconnect();
}

void FLBSocketAsync::Disconnect()
{
	if (_Sock != nullptr)
	{
		if (_Sock->GetConnectionState() == SCS_Connected)
		{
			_Sock->Close();
			UE_LOG(LogTemp, Log, TEXT("SockAsync Closed."));
		}
	}
	_Sock = nullptr;
}

void FLBSocketAsync::SendAsync(TSharedPtr<FBufferArchive> Packet)
{
    	AsyncTask(ENamedThreads::AnyThread, [this, Packet]()
		{
			if (_Sock == nullptr || this == nullptr)
			{
				return;
			}

			int32 sentBytes;
            // blocking until send all things in socket buffer
			bool success = _Sock->Send(Packet->GetData(), Packet->Num(), sentBytes);
			if (success)
			{
				OnSendCompleted();
			}
			else
			{
				OnSendFailed();
			}
		});
}


void FLBSocketAsync::RecvAsync()
{
    AsyncTask(ENamedThreads::AnyThread, [this]() {
        if (_Sock == nullptr || this == nullptr) {
            return;
        }
        TArray<uint8> HeaderBuffer;
        uint32 HeaderSize = sizeof(FLBMessageHeader);
        HeaderBuffer.AddZeroed(HeaderSize);

        bool SuccessRecvHeader = false;
        int32 ReadBytes = 0;
        // blocking until read specified bytes
        SuccessRecvHeader = _Sock->Recv(
            HeaderBuffer.GetData(), HeaderBuffer.Num(), ReadBytes, ESocketReceiveFlags::Type::WaitAll);

        // Receive Header
        if (SuccessRecvHeader)
		{
            FLBMessageHeader Header;
            FMemoryReader Reader(HeaderBuffer);
			Reader << Header;

			int32 dataSize = Header.DataSize;
			TArray<uint8> Data;
			Data.SetNumZeroed(dataSize);
			// blocking until read all data bytes
			bool RecvSuccess = _Sock->Recv(Data.GetData(), Data.Num(), ReadBytes, ESocketReceiveFlags::Type::WaitAll);
			if (RecvSuccess)
			{
				OnRecvCompleted(Header.MessageId, Data);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("LBS Recv Data Failed."));
				OnRecvFailed();
			}
        }
		else
		{
			UE_LOG(LogTemp, Error, TEXT("LBS Recv Header Failed."));
			OnRecvFailed();
		}
    });
}

