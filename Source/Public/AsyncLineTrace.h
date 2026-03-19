// Copyright 2024 Berdo Music Michal Cywinski. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "WorldCollision.h"
#include "AsyncLineTrace.generated.h"

#pragma region
// Using custom enum because EAsyncTraceType is not blueprint exposed by default
UENUM(BlueprintType)
enum ETraceOutput : uint8
{
	Single UMETA(DisplayName = "Single Trace"),
	Multi UMETA(DisplayName = "Multi Trace")
};

enum ETraceType : uint8
{
	Channel,
	Profile,
	ObjectType
};

USTRUCT(BlueprintType)
struct FTraceStartStopVectors
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "AsyncLineTrace")
	FVector StartLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, Category = "AsyncLineTrace")
	FVector EndLocation = FVector::ZeroVector;

	bool operator==(FTraceStartStopVectors OtherVectors) const
	{
		return StartLocation == OtherVectors.StartLocation && EndLocation == OtherVectors.EndLocation;
	}

	bool operator==(const FTraceStartStopVectors& OtherVectors) const
	{
		return StartLocation == OtherVectors.StartLocation && EndLocation == OtherVectors.EndLocation;
	}
};

USTRUCT(BlueprintType)
struct FAsyncTraceInputData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "AsyncLineTrace")
	FName TraceID;
	UPROPERTY(BlueprintReadWrite, Category = "AsyncLineTrace")
	const UObject* WorldContextObject = nullptr;
	UPROPERTY(BlueprintReadWrite, Category = "AsyncLineTrace")
	const AActor* TraceOrginActor = nullptr;
	UPROPERTY(BlueprintReadWrite, Category = "AsyncLineTrace")
	TArray<FTraceStartStopVectors> StartAndEndLocations;
	UPROPERTY(BlueprintReadWrite, Category = "AsyncLineTrace")
	bool bTraceComplex = false;
	UPROPERTY(BlueprintReadWrite, Category = "AsyncLineTrace")
	TArray<AActor*> ActorsToIgnore;

	//DEBUG Helpers
	UPROPERTY(BlueprintReadWrite, Category = "AsyncLineTrace")
	bool bPrintCurrentHitInfo = false;
	UPROPERTY(BlueprintReadWrite, Category = "AsyncLineTrace")
	bool bDebugDraw = false;
	UPROPERTY(BlueprintReadWrite, Category = "AsyncLineTrace")
	float DrawTime = 2.f;
	UPROPERTY(BlueprintReadWrite, Category = "AsyncLineTrace")
	FLinearColor TraceColor = FLinearColor::Red;
	UPROPERTY(BlueprintReadWrite, Category = "AsyncLineTrace")
	FLinearColor HitColor = FLinearColor::Green;
};

#pragma endregion // Structs, enums

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLineTraceHitsCompleted, const TArray<FHitResult>&, OutHits);

UCLASS()
class ASYNCLINETRACEPLUGIN_API UAsyncLineTrace : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	bool operator==(const UAsyncLineTrace& Other) const
	{
		return this == &Other;
	}	

	FName CurrentTraceID = "";
	TArray<FHitResult> OutHits;

	void CancelAsyncLineTrace();

protected:
	// Using custom enum because EAsyncTraceType is not blueprint exposed by default
	void ConvertTraceType(ETraceOutput InCustomType);
	EAsyncTraceType TraceOutput;
	
	FAsyncTraceInputData InputData;
	ETraceType TraceType = ETraceType::Channel;	
	
	ECollisionChannel CollisionChannel;
	FName CollisionProfile;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	
private:	
	virtual void Activate() override;
	UFUNCTION()
	void StartAsyncTraceTask();
	void PerformAsyncTraces();
	void GetCurrentTraceLocations(const FTraceStartStopVectors& InVectors, FVector& OutStart, FVector& OutEnd) const;
	void OnAsyncTraceCompleted(const FTraceHandle& InHandle, FTraceDatum& InData);
	UFUNCTION()
	void ExitAsyncTraceTask();
	bool bTraceInProgress;
	bool bCalledCancel;
	
	FLineTraceHitsCompleted OnCompleted;	
	int32 PendingTraceCount = 0;	
	TArray<FTraceStartStopVectors> DebugTraces;

	UPROPERTY()
	TWeakObjectPtr<const UObject> WeakWorldContextObject;
	
	bool bValidityCheck() const;	
	

	void HandleSingleLineTrace(FTraceDatum& InData, const UWorld* World);
	void HandleMultiLineTrace(const FTraceDatum& InData, const UWorld* World);

	void HandleDebugs(const UWorld* InWorld, const FHitResult& InHitResult) const;
	static void DebugPrintHitInfo(const FHitResult& InHit);
};

UCLASS()
class ASYNCLINETRACEPLUGIN_API UAsyncLineTraceChannel : public UAsyncLineTrace
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"), Category = "AsyncTrace")
	static UAsyncLineTraceChannel* AsyncLineTraceChannel(TEnumAsByte<ETraceOutput> InTraceType,
		ECollisionChannel InChannel, const FAsyncTraceInputData InData);
};

UCLASS()
class ASYNCLINETRACEPLUGIN_API UAsyncLineTraceProfile : public UAsyncLineTrace
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"), Category = "AsyncTrace")
	static UAsyncLineTraceProfile* AsyncLineTraceProfile(TEnumAsByte<ETraceOutput> InTraceType,
		FName InCollisionProfile, const FAsyncTraceInputData InData);
};

UCLASS()
class ASYNCLINETRACEPLUGIN_API UAsyncLineTraceObjects : public UAsyncLineTrace
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"), Category = "AsyncTrace")
	static UAsyncLineTraceObjects* AsyncLineTraceObjects(TEnumAsByte<ETraceOutput> InTraceType,
		TArray<TEnumAsByte<EObjectTypeQuery>> InObjectTypes, const FAsyncTraceInputData InData);
};