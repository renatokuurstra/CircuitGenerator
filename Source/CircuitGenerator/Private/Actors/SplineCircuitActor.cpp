// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Actors/SplineCircuitActor.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/StaticMesh.h"

#if WITH_EDITOR
#include "ScopedTransaction.h"
#endif

ASplineCircuitActor::ASplineCircuitActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	SetRootComponent(SplineComponent);
}

void ASplineCircuitActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Optionally we could recalculate on construction, but issue asks for a button.
}

void ASplineCircuitActor::RecalculateSpline()
{
#if WITH_EDITOR
	FScopedTransaction Transaction(FText::FromString("Recalculate Spline"));
	Modify();
#endif

	ClearSpawnedComponents();

	if (!Mesh || !SplineComponent)
	{
		return;
	}

	float CurrentSegmentLength = SegmentLength;
	if (CurrentSegmentLength <= 0.0f)
	{
		const FBoxSphereBounds MeshBounds = Mesh->GetBounds();
		CurrentSegmentLength = MeshBounds.BoxExtent.X * 2.0f;
	}

	if (CurrentSegmentLength <= 0.0f)
	{
		return;
	}

	const float TotalSplineLength = SplineComponent->GetSplineLength();
	const int32 NumberOfSegments = FMath::Max(1, FMath::FloorToInt(TotalSplineLength / CurrentSegmentLength));
	const float ActualSegmentLength = TotalSplineLength / NumberOfSegments;

	for (int32 i = 0; i < NumberOfSegments; ++i)
	{
		FName ComponentName = MakeUniqueObjectName(this, USplineMeshComponent::StaticClass(), TEXT("SplineMeshSegment"));
		USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(this, ComponentName, RF_Transactional);
		
		SplineMesh->RegisterComponent();
		SplineMesh->SetStaticMesh(Mesh);
		SplineMesh->SetMobility(EComponentMobility::Movable);
		SplineMesh->AttachToComponent(SplineComponent, FAttachmentTransformRules::KeepRelativeTransform);

		const float StartDistance = i * ActualSegmentLength;
		const float EndDistance = (i + 1) * ActualSegmentLength;

		const FVector StartPos = SplineComponent->GetLocationAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local);
		const FVector StartTangent = SplineComponent->GetDirectionAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local) * ActualSegmentLength;
		const FVector EndPos = SplineComponent->GetLocationAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Local);
		const FVector EndTangent = SplineComponent->GetDirectionAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Local) * ActualSegmentLength;

		SplineMesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);

		// Apply spline roll and scale
		const float StartRoll = SplineComponent->GetRollAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local);
		const float EndRoll = SplineComponent->GetRollAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Local);
		SplineMesh->SetStartRoll(FMath::DegreesToRadians(StartRoll));
		SplineMesh->SetEndRoll(FMath::DegreesToRadians(EndRoll));

		const FVector StartScale = SplineComponent->GetScaleAtDistanceAlongSpline(StartDistance);
		const FVector EndScale = SplineComponent->GetScaleAtDistanceAlongSpline(EndDistance);
		SplineMesh->SetStartScale(FVector2D(StartScale.Y, StartScale.Z));
		SplineMesh->SetEndScale(FVector2D(EndScale.Y, EndScale.Z));
		
		SpawnedMeshComponents.Add(SplineMesh);
	}
}

void ASplineCircuitActor::ClearSpawnedComponents()
{
	for (USplineMeshComponent* Comp : SpawnedMeshComponents)
	{
		if (Comp)
		{
#if WITH_EDITOR
			Comp->Modify();
#endif
			Comp->DestroyComponent();
		}
	}
	SpawnedMeshComponents.Empty();
}
