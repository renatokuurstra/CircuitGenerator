// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SplineCircuitActor.generated.h"

class USplineComponent;
class USplineMeshComponent;
class UStaticMesh;
class UPhysicalMaterial;

/**
 * ASplineCircuitActor: An actor that spawns USplineMeshComponents along a USplineComponent.
 * Why: To easily generate tracks or roads along a spline path in the editor.
 */
UCLASS()
class CIRCUITGENERATOR_API ASplineCircuitActor : public AActor
{
	GENERATED_BODY()

public:
	ASplineCircuitActor();

	/** The mesh to use for the spline segments */
	UPROPERTY(EditAnywhere, Category = "SplineCircuit")
	TObjectPtr<UStaticMesh> Mesh;

	/** The collision profile to use for the spline segments */
	UPROPERTY(EditAnywhere, Category = "SplineCircuit")
	TEnumAsByte<ECollisionEnabled::Type> CollisionEnabled = ECollisionEnabled::NoCollision;

	/** The collision profile name to use for the spline segments (e.g., "BlockAll", "Pawn") */
	UPROPERTY(EditAnywhere, Category = "SplineCircuit")
	FName CollisionProfileName = TEXT("BlockAll");

	/** The mobility to use for the spline segments. Static is preferred for optimization. */
	UPROPERTY(EditAnywhere, Category = "SplineCircuit")
	TEnumAsByte<EComponentMobility::Type> Mobility = EComponentMobility::Static;

	/** The physics material to apply to the mesh segments */
	UPROPERTY(EditAnywhere, Category = "SplineCircuit")
	TObjectPtr<UPhysicalMaterial> PhysicsMaterialOverride;

	/** 
	 * The length of each mesh segment along the spline.
	 * If 0, it will use the bounds of the mesh.
	 */
	UPROPERTY(EditAnywhere, Category = "SplineCircuit")
	float SegmentLength = 0.0f;

	/** The spline component that defines the path */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SplineCircuit")
	TObjectPtr<USplineComponent> SplineComponent;

	/** Recalculates and spawns spline mesh components along the spline */
	UFUNCTION(CallInEditor, Category = "SplineCircuit")
	void RecalculateSpline();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	/** Internal list of spawned spline mesh components for tracking/cleanup */
	UPROPERTY(VisibleInstanceOnly, Category = "SplineCircuit")
	TArray<TObjectPtr<USplineMeshComponent>> SpawnedMeshComponents;

	void ClearSpawnedComponents();
};
