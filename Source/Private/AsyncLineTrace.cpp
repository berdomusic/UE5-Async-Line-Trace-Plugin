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
	WorldContextObject = InputData.WorldContextObject;

	if (!bValidityCheck())
	{
		ASYNC_TRACE_LOG(LogAsyncTrace, Error, TEXT("Can't start Async Line Trace"));
		OnExitedAsyncTrace.Broadcast();
		return;
	}

	bTraceInProgress = true;
	CurrentTraceIndex = 0;
	OutHits.Empty();
	TraceCompletedDelegate.BindUObject(this, &UAsyncLineTrace::OnTraceCompleted);

	OnActivatedAsyncTrace.Broadcast();
}

void UAsyncLineTrace::CancelAsyncLineTrace()
{
	ASYNC_TRACE_LOG(LogAsyncTrace, Warning, TEXT("Async LineTrace cancelled"));
	bCalledCancel = true;
}

bool UAsyncLineTrace::bValidityCheck() const
{
	if (!InputData.WorldContextObject)
	{
		ASYNC_TRACE_LOG(LogAsyncTrace, Error, TEXT("Invalid world context object"));
		return false;
	}

	if (!WorldContextObject->GetWorld())
	{
		ASYNC_TRACE_LOG(LogAsyncTrace, Error, TEXT("Invalid world"));
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

void UAsyncLineTrace::ConvertTraceType(ETraceTypeCustom InCustomType)
{
	switch (InCustomType)
	{
	case ETraceTypeCustom::Single:
		TraceType = EAsyncTraceType::Single;
		break;
	case ETraceTypeCustom::Multi:
		TraceType = EAsyncTraceType::Multi;
		break;
	default:
		TraceType = EAsyncTraceType::Single;
		break;
	}
}

void UAsyncLineTrace::SetCurrentTraceStartEnd()
{
	if (InputData.TraceOrginActor)
	{
		CurrentTraceStart = InputData.TraceOrginActor->GetActorLocation();
		CurrentTraceEnd = CurrentTraceStart + InputData.StartAndEndLocations[CurrentTraceIndex].EndLocation;
	}
	else
	{
		CurrentTraceStart = InputData.StartAndEndLocations[CurrentTraceIndex].StartLocation;
		CurrentTraceEnd = InputData.StartAndEndLocations[CurrentTraceIndex].EndLocation;
	}
}

void UAsyncLineTrace::HandleSingleLineTrace(FTraceDatum& InData, const UWorld* World)
{
	const FHitResult hit = InData.OutHits[0];
	OutHits.Add(InData.OutHits[0]);

	if (InputData.bPrintCurrentHitInfo)
	{
		DebugPrintHitInfo(hit);
	}
	if (InputData.bDebugDraw)
	{
		DrawDebugSphere(World, hit.Location, 5.f, 12, InputData.HitColor.ToFColor(true), false, InputData.DrawTime, 0, 5);
	}
}

void UAsyncLineTrace::HandleMultiLineTrace(const FTraceDatum& InData, const UWorld* World)
{
	const TArray<FHitResult>& currentHits = InData.OutHits;
	for (const FHitResult& hit : currentHits)
	{
		OutHits.Add(hit);
		if (InputData.bPrintCurrentHitInfo)
		{
			DebugPrintHitInfo(hit);
		}
		if (InputData.bDebugDraw)
		{
			DrawDebugSphere(World, hit.Location, 5.f, 12, InputData.HitColor.ToFColor(true), false, InputData.DrawTime, 0, 5);
		}
	}
}

void UAsyncLineTrace::OnTraceCompleted(const FTraceHandle& InHandle, FTraceDatum& InData)
{
	const UWorld* world = WorldContextObject->GetWorld();
	if (world)
	{
		if (InputData.bDebugDraw)
		{
			DrawDebugLine(world, CurrentTraceStart, CurrentTraceEnd,
				InputData.TraceColor.ToFColor(true), false, InputData.DrawTime, NULL, 2.0f);
		}
		if (!InData.OutHits.IsEmpty())
		{
			switch (TraceType)
			{
			case EAsyncTraceType::Single:
				HandleSingleLineTrace(InData, world);
				break;
			case EAsyncTraceType::Multi:
				HandleMultiLineTrace(InData, world);
				break;
			default:
				HandleSingleLineTrace(InData, world);
				break;
			}

		}
	}

	OnRequestedAsyncTrace.Broadcast();
}

void UAsyncLineTrace::DebugPrintHitInfo(const FHitResult& InHit)
{
	const FString actorName = InHit.GetActor()->GetActorNameOrLabel();
	const FVector hitLocation = InHit.ImpactPoint;
	ASYNC_TRACE_LOG(LogAsyncTrace, Log, TEXT("Hit Actor: %s at Location: %s"), *actorName, *hitLocation.ToString());
}

#pragma region

UAsyncLineTraceChannel* UAsyncLineTraceChannel::AsyncLineTraceChannel(TEnumAsByte<ETraceTypeCustom> InTraceType, ECollisionChannel InChannel, const FAsyncTraceInputData InData)
{
	UAsyncLineTraceChannel* Node = NewObject<UAsyncLineTraceChannel>();

	Node->ConvertTraceType(InTraceType);
	Node->CollisionChannel = InChannel;
	Node->InputData = InData;

	Node->CurrentTraceID = InData.TraceID;

	Node->OnActivatedAsyncTrace.AddDynamic(Node, &UAsyncLineTraceChannel::StartLineTraceChannel);
	Node->OnRequestedAsyncTrace.AddDynamic(Node, &UAsyncLineTraceChannel::RequestLineTraceChannel);
	Node->OnExitedAsyncTrace.AddDynamic(Node, &UAsyncLineTraceChannel::ExitLineTraceChannel);
	return Node;
}

void UAsyncLineTraceChannel::StartLineTraceChannel()
{
	UAsyncTraceSubsystem* subsystem = UAsyncTraceSubsystem::Get(WorldContextObject);
	if (!subsystem)
	{
		ExitLineTraceChannel();
		return;
	}
	subsystem->RegisterAsyncLineTrace(this);
	ProcessLineTraceChannel();
}

FTraceHandle UAsyncLineTraceChannel::ProcessLineTraceChannel()
{
	if (!WorldContextObject)
	{
		ASYNC_TRACE_LOG(LogAsyncTrace, Error, TEXT("Invalid world context object"));
		ExitLineTraceChannel();
		return CurrentTraceHandle;
	}

	UWorld* world = WorldContextObject->GetWorld();
	if (!world)
	{
		ASYNC_TRACE_LOG(LogAsyncTrace, Error, TEXT("Invalid world"));
		ExitLineTraceChannel();
		return CurrentTraceHandle;
	}

	FCollisionQueryParams params;
	params.bTraceComplex = InputData.bTraceComplex;
	params.AddIgnoredActors(InputData.ActorsToIgnore);

	SetCurrentTraceStartEnd();

	CurrentTraceHandle = world->AsyncLineTraceByChannel(TraceType, CurrentTraceStart, CurrentTraceEnd, CollisionChannel, params,
		FCollisionResponseParams::DefaultResponseParam, &TraceCompletedDelegate);

	return CurrentTraceHandle;
}

void UAsyncLineTraceChannel::RequestLineTraceChannel()
{
	++CurrentTraceIndex;
	if (CurrentTraceIndex < InputData.StartAndEndLocations.Num() && !bCalledCancel)
	{
		ProcessLineTraceChannel();
	}
	else
	{
		ExitLineTraceChannel();
	}
}

void UAsyncLineTraceChannel::ExitLineTraceChannel()
{
	if (UAsyncTraceSubsystem* subsystem = UAsyncTraceSubsystem::Get(WorldContextObject))
	{
		subsystem->UnregisterAsyncLineTrace(this);
	}

	Completed.Broadcast(OutHits);
	bTraceInProgress = false;
}
#pragma endregion //channel trace

#pragma region

UAsyncLineTraceProfile* UAsyncLineTraceProfile::AsyncLineTraceProfile(TEnumAsByte<ETraceTypeCustom> InTraceType,
	FName InCollisionProfile, const FAsyncTraceInputData InData)
{
	UAsyncLineTraceProfile* Node = NewObject<UAsyncLineTraceProfile>();

	Node->ConvertTraceType(InTraceType);
	Node->CollisionProfile = InCollisionProfile;
	Node->InputData = InData;

	Node->CurrentTraceID = InData.TraceID;

	Node->OnActivatedAsyncTrace.AddDynamic(Node, &UAsyncLineTraceProfile::StartLineTraceProfile);
	Node->OnRequestedAsyncTrace.AddDynamic(Node, &UAsyncLineTraceProfile::RequestLineTraceProfile);
	Node->OnExitedAsyncTrace.AddDynamic(Node, &UAsyncLineTraceProfile::ExitLineTraceProfile);
	return Node;
}

void UAsyncLineTraceProfile::StartLineTraceProfile()
{
	UAsyncTraceSubsystem* subsystem = UAsyncTraceSubsystem::Get(WorldContextObject);
	if (!subsystem)
	{
		ExitLineTraceProfile();
		return;
	}
	subsystem->RegisterAsyncLineTrace(this);
	ProcessLineTraceProfile();
}

FTraceHandle UAsyncLineTraceProfile::ProcessLineTraceProfile()
{
	if (!WorldContextObject)
	{
		//log message
		ExitLineTraceProfile();
		return CurrentTraceHandle;
	}

	UWorld* world = WorldContextObject->GetWorld();
	if (!world)
	{
		//log message
		ExitLineTraceProfile();
		return CurrentTraceHandle;
	}

	FCollisionQueryParams params;
	params.bTraceComplex = InputData.bTraceComplex;
	params.AddIgnoredActors(InputData.ActorsToIgnore);

	SetCurrentTraceStartEnd();

	CurrentTraceHandle = world->AsyncLineTraceByProfile(TraceType, CurrentTraceStart, CurrentTraceEnd, CollisionProfile, params
		, &TraceCompletedDelegate);

	return CurrentTraceHandle;
}

void UAsyncLineTraceProfile::RequestLineTraceProfile()
{
	++CurrentTraceIndex;
	if (CurrentTraceIndex < InputData.StartAndEndLocations.Num() && !bCalledCancel)
	{
		ProcessLineTraceProfile();
	}
	else
	{
		ExitLineTraceProfile();
	}
}

void UAsyncLineTraceProfile::ExitLineTraceProfile()
{
	if (UAsyncTraceSubsystem* subsystem = UAsyncTraceSubsystem::Get(WorldContextObject))
	{
		subsystem->UnregisterAsyncLineTrace(this);
	}

	Completed.Broadcast(OutHits);
	bTraceInProgress = false;
}

#pragma endregion //profile trace

#pragma region

UAsyncLineTraceObjects* UAsyncLineTraceObjects::AsyncLineTraceObjects(TEnumAsByte<ETraceTypeCustom> InTraceType,
	TArray<TEnumAsByte<EObjectTypeQuery>> InObjectTypes, const FAsyncTraceInputData InData)
{
	UAsyncLineTraceObjects* Node = NewObject<UAsyncLineTraceObjects>();

	Node->ConvertTraceType(InTraceType);
	Node->ObjectTypes = InObjectTypes;
	Node->InputData = InData;

	Node->CurrentTraceID = InData.TraceID;

	Node->OnActivatedAsyncTrace.AddDynamic(Node, &UAsyncLineTraceObjects::StartLineTraceObjects);
	Node->OnRequestedAsyncTrace.AddDynamic(Node, &UAsyncLineTraceObjects::RequestLineTraceObjects);
	Node->OnExitedAsyncTrace.AddDynamic(Node, &UAsyncLineTraceObjects::ExitLineTraceObjects);
	return Node;
}

void UAsyncLineTraceObjects::StartLineTraceObjects()
{
	UAsyncTraceSubsystem* subsystem = UAsyncTraceSubsystem::Get(WorldContextObject);
	if (!subsystem)
	{
		ExitLineTraceObjects();
		return;
	}
	subsystem->RegisterAsyncLineTrace(this);
	ProcessLineTraceObjects();
}

FTraceHandle UAsyncLineTraceObjects::ProcessLineTraceObjects()
{
	if (!WorldContextObject)
	{
		//log message
		ExitLineTraceObjects();
		return CurrentTraceHandle;
	}

	UWorld* world = WorldContextObject->GetWorld();
	if (!world)
	{
		//log message
		ExitLineTraceObjects();
		return CurrentTraceHandle;
	}

	FCollisionQueryParams params;
	params.bTraceComplex = InputData.bTraceComplex;
	params.AddIgnoredActors(InputData.ActorsToIgnore);

	FCollisionObjectQueryParams objectTypes;

	for (const TEnumAsByte<EObjectTypeQuery>& objectTypeToAdd : ObjectTypes)
	{
		objectTypes.AddObjectTypesToQuery(UEngineTypes::ConvertToCollisionChannel(objectTypeToAdd));
	}

	SetCurrentTraceStartEnd();

	CurrentTraceHandle = world->AsyncLineTraceByObjectType(TraceType, CurrentTraceStart, CurrentTraceEnd, objectTypes, params
		, &TraceCompletedDelegate);

	return CurrentTraceHandle;
}

void UAsyncLineTraceObjects::RequestLineTraceObjects()
{
	++CurrentTraceIndex;
	if (CurrentTraceIndex < InputData.StartAndEndLocations.Num() && !bCalledCancel)
	{
		ProcessLineTraceObjects();
	}
	else
	{
		ExitLineTraceObjects();
	}
}

void UAsyncLineTraceObjects::ExitLineTraceObjects()
{
	if (UAsyncTraceSubsystem* subsystem = UAsyncTraceSubsystem::Get(WorldContextObject))
	{
		subsystem->UnregisterAsyncLineTrace(this);
	}

	Completed.Broadcast(OutHits);
	bTraceInProgress = false;
}


#pragma endregion //object trace