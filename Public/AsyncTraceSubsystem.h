// Copyright 2024 Berdo Music Michal Cywinski. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AsyncTraceSubsystem.generated.h"

DEFINE_LOG_CATEGORY_STATIC(LogAsyncTrace, Log, All);

#if !UE_BUILD_SHIPPING
#define ASYNC_TRACE_LOG(CategoryName, Verbosity, Format, ...) UE_LOG(CategoryName, Verbosity, Format, ##__VA_ARGS__)
#else
#define ASYNC_TRACE_LOG(CategoryName, Verbosity, Format, ...)
#endif

class UAsyncLineTrace;
/**
 *
 */
UCLASS()
class ASYNCLINETRACEPLUGIN_API UAsyncTraceSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	static UAsyncTraceSubsystem* Get(const UObject* InWorldContextObject);

	void RegisterAsyncLineTrace(UAsyncLineTrace* InTrace);
	void UnregisterAsyncLineTrace(UAsyncLineTrace* InTrace);

	UFUNCTION(BlueprintCallable, Category = "AsyncLineTrace")
	TArray<FHitResult> GetCurrentHitsByID(FName InID);

	UFUNCTION(BlueprintCallable, Category = "AsyncLineTrace")
	void CancelAsyncLineTracesByID(FName InIDToCancel);

	UFUNCTION(BlueprintCallable, Category = "AsyncLineTrace")
	void CancelAllAsyncLineTraces();

	UPROPERTY(BlueprintReadOnly, Category = "AsyncLineTrace")
	TArray<UAsyncLineTrace*> ActiveAsyncLineTraces;

};
