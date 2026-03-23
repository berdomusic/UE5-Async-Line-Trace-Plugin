// Copyright 2024 Berdo Music Michal Cywinski.All rights reserved.

#include "AsyncLineTrace.h"
#include "AsyncTraceSubsystem.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "WorldCollision.h"
#include "Engine/HitResult.h"

void UAsyncLineTrace::Activate()
{
	WeakWorldContextObject = InputData.WorldContextObject;

	if (!bValidityCheck())
	{
		ASYNC_TRACE_LOG(LogAsyncTrace, Error, TEXT("Can't start Async Line Trace"));
		ExitAsyncTraceTask();
		return;
	}

	bTraceInProgress = true;
	OutHits.Empty();
	PendingTraceCount = InputData.StartAndEndLocations.Num();
	DebugTraces.Empty();
	
	StartAsyncTraceTask();
}

void UAsyncLineTrace::CancelAsyncLineTrace()
{
	ASYNC_TRACE_LOG(LogAsyncTrace, Warning, TEXT("Async LineTrace cancelled"));
	bCalledCancel = true;
	ExitAsyncTraceTask();
}

void UAsyncLineTrace::StartAsyncTraceTask()
{
	if (!WeakWorldContextObject.IsValid())
		return;
	UAsyncTraceSubsystem* subsystem = UAsyncTraceSubsystem::Get(WeakWorldContextObject.Get());
	if (!subsystem)
	{
		ASYNC_TRACE_LOG(LogAsyncTrace, Error, TEXT("Subsystem not valid"));
		ExitAsyncTraceTask();
		return;
	}
	if (InputData.StartAndEndLocations.IsEmpty())
	{
		ASYNC_TRACE_LOG(LogAsyncTrace, Error, TEXT("No input data provided"));
		ExitAsyncTraceTask();
		return;
	}	
	subsystem->RegisterAsyncLineTrace(this);
	PerformAsyncTraces();
}

void UAsyncLineTrace::PerformAsyncTraces()
{
	UWorld* world = WeakWorldContextObject.Get()->GetWorld();
	if (!IsValid(world))
	{
		ASYNC_TRACE_LOG(LogAsyncTrace, Error, TEXT("World not valid"));
		ExitAsyncTraceTask();
		return;
	}
	FCollisionQueryParams params;
	params.bTraceComplex = InputData.bTraceComplex;
	params.AddIgnoredActors(InputData.ActorsToIgnore);
	
	FCollisionObjectQueryParams objectTypes;
	if (TraceType == ObjectType)
		for (const TEnumAsByte<EObjectTypeQuery>& objectTypeToAdd : ObjectTypes)
			objectTypes.AddObjectTypesToQuery(UEngineTypes::ConvertToCollisionChannel(objectTypeToAdd));
	
	for (int32 i = 0; i < InputData.StartAndEndLocations.Num(); ++i)
	{
		FVector start;
		FVector end;
		GetCurrentTraceLocations(InputData.StartAndEndLocations[i], start, end);
		DebugTraces.Add(FTraceStartStopVectors(start, end));
		
		FTraceDelegate traceDelegate;
		traceDelegate.BindWeakLambda(this,
	[WeakThis = TWeakObjectPtr<UAsyncLineTrace>(this)]
		(const FTraceHandle& Handle, FTraceDatum& Data)
{
	if (!WeakThis.IsValid()) return;
	WeakThis->OnAsyncTraceCompleted(Handle, Data);
});
		switch (TraceType)
		{
		case Channel:
			world->AsyncLineTraceByChannel(TraceOutput, start, end, CollisionChannel, params,
		FCollisionResponseParams::DefaultResponseParam, &traceDelegate);
			break;
			
		case Profile:
			world->AsyncLineTraceByProfile(TraceOutput, start, end, CollisionProfile, params,
				&traceDelegate);
			break;
			
		case ObjectType:
			world->AsyncLineTraceByObjectType(TraceOutput, start, end, objectTypes, params, 
				&traceDelegate);
			break;
			
		default:
			checkNoEntry()
			break;
		}
	}
}

void UAsyncLineTrace::GetCurrentTraceLocations(const FTraceStartStopVectors& InVectors, FVector& OutStart,
                                               FVector& OutEnd) const
{
	if (IsValid(InputData.TraceOrginActor))
	{
		OutStart = InputData.TraceOrginActor->GetActorLocation();
		OutEnd = OutStart + InVectors.EndLocation;
	}
	else
	{
		OutStart = InVectors.StartLocation;
		OutEnd = InVectors.EndLocation;
	}
}

void UAsyncLineTrace::OnAsyncTraceCompleted(const FTraceHandle& InHandle, FTraceDatum& InData)
{
	if (bCalledCancel)
	{
		ASYNC_TRACE_LOG(LogAsyncTrace, Warning, TEXT("Trace was cancelled"));
		return;
	}
	if (!WeakWorldContextObject.IsValid())
	{
		ASYNC_TRACE_LOG(LogAsyncTrace, Error, TEXT("World context invalid"));
		return;
	}

	const UWorld* world = WeakWorldContextObject->GetWorld();
	if (!world)
	{
		ASYNC_TRACE_LOG(LogAsyncTrace, Error, TEXT("world context invalid"));
		return;
	}

	if (!InData.OutHits.IsEmpty())
	{
		switch (TraceOutput)
		{
		case EAsyncTraceType::Single:
			HandleSingleLineTrace(InData, world);
			break;
		case EAsyncTraceType::Multi:
			HandleMultiLineTrace(InData, world);
			break;
		default:
			checkNoEntry()
		}
	}

	PendingTraceCount--;

	if (PendingTraceCount <= 0)
	{
		// Debug draw
		if (InputData.bDebugDraw && !DebugTraces.IsEmpty())
			for (const FTraceStartStopVectors& trace : DebugTraces)
				DrawDebugLine(world,
					trace.StartLocation,
					trace.EndLocation,
					InputData.TraceColor.ToFColor(true),
					false,
					InputData.DrawTime,
					0,
					2.0f);
		ExitAsyncTraceTask();
	}
}

