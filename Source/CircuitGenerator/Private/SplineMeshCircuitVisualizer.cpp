// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for license information.

#include "SplineMeshCircuitVisualizer.h"

#if WITH_EDITOR

#include "SplineMeshCircuit.h"
#include "Components/SplineMeshComponent.h"
#include "SceneView.h"
#include "EditorModeManager.h"
#include "EditorViewportClient.h"
#include "HitProxies.h"
#include "Editor.h"
#include "ScopedTransaction.h"

IMPLEMENT_HIT_PROXY(HCircuitEndpointProxy, HComponentVisProxy);

FSplineMeshCircuitComponentVisualizer::FSplineMeshCircuitComponentVisualizer()
{
}

void FSplineMeshCircuitComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	const USplineMeshComponent* SplineComp = Cast<const USplineMeshComponent>(Component);
	if (!SplineComp)
	{
		return;
	}

	ASplineMeshCircuit* OwnerActor = nullptr;
	int32 SegmentIndex = INDEX_NONE;
	if (!ResolveActorAndIndexFromComponent(SplineComp, OwnerActor, SegmentIndex))
	{
		return; // not part of a SplineMeshCircuit
	}

 FVector StartPos, StartTan, EndPos, EndTan;
	// Access start/end via const-friendly accessors to avoid casting
	StartPos = SplineComp->GetStartPosition();
	StartTan = SplineComp->GetStartTangent();
	EndPos = SplineComp->GetEndPosition();
	EndTan = SplineComp->GetEndTangent();

	// Draw start/end points with different colors
	const float HandleSize = 8.0f;
	const FLinearColor StartColor = FLinearColor::Green;
	const FLinearColor EndColor = FLinearColor::Red;

 PDI->SetHitProxy(new HCircuitEndpointProxy(const_cast<UActorComponent*>(static_cast<const UActorComponent*>(SplineComp)), SegmentIndex, false));
	PDI->DrawPoint(StartPos, StartColor, HandleSize, SDPG_Foreground);
	PDI->SetHitProxy(nullptr);

 PDI->SetHitProxy(new HCircuitEndpointProxy(const_cast<UActorComponent*>(static_cast<const UActorComponent*>(SplineComp)), SegmentIndex, true));
	PDI->DrawPoint(EndPos, EndColor, HandleSize, SDPG_Foreground);
	PDI->SetHitProxy(nullptr);
}

bool FSplineMeshCircuitComponentVisualizer::VisProxyHandleClick(FEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click)
{
	EditedActor.Reset();
	EditedComponent.Reset();
	EditedSegmentIndex = INDEX_NONE;
	bEditingEnd = true;
	PrevCollisionState = ECollisionEnabled::NoCollision;
	EditTransaction.Reset();

	if (VisProxy && VisProxy->Component.IsValid())
	{
		if (VisProxy->IsA(HCircuitEndpointProxy::StaticGetType()))
		{
			HCircuitEndpointProxy* Proxy = static_cast<HCircuitEndpointProxy*>(VisProxy);
			const USplineMeshComponent* CompConst = Cast<const USplineMeshComponent>(Proxy->Component.Get());
			USplineMeshComponent* Comp = const_cast<USplineMeshComponent*>(CompConst);
			ASplineMeshCircuit* Actor = nullptr; int32 Index = INDEX_NONE;
			if (ResolveActorAndIndexFromComponent(Comp, Actor, Index))
			{
				EditedActor = Actor;
				EditedComponent = Comp;
				// If user clicked a Start handle that is not the very first, remap to previous segment's End
				EditedSegmentIndex = Proxy->bIsEnd ? Index : FMath::Max(0, Index - 1);
				bEditingEnd = true; // we always edit an End in the data model (except global start)

				// If start of first segment, mark as editing global start
				if (!Proxy->bIsEnd && Index == 0)
				{
					bEditingEnd = false;
				}

				// Begin editor transaction for undo/redo
				EditTransaction = MakeUnique<FScopedTransaction>(NSLOCTEXT("CircuitGenerator", "EditSplineMeshCircuit", "Edit Spline Mesh Circuit"));
				if (Actor)
				{
					Actor->Modify();
				}

				if (EditedComponent.IsValid())
				{
					PrevCollisionState = EditedComponent->GetCollisionEnabled();
					EditedComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				}
				return true;
			}
		}
	}
	return false;
}

