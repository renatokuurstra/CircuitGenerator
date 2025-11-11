// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for license information.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineMeshComponent.h"

struct FPropertyChangedEvent;

#include "SplineMeshCircuit.generated.h"

class USplineMeshComponent;
class UStaticMesh;
class USceneComponent;

/**
 * Describes a spline mesh segment to be instantiated as a USplineMeshComponent.
 * Local-space data: only the End of the segment is authored; the Start is the previous segment's End (or actor Start).
 */
USTRUCT(BlueprintType)
struct CIRCUITGENERATOR_API FSplineMeshSegmentDesc
{
	GENERATED_BODY()

	/** Optional mesh override for this segment; if null, uses the actor DefaultMesh. */
	UPROPERTY(EditAnywhere, Category = "Circuit")
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	/** Local-space end position of the segment. */
	UPROPERTY(EditAnywhere, Category = "Circuit", meta = (MakeEditWidget))
	FVector EndPos = FVector(100.f, 0.f, 0.f);

	/** Local-space tangent direction/length at the end. */
	UPROPERTY(EditAnywhere, Category = "Circuit")
	FVector EndTangent = FVector(100.f, 0.f, 0.f);

	/** Per-end roll (degrees) at End. */
	UPROPERTY(EditAnywhere, Category = "Circuit")
	float EndRoll = 0.f;

	/** Per-end scale at End. X=width, Y=thickness. */
	UPROPERTY(EditAnywhere, Category = "Circuit")
	FVector2D EndScale = FVector2D(1.f, 1.f);

	/** Axis considered as "forward" for the mesh orientation along the spline. */
	UPROPERTY(EditAnywhere, Category = "Circuit")
	TEnumAsByte<ESplineMeshAxis::Type> ForwardAxis = ESplineMeshAxis::X;
};

/** How to smooth the join between adjacent segments when one end is edited. */
UENUM(BlueprintType)
enum class ECircuitJoinSmoothing : uint8
{
	None,   // C0 only
	Smooth  // C0 + adjust tangents locally for smoothness
};

/**
 * Actor that builds a chain of spline mesh components from data descriptors.
 * Construction is data-driven: we mirror the Segments array with components.
 */
UCLASS()
class CIRCUITGENERATOR_API ASplineMeshCircuit : public AActor
{
	GENERATED_BODY()

public:
	ASplineMeshCircuit();

	/** Default mesh if a segment doesn't set Mesh. */
	UPROPERTY(EditAnywhere, Category = "Circuit|Defaults")
	TObjectPtr<UStaticMesh> DefaultMesh = nullptr;

	/** Local-space start parameters for the first segment. */
	UPROPERTY(EditAnywhere, Category = "Circuit|Start", meta = (MakeEditWidget))
	FVector StartPos = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, Category = "Circuit|Start")
	FVector StartTangent = FVector(100.f, 0.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Circuit|Start")
	float StartRoll = 0.f;
	UPROPERTY(EditAnywhere, Category = "Circuit|Start")
	FVector2D StartScale = FVector2D(1.f, 1.f);

	/** Data describing each spline mesh segment in the circuit. Only End fields are authored. */
	UPROPERTY(EditAnywhere, Category = "Circuit")
	TArray<FSplineMeshSegmentDesc> Segments;

	/** Join smoothing policy applied when editing in editor. */
	UPROPERTY(EditAnywhere, Category = "Circuit|Editing")
	ECircuitJoinSmoothing JoinSmoothing = ECircuitJoinSmoothing::Smooth;

protected:
	/** Components generated from Segments. Visible for debugging; kept transient to avoid serialization. */
	UPROPERTY(VisibleAnywhere, Transient, Category = "Circuit")
	TArray<TObjectPtr<USplineMeshComponent>> SegmentComponents;

	// AActor
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITORONLY_DATA
public:
	// Editor-only selection state for UX. Index of selected segment; -1 means none.
	UPROPERTY(Transient)
	int32 SelectedSegmentIndex = -1;
#endif
#if WITH_EDITOR
	void MarkSegmentDirty(int32 Index);
	void RebuildFromData();
	void ApplyContinuityAtJunction(int32 LeftIndex /*end of left joins start of right*/);
#endif

#if WITH_EDITOR
public:
	int32 GetIndexForComponent(const USplineMeshComponent* InComp) const;

	// Adds a new segment initialized to start at the last end and extend 100 units forward (Call in Editor)
	UFUNCTION(CallInEditor, Category = "Circuit|Editing")
	void AddSegment();

	// Compute the default descriptor for a newly added segment at index NewIndex.
	static FSplineMeshSegmentDesc MakeDefaultSegment(const ASplineMeshCircuit& Actor, int32 NewIndex);

protected:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