void UAsyncLineTrace::ExitAsyncTraceTask()
{
	bTraceInProgress = false;
	if (WeakWorldContextObject.IsValid())
		if (UAsyncTraceSubsystem* subsystem = UAsyncTraceSubsystem::Get(WeakWorldContextObject.Get()))
			subsystem->UnregisterAsyncLineTrace(this);
	OnCompleted.Broadcast(OutHits);	
	SetReadyToDestroy();
}

bool UAsyncLineTrace::bValidityCheck() const
{
	if (!IsValid(InputData.WorldContextObject))
	{
		ASYNC_TRACE_LOG(LogAsyncTrace, Error, TEXT("Invalid world context object"));
		return false;
	}

	if (!InputData.WorldContextObject->GetWorld())
	{
		ASYNC_TRACE_LOG(LogAsyncTrace, Error, TEXT("Invalid world"));
		return false;
	}
	
	if (!WeakWorldContextObject.IsValid())
	{
		ASYNC_TRACE_LOG(LogAsyncTrace, Error, TEXT("Invalid weak ptr"));
		return false;
	}

	if (InputData.StartAndEndLocations.Num() <= 0)
	{
		ASYNC_TRACE_LOG(LogAsyncTrace, Error, TEXT("No start/end locations provided"));
		return false;
	}
	if (bTraceInProgress)
	{
		ASYNC_TRACE_LOG(LogAsyncTrace, Warning, TEXT("Trace still in progress"));
		return false;
	}
	return true;
}

void UAsyncLineTrace::ConvertTraceType(ETraceOutput InCustomType)
{
	switch (InCustomType)
	{
	case ETraceOutput::Single:
		TraceOutput = EAsyncTraceType::Single;
		break;
	case ETraceOutput::Multi:
		TraceOutput = EAsyncTraceType::Multi;
		break;
	default:
		TraceOutput = EAsyncTraceType::Single;
		break;
	}
}

void UAsyncLineTrace::HandleSingleLineTrace(FTraceDatum& InData, const UWorld* World)
{
	const FHitResult& hit = InData.OutHits[0];
	OutHits.Add(InData.OutHits[0]);
	HandleDebugs(World, hit);	
}

void UAsyncLineTrace::HandleMultiLineTrace(const FTraceDatum& InData, const UWorld* World)
{
	const TArray<FHitResult>& currentHits = InData.OutHits;
	for (const FHitResult& hit : currentHits)
	{
		OutHits.Add(hit);
		HandleDebugs(World, hit);
	}
}

void UAsyncLineTrace::HandleDebugs(const UWorld* InWorld, const FHitResult& InHitResult) const
{
	if (InputData.bPrintCurrentHitInfo)
		DebugPrintHitInfo(InHitResult);
	if (InputData.bDebugDraw)
		DrawDebugSphere(InWorld, InHitResult.Location, 5.f, 12, InputData.HitColor.ToFColor(true), false, InputData.DrawTime, 0, 5);
}

void UAsyncLineTrace::DebugPrintHitInfo(const FHitResult& InHit)
{
	AActor* actor = InHit.GetActor();
	if (!IsValid(actor))
		return;	
	const FString actorName = actor->GetActorNameOrLabel();
	const FVector hitLocation = InHit.ImpactPoint;
	ASYNC_TRACE_LOG(LogAsyncTrace, Log, TEXT("Hit Actor: %s at Location: %s"), *actorName, *hitLocation.ToString());
}

UAsyncLineTraceChannel* UAsyncLineTraceChannel::AsyncLineTraceChannel(TEnumAsByte<ETraceOutput> InTraceType, ECollisionChannel InChannel, const FAsyncTraceInputData InData)
{
	UAsyncLineTraceChannel* Node = NewObject<UAsyncLineTraceChannel>();

	Node->ConvertTraceType(InTraceType);
	Node->CollisionChannel = InChannel;
	Node->InputData = InData;
	Node->CurrentTraceID = InData.TraceID;
	Node->TraceType = Channel;
	
	return Node;
}

UAsyncLineTraceProfile* UAsyncLineTraceProfile::AsyncLineTraceProfile(TEnumAsByte<ETraceOutput> InTraceType,
                                                                      FName InCollisionProfile, const FAsyncTraceInputData InData)
{
	UAsyncLineTraceProfile* Node = NewObject<UAsyncLineTraceProfile>();

	Node->ConvertTraceType(InTraceType);
	Node->CollisionProfile = InCollisionProfile;
	Node->InputData = InData;
	Node->CurrentTraceID = InData.TraceID;
	Node->TraceType = Profile;
	
	return Node;
}

UAsyncLineTraceObjects* UAsyncLineTraceObjects::AsyncLineTraceObjects(TEnumAsByte<ETraceOutput> InTraceType,
	TArray<TEnumAsByte<EObjectTypeQuery>> InObjectTypes, const FAsyncTraceInputData InData)
{
	UAsyncLineTraceObjects* Node = NewObject<UAsyncLineTraceObjects>();

	Node->ConvertTraceType(InTraceType);
	Node->ObjectTypes = InObjectTypes;
	Node->InputData = InData;
	Node->CurrentTraceID = InData.TraceID;
	Node->TraceType = ObjectType;
	
	return Node;
}