bool FSplineMeshCircuitComponentVisualizer::HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale)
{
	if (!EditedActor.IsValid())
	{
		return false;
	}

	UpdateDataFromWidgetDelta(DeltaTranslate, DeltaRotate, DeltaScale);
	return true;
}

bool FSplineMeshCircuitComponentVisualizer::HandleInputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	// Not used; return false to allow others to process keys
	return false;
}

void FSplineMeshCircuitComponentVisualizer::EndEditing()
{
	if (EditedComponent.IsValid())
	{
		EditedComponent->SetCollisionEnabled(PrevCollisionState);
	}
	// End undo transaction by destroying the scoped object
	EditTransaction.Reset();
	EditedActor.Reset();
	EditedComponent.Reset();
	EditedSegmentIndex = INDEX_NONE;
}

void FSplineMeshCircuitComponentVisualizer::UpdateDataFromWidgetDelta(const FVector& DeltaTranslate, const FRotator& DeltaRotate, const FVector& DeltaScale)
{
	ASplineMeshCircuit* Actor = EditedActor.Get();
	if (!Actor)
	{
		return;
	}

	// Convert world delta into actor local space
	const FTransform& TM = Actor->GetActorTransform();
	const FVector LocalDelta = TM.InverseTransformVector(DeltaTranslate);

	Actor->Modify();

	auto NormalizeRoll = [](float Degrees) -> float { return FRotator::ClampAxis(Degrees); };
	auto ClampScale = [](const FVector2D& In)->FVector2D
	{
		const float MinS = 0.05f, MaxS = 10.0f;
		return FVector2D(FMath::Clamp(In.X, MinS, MaxS), FMath::Clamp(In.Y, MinS, MaxS));
	};

	bool bChanged = false;

	if (!bEditingEnd)
	{
		// Editing global start
		if (!LocalDelta.IsNearlyZero())
		{
			Actor->StartPos += LocalDelta; bChanged = true;
		}
		if (!DeltaRotate.IsZero())
		{
			Actor->StartRoll = NormalizeRoll(Actor->StartRoll + DeltaRotate.Roll); bChanged = true;
		}
		if (!DeltaScale.IsNearlyZero())
		{
			Actor->StartScale = ClampScale(Actor->StartScale + FVector2D(DeltaScale.X, DeltaScale.Y)); bChanged = true;
		}
		if (bChanged && Actor->JoinSmoothing == ECircuitJoinSmoothing::Smooth && Actor->Segments.Num() > 0)
		{
			Actor->ApplyContinuityAtJunction(0);
		}
	}
	else if (Actor->Segments.IsValidIndex(EditedSegmentIndex))
	{
		FSplineMeshSegmentDesc& Seg = Actor->Segments[EditedSegmentIndex];
		if (!LocalDelta.IsNearlyZero())
		{
			Seg.EndPos += LocalDelta; bChanged = true;
		}
		if (!DeltaRotate.IsZero())
		{
			Seg.EndRoll = NormalizeRoll(Seg.EndRoll + DeltaRotate.Roll); bChanged = true;
		}
		if (!DeltaScale.IsNearlyZero())
		{
			Seg.EndScale = ClampScale(Seg.EndScale + FVector2D(DeltaScale.X, DeltaScale.Y)); bChanged = true;
		}
		if (bChanged)
		{
			// Smooth only the junction impacted: between EditedSegmentIndex and EditedSegmentIndex+1
			Actor->ApplyContinuityAtJunction(EditedSegmentIndex);
		}
	}

	if (bChanged)
	{
		Actor->RebuildFromData();
	}
}

bool FSplineMeshCircuitComponentVisualizer::ResolveActorAndIndexFromComponent(const USplineMeshComponent* Comp, ASplineMeshCircuit*& OutActor, int32& OutIndex) const
{
	OutActor = nullptr;
	OutIndex = INDEX_NONE;
	if (!Comp)
	{
		return false;
	}
	AActor* Owner = Comp->GetOwner();
	ASplineMeshCircuit* Circuit = Cast<ASplineMeshCircuit>(Owner);
	if (!Circuit)
	{
		return false;
	}

	const int32 Index = Circuit->GetIndexForComponent(Comp);
	if (Index == INDEX_NONE)
	{
		return false;
	}
	OutActor = Circuit;
	OutIndex = Index;
	return true;
}

#endif // WITH_EDITOR
