// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for license information.

#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR

#include "ComponentVisualizer.h"
#include "HitProxies.h"

class ASplineMeshCircuit;
class USplineMeshComponent;
class UActorComponent;
class FPrimitiveDrawInterface;
class FSceneView;
class FScopedTransaction;

/** Hit proxy for selecting and dragging an endpoint handle of a circuit segment */
struct HCircuitEndpointProxy : public HComponentVisProxy
{
	DECLARE_HIT_PROXY();

	HCircuitEndpointProxy(UActorComponent* InComponent, int32 InSegmentIndex, bool bInIsEnd)
		: HComponentVisProxy(InComponent, HPP_Wireframe)
		, SegmentIndex(InSegmentIndex)
		, bIsEnd(bInIsEnd)
	{}

	int32 SegmentIndex;
	bool bIsEnd; // true = End, false = Start
};

/** Component visualizer that provides selection and gizmo editing for ASplineMeshCircuit segments */
class FSplineMeshCircuitComponentVisualizer : public FComponentVisualizer
{
public:
	FSplineMeshCircuitComponentVisualizer();
	virtual ~FSplineMeshCircuitComponentVisualizer() override {}

	// FComponentVisualizer
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual bool VisProxyHandleClick(FEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click) override;
	virtual bool HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale) override;
	virtual bool HandleInputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) override;
	virtual void EndEditing() override;

private:
	TWeakObjectPtr<ASplineMeshCircuit> EditedActor;
	TWeakObjectPtr<USplineMeshComponent> EditedComponent;
	int32 EditedSegmentIndex = INDEX_NONE;
	bool bEditingEnd = true; // true if editing End, false for Start
	ECollisionEnabled::Type PrevCollisionState = ECollisionEnabled::NoCollision;
	TUniquePtr<FScopedTransaction> EditTransaction;

	void UpdateDataFromWidgetDelta(const FVector& DeltaTranslate, const FRotator& DeltaRotate, const FVector& DeltaScale);
	bool ResolveActorAndIndexFromComponent(const USplineMeshComponent* Comp, ASplineMeshCircuit*& OutActor, int32& OutIndex) const;
};

#endif // WITH_EDITOR
