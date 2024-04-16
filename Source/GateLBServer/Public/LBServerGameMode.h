// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LBServerObject.h"
#include "LBServerGameMode.generated.h"

/**
 * 
 */
UCLASS()
class GATELBSERVER_API ALBServerGameMode : public AGameModeBase
{
	GENERATED_BODY()

	virtual void StartPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);
	virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "GateLBServer")
	void InitServer();

    UFUNCTION(BlueprintCallable, Category = "GateLBServer")
    ULBServerObject* GetLBServerObjectInstance();
	
	UFUNCTION(BlueprintCallable, Category = "GateLBServer")
	void UpdateServerMatchMap(const TArray<FLBServerMatchInfo>& ServerMatchInfoArray);

	private:
	
	ULBServerObject* ServerObjectPtr = nullptr;

};
