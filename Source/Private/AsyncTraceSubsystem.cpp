// Copyright 2024 Berdo Music Michal Cywinski. All rights reserved.

#include "AsyncTraceSubsystem.h"
#include "AsyncLineTrace.h"
#include "Engine/World.h"
#include "Engine/HitResult.h"
#include "WorldCollision.h"


UAsyncTraceSubsystem* UAsyncTraceSubsystem::Get(const UObject* InWorldContextObject)
{
	if (InWorldContextObject)
		if (const UWorld* const World = InWorldContextObject->GetWorld())
			return World->GetSubsystem<UAsyncTraceSubsystem>();
	return nullptr;
}

TArray<FHitResult> UAsyncTraceSubsystem::GetCurrentHitsByID(FName InID)
{
	Cleanup();
	TArray<FHitResult> currentHits;

	for (const TWeakObjectPtr<UAsyncLineTrace>& weakTrace : ActiveAsyncLineTraces)
		if (UAsyncLineTrace* trace = weakTrace.Get())
			if (trace->CurrentTraceID == InID)
				currentHits.Append(trace->OutHits);
	return currentHits;
}

void UAsyncTraceSubsystem::CancelAsyncLineTracesByID(FName InIDToCancel)
{
	Cleanup();
	for (const TWeakObjectPtr<UAsyncLineTrace>& weakTrace : ActiveAsyncLineTraces)
		if (UAsyncLineTrace* trace = weakTrace.Get())
			if (trace->CurrentTraceID == InIDToCancel)
				trace->CancelAsyncLineTrace();
}

void UAsyncTraceSubsystem::CancelAllAsyncLineTraces()
{
	Cleanup();
	for (const TWeakObjectPtr<UAsyncLineTrace>& weakTrace : ActiveAsyncLineTraces)
		if (UAsyncLineTrace* trace = weakTrace.Get())
			trace->CancelAsyncLineTrace();
}

void UAsyncTraceSubsystem::GetActiveAsyncLineTraces(TArray<UAsyncLineTrace*>& OutTraces)
{
	Cleanup();
	OutTraces.Reset();

	for (const TWeakObjectPtr<UAsyncLineTrace>& weakTrace : ActiveAsyncLineTraces)
		if (UAsyncLineTrace* Trace = weakTrace.Get())
			OutTraces.Add(Trace);
}

void UAsyncTraceSubsystem::RegisterAsyncLineTrace(UAsyncLineTrace* InTrace)
{
	if (InTrace)
		if (!ActiveAsyncLineTraces.Contains(InTrace))
			ActiveAsyncLineTraces.Add(InTrace);
}

void UAsyncTraceSubsystem::UnregisterAsyncLineTrace(const UAsyncLineTrace* InTrace)
{
	if (InTrace)
		for (int32 i = 0; i < ActiveAsyncLineTraces.Num(); ++i)
			if (ActiveAsyncLineTraces[i].Get() == InTrace)
			{
				ActiveAsyncLineTraces.RemoveAt(i);
				break;
			}
}

void UAsyncTraceSubsystem::Cleanup()
{
	if (!ActiveAsyncLineTraces.IsEmpty())
	{
		ActiveAsyncLineTraces.RemoveAll([](const TWeakObjectPtr<UAsyncLineTrace>& ptr)
		{
			return !ptr.IsValid();
		});
		ActiveAsyncLineTraces.Shrink();
	}
}