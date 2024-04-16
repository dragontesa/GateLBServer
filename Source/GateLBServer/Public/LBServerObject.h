// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core.h"
#include "UObject/NoExportTypes.h"
#include "LBServerSocket.h"
#include "LBMessage.h"

#include "LBServerObject.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(GateLBServerLog, Log, All);

USTRUCT(BlueprintType)
struct FLBServerMatchInfo
{
	GENERATED_BODY()

public:
    FLBServerMatchInfo& operator>>(FLBGameServerMatchMessage& MatchMessage)
	{
		MatchMessage.Port = Port;
		MatchMessage.ServerIp4 = ServerIp4;
		MatchMessage.ServerIp6 = ServerIp6;
		return *this;
	}

	UPROPERTY(BlueprintReadWrite, Category = "GateLBServer")
	int32 GameId;
	
	UPROPERTY(BlueprintReadWrite, Category = "GateLBServer")
    int32   Port;

	UPROPERTY(BlueprintReadWrite, Category = "GateLBServer")
    int32   Padding;

    UPROPERTY(BlueprintReadWrite, Category = "GateLBServer")
	int32  GamePlayerNumbers;  // Server'll be get matched when GmaePlayerNumbers is 0

	UPROPERTY(BlueprintReadWrite, Category = "GateLBServer")
	FString ServerIp4;

	UPROPERTY(BlueprintReadWrite, Category = "GateLBServer")
    FString ServerIp6;

};

class ULBServerObject;
class FLBServerSocket;
using FLBServerMatchResult = FLBServerMatchInfo;

// Delegate would be not suite on concurrency case as long as Delegate not support shared pointer, 
// so only the way for job fetch logic is callback
#define USE_DELEGATE_ON_CONCURRENCY_CASE 0
#if USE_DELEGATE_ON_CONCURRENCY_CASE
DECLARE_DELEGATE(OnFindServerMatchDelegate);
DECLARE_DELEGATE_OneParam(OnResultServerMatchDelegate, FLBServerMatchResult, MatchInfo);
DECLARE_DELEGATE_OneParam(OnIncreaseGamePlayerDelegate, const FString&, DestServerIp);
DECLARE_DELEGATE_OneParam(OnDecreaseGamePlayerDelegate, const FString&, DestServerIp);
#else
// using OnFindServerMatchCallback = void(*)(TSharedPtr<TPromise<FLBServerMatchResult>>);
using OnFindServerMatchCallbackType = TMemFunPtrType<false, ULBServerObject, void(TSharedPtr<TPromise<FLBServerMatchResult>>)>::Type;
using OnRequestIncreaseGamePlayerCallbackType = TMemFunPtrType<false, ULBServerObject, void(const FString&)>::Type;
using OnRequestDecreaseGamePlayerCallbackType = TMemFunPtrType<false, ULBServerObject, void(const FString&)>::Type;
#endif

struct JobContextType{
	TSharedPtr<FLBServerSocket>             Customer;
	FLBMessageToken                         Token;
	ELBMessageType                          MessageType;
	int32                                   GameId;
};

class FLBServerJob
{
	friend class ULBServerObject;
	public:
	FLBServerJob(const JobContextType&);
	~FLBServerJob();
	void Execute();

    private:
	TFuture<FLBServerMatchResult> RequestFindServerMatch();
	void RequestIncreaseGamePlayer();
	void RequestDecreaseGamePlayer();
	void HandleServerMatchResult(TFuture<FLBServerMatchResult> MatchResult);

    private:
    /**
	 * Interfaces to tell message between ServerSock and Front-Object
	*/
// #if USE_DELEGATE_ON_CONCURRENCY_CASE
// 	OnFindServerMatchDelegate  FindServerMatchDelegate;
// 	OnIncreaseGamePlayerDelegate IncreaseGamePlayerDelegate;
// 	OnDecreaseGamePlayerDelegate DecreaseGamePlayerDelegate;
// #else
//  OnFindServerMatchCallbackType FindServerMatchCallback;
// 	OnRequestIncreaseGamePlayerCallbackType RequestIncreaseGamePlayerCallback;
// 	OnRequestDecreaseGamePlayerCallbackType RequestDecreaseGamePlayerCallback;
// #endif

    ULBServerObject* Server;
	TSharedPtr<FLBServerSocket>  Customer;
	FLBMessageToken              MessageToken;
	ELBMessageType               MessageType;
};

/**
 * 
 */
UCLASS(Blueprintable)
class GATELBSERVER_API ULBServerObject : public UObject
{
	GENERATED_BODY()

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGameServerMatchDelegate, int32, id, int32, GameId);     // Update Game Id in the Server Match Info on map & UI
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnIncreaseGamePlayerDelegate, int32, id, int32, GameId);  // Increase Player number in the Server Match Info on map & UI
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDecreaseGamePlayerDelegate, int32, id, int32, GameId);  // Decrease Player number in the Server Match Info on map & UI

	public:
	ULBServerObject();
	~ULBServerObject();

    void RequestServerJob(TSharedPtr<FLBServerJob> Job);

	UFUNCTION(BlueprintCallable, Category = "GateLBServer")
	void AddGameServerMatch(int32 ServerId, const FLBServerMatchInfo& ServerMatchMessage);

	UFUNCTION(BlueprintCallable, Category = "GateLBServer")
	void RemoveGameServerMatch(int32 ServerId);

    UPROPERTY(BlueprintAssignable,Category = "GateLBServer")
    FOnGameServerMatchDelegate UpdateGameServerMatchEvent;

	UPROPERTY(BlueprintAssignable,Category = "GateLBServer")
	FOnIncreaseGamePlayerDelegate IncreaseGamePlayerEvent;

	UPROPERTY(BlueprintAssignable,Category = "GateLBServer")
	FOnDecreaseGamePlayerDelegate DecreaseGamePlayerEvent;

    UPROPERTY(EditAnywhere,Category = "GateLBServer")
	TMap<int32, FLBServerMatchInfo> MatchServerMap; // (server id(list seq), server match info)

    void HandleFindServerMatch(int32 GameId, TSharedPtr<TPromise<FLBServerMatchResult>> MatchPromise);
	void HandleIncreaseGamePlayer(int32 GameId);
	void HandleDecreaseGamePlayer(int32 GameId);

   friend class FLBServerJob;
    private:
	TQueue<TSharedPtr<FLBServerJob>, EQueueMode::Spsc> JobQueue;
	FLBServerSocket LBServerListenSock;
};
