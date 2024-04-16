// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LBSocketAsync.h"
#include "LBMessage.h"

struct JobContextType;
class ULBServerObject;

/**
 * 
 */
class GATELBSERVER_API FLBServerSocket: public FLBSocketAsync
{
public:
	using OnSendCompletedCallback = void(*)(const FLBServerSocket*);
	using OnSendFailedCallback    = void(*)(const FLBServerSocket*);
	using OnRecvCompletedCallback = void(*)(const FLBServerSocket*, uint32, const TArray<uint8>&);
	using OnRecvFailedCallback    = void(*)(const FLBServerSocket*);

	FLBServerSocket();
	FLBServerSocket(FSocket* NewSock);
	virtual ~FLBServerSocket();

	void Initialize(ULBServerObject* ServerObject);
	void PushJob(const JobContextType* JobContext);
	void ServerListen();
	void AcceptConnection();
	void Finish();

	void RegistOnSendCompletedCallback(OnSendCompletedCallback cb);
	void RegistOnSendFailedCallback(OnSendFailedCallback cb);
	void RegistOnRecvCompletedCallback(OnRecvCompletedCallback cb);
	void RegistOnRecvFailedCallback(OnRecvFailedCallback cb);
	OnSendCompletedCallback   HandleSendCompletedCallback;
	OnSendFailedCallback      HandleSendFailedCallback;
	OnRecvCompletedCallback   HandleRecvCompletedCallback;
	OnRecvFailedCallback      HandleRecvFailedCallback;

protected:
	// these'll be called in None GameThread, 
	virtual void OnSendCompleted();
	virtual void OnSendFailed();
	virtual void OnRecvCompleted(uint32 MessageId, const TArray<uint8>& InData);
	virtual void OnRecvFailed();

	private:
	bool Stopping = false;
	ULBServerObject* Server;
	TSharedPtr<FLBServerSocket> ThisRef;
};
