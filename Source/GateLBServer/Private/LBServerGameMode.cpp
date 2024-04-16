// Fill out your copyright notice in the Description page of Project Settings.


#include "LBServerGameMode.h"
#include "LBSocketAsync.h"



void ALBServerGameMode::StartPlay()
{
    Super::StartPlay();
}

void ALBServerGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Disconnect and Release Server Socket
    Super::EndPlay(EndPlayReason);
}

void ALBServerGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
}

void ALBServerGameMode::InitServer()
{
    ServerObjectPtr = NewObject<ULBServerObject>();
}

ULBServerObject* ALBServerGameMode::GetLBServerObjectInstance()
{
    return ServerObjectPtr;
}

void ALBServerGameMode::UpdateServerMatchMap(const TArray<FLBServerMatchInfo>& ServerMatchInfoArray)
{
    if (ServerObjectPtr == nullptr)
    {
        UE_LOG(GateLBServerLog, Warning, TEXT("Invalid Server Object !!"));
        return;
    }

    ServerObjectPtr->MatchServerMap.Empty();
    int i = 0;
    for (const auto &a : ServerMatchInfoArray)
    {
        ServerObjectPtr->MatchServerMap.Add(++i, a);
        UE_LOG(GateLBServerLog, Warning, TEXT("Server Info: %s : %d"), *a.ServerIp4, a.Port);
    }
}
