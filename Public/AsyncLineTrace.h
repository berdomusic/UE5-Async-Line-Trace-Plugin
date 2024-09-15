// Copyright 2024 Berdo Music Michal Cywinski. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "WorldCollision.h"
#include "AsyncLineTrace.generated.h"

#pragma region
// Using custom enum because EAsyncTraceType is not blueprint exposed by default
UENUM(BlueprintType)
enum ETraceTypeCustom : uint8
{
	Single UMETA(DisplayName = "Single Trace"),
	Multi UMETA(DisplayName = "Multi Trace")
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
/**
 *
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FActivateAsyncTrace);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRequestAsyncTrace);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FExitAsyncTrace);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLineTraceHitsCompleted, const TArray<FHitResult>&, OutHits);

/**
 *
 */
UCLASS()
class ASYNCLINETRACEPLUGIN_API UAsyncLineTrace : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:

	bool operator==(const UAsyncLineTrace& Other) const
	{
		return this == &Other;
	}

	virtual void Activate() override;

	FName CurrentTraceID = "";
	TArray<FHitResult> OutHits;

	void CancelAsyncLineTrace();

protected:

	bool bTraceInProgress;
	bool bCalledCancel;

	FAsyncTraceInputData InputData;

	//Delegates
	FActivateAsyncTrace OnActivatedAsyncTrace;
	FRequestAsyncTrace OnRequestedAsyncTrace;
	FExitAsyncTrace OnExitedAsyncTrace;

	FVector CurrentTraceStart;
	FVector CurrentTraceEnd;

	const UObject* WorldContextObject;

	int CurrentTraceIndex;

	UFUNCTION()
	bool bValidityCheck() const;

	EAsyncTraceType TraceType;
	void ConvertTraceType(ETraceTypeCustom InCustomType);

	void SetCurrentTraceStartEnd();

	void HandleSingleLineTrace(FTraceDatum& InData, const UWorld* World);
	void HandleMultiLineTrace(const FTraceDatum& InData, const UWorld* World);

	//Async trace interface
	FTraceHandle CurrentTraceHandle;
	void OnTraceCompleted(const FTraceHandle& InHandle, FTraceDatum& InData);

	FTraceDelegate TraceCompletedDelegate;

	static void DebugPrintHitInfo(const FHitResult& InHit);

private:

};

UCLASS()
class ASYNCLINETRACEPLUGIN_API UAsyncLineTraceChannel : public UAsyncLineTrace
{
	GENERATED_BODY()

private:

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"), Category = "AsyncTrace")
	static UAsyncLineTraceChannel* AsyncLineTraceChannel(TEnumAsByte<ETraceTypeCustom> InTraceType,
		ECollisionChannel InChannel, const FAsyncTraceInputData InData);

	UFUNCTION()
	void StartLineTraceChannel();

	FTraceHandle ProcessLineTraceChannel();

	UFUNCTION()
	void RequestLineTraceChannel();

	UFUNCTION()
	void ExitLineTraceChannel();

	UPROPERTY(BlueprintAssignable)
	FLineTraceHitsCompleted Completed;

	ECollisionChannel CollisionChannel;
};

UCLASS()
class ASYNCLINETRACEPLUGIN_API UAsyncLineTraceProfile : public UAsyncLineTrace
{
	GENERATED_BODY()

private:

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"), Category = "AsyncTrace")
	static UAsyncLineTraceProfile* AsyncLineTraceProfile(TEnumAsByte<ETraceTypeCustom> InTraceType,
		FName InCollisionProfile, const FAsyncTraceInputData InData);

	UFUNCTION()
	void StartLineTraceProfile();

	FTraceHandle ProcessLineTraceProfile();

	UFUNCTION()
	void RequestLineTraceProfile();

	UFUNCTION()
	void ExitLineTraceProfile();

	UPROPERTY(BlueprintAssignable)
	FLineTraceHitsCompleted Completed;

	FName CollisionProfile;
};

UCLASS()
class ASYNCLINETRACEPLUGIN_API UAsyncLineTraceObjects : public UAsyncLineTrace
{
	GENERATED_BODY()

private:

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"), Category = "AsyncTrace")
	static UAsyncLineTraceObjects* AsyncLineTraceObjects(TEnumAsByte<ETraceTypeCustom> InTraceType,
		TArray<TEnumAsByte<EObjectTypeQuery>> InObjectTypes, const FAsyncTraceInputData InData);

	UFUNCTION()
	void StartLineTraceObjects();

	FTraceHandle ProcessLineTraceObjects();

	UFUNCTION()
	void RequestLineTraceObjects();

	UFUNCTION()
	void ExitLineTraceObjects();

	UPROPERTY(BlueprintAssignable)
	FLineTraceHitsCompleted Completed;

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
};