// Copyright 2024 Berdo Music Michal Cywinski. All rights reserved.

#include "AsyncTraceSubsystem.h"
#include "AsyncLineTrace.h"
#include "Engine/World.h"
#include "Engine/HitResult.h"
#include "WorldCollision.h"


UAsyncTraceSubsystem* UAsyncTraceSubsystem::Get(const UObject* InWorldContextObject)
{
	if (InWorldContextObject)
	{
		if (const UWorld* const World = InWorldContextObject->GetWorld())
		{
			return World->GetSubsystem<UAsyncTraceSubsystem>();
		}
	}

	return nullptr;
}


void UAsyncTraceSubsystem::RegisterAsyncLineTrace(UAsyncLineTrace* InTrace)
{
	if (InTrace)
	{
		ActiveAsyncLineTraces.AddUnique(InTrace);
	}
}

void UAsyncTraceSubsystem::UnregisterAsyncLineTrace(UAsyncLineTrace* InTrace)
{
	if (InTrace && ActiveAsyncLineTraces.Contains(InTrace))
	{
		ActiveAsyncLineTraces.Remove(InTrace);
	}
}

TArray<FHitResult> UAsyncTraceSubsystem::GetCurrentHitsByID(FName InID)
{
	TArray<FHitResult> currentHits;

	for (auto const asyncTrace : ActiveAsyncLineTraces)
	{
		if (asyncTrace)
		{
			if (asyncTrace->CurrentTraceID == InID)
			{
				currentHits.Append(asyncTrace->OutHits);
			}
		}
	}

	return currentHits;
}

void UAsyncTraceSubsystem::CancelAsyncLineTracesByID(FName InIDToCancel)
{
	for (auto const asyncTrace : ActiveAsyncLineTraces)
	{
		if (asyncTrace)
		{
			if (asyncTrace->CurrentTraceID == InIDToCancel)
			{
				asyncTrace->CancelAsyncLineTrace();
			}
		}
	}
}

void UAsyncTraceSubsystem::CancelAllAsyncLineTraces()
{
	for (auto const asyncTrace : ActiveAsyncLineTraces)
	{
		if (asyncTrace)
		{
			asyncTrace->CancelAsyncLineTrace();
		}
	}
}
