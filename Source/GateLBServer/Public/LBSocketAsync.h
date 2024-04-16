// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NetworkMessage.h"

// DECLARE_STATS_GROUP(TEXT("LBSocketAsync"), STATGROUP_LBSocketAsync, STATCAT_Advanced);
// DECLARE_CYCLE_STAT(TEXT("Send"), STAT_Send, STATGROUP_LBSocketAsync);
// DECLARE_CYCLE_STAT(TEXT("Recv"), STAT_Recv, STATGROUP_LBSocketAsync);

class FSocket;
/**
 * 
 */
class GATELBSERVER_API FLBSocketAsync
{
public:
	FLBSocketAsync(const FString& description=FString());
	FLBSocketAsync(FSocket* NewSock);
	virtual ~FLBSocketAsync();
	void Disconnect();

	virtual void SendAsync(TSharedPtr<FBufferArchive> Packet);
	virtual void RecvAsync();

    protected:
    // these'll be called in GameThread
	virtual void OnSendCompleted() = 0;
	virtual void OnSendFailed() = 0;

	virtual void OnRecvCompleted(uint32 MessageId, const TArray<uint8>& InData) = 0;
	virtual void OnRecvFailed() = 0;

	protected:
	FSocket* _Sock;
};
