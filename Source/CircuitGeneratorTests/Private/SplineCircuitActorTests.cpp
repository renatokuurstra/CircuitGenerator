// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "CoreMinimal.h"
#include "CQTest.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Actors/SplineCircuitActor.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Engine/StaticMesh.h"

TEST_CLASS(CircuitGenerator_SplineCircuitActor, "CircuitGenerator.SplineCircuitActor")
{
	TObjectPtr<UWorld> World;
	TObjectPtr<ASplineCircuitActor> Actor;
	TObjectPtr<UStaticMesh> DummyMesh;

	BEFORE_EACH()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false, FName(TEXT("SplineCircuitTestWorld")));
		ASSERT_THAT(IsTrue(World != nullptr, "SplineCircuitTestWorld should be successfully created"));

		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		Actor = World->SpawnActor<ASplineCircuitActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
		ASSERT_THAT(IsTrue(Actor != nullptr, "SplineCircuitActor should be successfully spawned"));

		// Create a simple dummy mesh for testing
		DummyMesh = NewObject<UStaticMesh>(World, TEXT("DummyMesh"), RF_Transient);
		// Note: we can't easily set bounds or geometry on a raw UStaticMesh without more setup, 
		// but we can check if the actor handles it without crashing.
	}

	AFTER_EACH()
	{
		if (World)
		{
			World->DestroyWorld(false);
			World = nullptr;
		}
		Actor = nullptr;
		DummyMesh = nullptr;
	}

	TEST_METHOD(Recalculate_With_No_Mesh_Should_Do_Nothing)
	{
		Actor->Mesh = nullptr;
		Actor->RecalculateSpline();

		TArray<USplineMeshComponent*> Comps;
		Actor->GetComponents<USplineMeshComponent>(Comps);
		ASSERT_THAT(IsTrue(Comps.Num() == 0, "No SplineMeshComponents should be spawned if Mesh is null"));
	}

	TEST_METHOD(Recalculate_Spawns_Components_When_Mesh_Is_Set)
	{
		Actor->Mesh = DummyMesh;
		Actor->SegmentLength = 100.0f;
		
		// Add some length to the spline
		Actor->SplineComponent->ClearSplinePoints();
		Actor->SplineComponent->AddSplinePoint(FVector(0, 0, 0), ESplineCoordinateSpace::Local);
		Actor->SplineComponent->AddSplinePoint(FVector(200, 0, 0), ESplineCoordinateSpace::Local);
		Actor->SplineComponent->UpdateSpline();

		Actor->RecalculateSpline();

		TArray<USplineMeshComponent*> Comps;
		Actor->GetComponents<USplineMeshComponent>(Comps);
		// Total length 200, segment 100 -> should be 2 components
		ASSERT_THAT(IsTrue(Comps.Num() == 2, "Should spawn exactly 2 components for a 200 unit spline with 100 unit segments"));
	}

	TEST_METHOD(Recalculate_Clears_Old_Components)
	{
		Actor->Mesh = DummyMesh;
		Actor->SegmentLength = 100.0f;
		Actor->SplineComponent->ClearSplinePoints();
		Actor->SplineComponent->AddSplinePoint(FVector(0, 0, 0), ESplineCoordinateSpace::Local);
		Actor->SplineComponent->AddSplinePoint(FVector(100, 0, 0), ESplineCoordinateSpace::Local);
		Actor->SplineComponent->UpdateSpline();

		Actor->RecalculateSpline();
		
		TArray<USplineMeshComponent*> CompsBefore;
		Actor->GetComponents<USplineMeshComponent>(CompsBefore);
		ASSERT_THAT(IsTrue(CompsBefore.Num() == 1, "Should have 1 component before recalculation"));

		// Recalculate again
		Actor->RecalculateSpline();

		TArray<USplineMeshComponent*> CompsAfter;
		Actor->GetComponents<USplineMeshComponent>(CompsAfter);
		ASSERT_THAT(IsTrue(CompsAfter.Num() == 1, "Should still have 1 component after recalculation (old one should be cleared)"));
	}

	TEST_METHOD(Recalculate_Applies_Collision_Enabled_Settings)
	{
		Actor->Mesh = DummyMesh;
		Actor->SegmentLength = 100.0f;
		Actor->SplineComponent->ClearSplinePoints();
		Actor->SplineComponent->AddSplinePoint(FVector(0, 0, 0), ESplineCoordinateSpace::Local);
		Actor->SplineComponent->AddSplinePoint(FVector(100, 0, 0), ESplineCoordinateSpace::Local);
		Actor->SplineComponent->UpdateSpline();

		// Default should be QueryAndPhysics
		Actor->CollisionEnabled = ECollisionEnabled::QueryAndPhysics;
		Actor->RecalculateSpline();

		TArray<USplineMeshComponent*> Comps;
		Actor->GetComponents<USplineMeshComponent>(Comps);
		ASSERT_THAT(IsTrue(Comps.Num() == 1, "Should have exactly 1 component for collision test"));
		ASSERT_THAT(IsTrue(Comps[0]->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics, "Spline mesh should inherit QueryAndPhysics collision setting from actor"));

		// Change to NoCollision
		Actor->CollisionEnabled = ECollisionEnabled::NoCollision;
		Actor->RecalculateSpline();

		Comps.Empty();
		Actor->GetComponents<USplineMeshComponent>(Comps);
		ASSERT_THAT(IsTrue(Comps.Num() == 1, "Should still have 1 component after updating collision setting"));
		ASSERT_THAT(IsTrue(Comps[0]->GetCollisionEnabled() == ECollisionEnabled::NoCollision, "Spline mesh should inherit NoCollision setting from actor after recalculation"));
	}

	TEST_METHOD(Recalculate_Applies_Physics_Material_Settings)
	{
		Actor->Mesh = DummyMesh;
		Actor->SegmentLength = 100.0f;
		Actor->SplineComponent->ClearSplinePoints();
		Actor->SplineComponent->AddSplinePoint(FVector(0, 0, 0), ESplineCoordinateSpace::Local);
		Actor->SplineComponent->AddSplinePoint(FVector(100, 0, 0), ESplineCoordinateSpace::Local);
		Actor->SplineComponent->UpdateSpline();

		UPhysicalMaterial* DummyPhysMat = NewObject<UPhysicalMaterial>(World, TEXT("DummyPhysMat"), RF_Transient);
		Actor->PhysicsMaterialOverride = DummyPhysMat;
		Actor->RecalculateSpline();

		TArray<USplineMeshComponent*> Comps;
		Actor->GetComponents<USplineMeshComponent>(Comps);
		ASSERT_THAT(IsTrue(Comps.Num() == 1, "Should have 1 component for physics material test"));
		// Note: USplineMeshComponent::GetPhysMaterialOverride() exists
		ASSERT_THAT(IsTrue(Comps[0]->BodyInstance.GetPhysMaterialOverride() == DummyPhysMat, "Spline mesh should inherit physical material override from actor"));
	}

	TEST_METHOD(Recalculate_Applies_Collision_Profile_Name)
	{
		Actor->Mesh = DummyMesh;
		Actor->SegmentLength = 100.0f;
		Actor->SplineComponent->ClearSplinePoints();
		Actor->SplineComponent->AddSplinePoint(FVector(0, 0, 0), ESplineCoordinateSpace::Local);
		Actor->SplineComponent->AddSplinePoint(FVector(100, 0, 0), ESplineCoordinateSpace::Local);
		Actor->SplineComponent->UpdateSpline();

		const FName TestProfile = TEXT("Pawn");
		Actor->CollisionProfileName = TestProfile;
		Actor->RecalculateSpline();

		TArray<USplineMeshComponent*> Comps;
		Actor->GetComponents<USplineMeshComponent>(Comps);
		ASSERT_THAT(IsTrue(Comps.Num() == 1, "Should have 1 component for collision profile test"));
		ASSERT_THAT(IsTrue(Comps[0]->GetCollisionProfileName() == TestProfile, "Spline mesh should inherit collision profile name from actor"));
	}

	TEST_METHOD(Recalculate_Applies_Mobility_Settings)
	{
		Actor->Mesh = DummyMesh;
		Actor->SegmentLength = 100.0f;
		Actor->SplineComponent->ClearSplinePoints();
		Actor->SplineComponent->AddSplinePoint(FVector(0, 0, 0), ESplineCoordinateSpace::Local);
		Actor->SplineComponent->AddSplinePoint(FVector(100, 0, 0), ESplineCoordinateSpace::Local);
		Actor->SplineComponent->UpdateSpline();

		// Test Static
		Actor->Mobility = EComponentMobility::Static;
		Actor->RecalculateSpline();

		TArray<USplineMeshComponent*> Comps;
		Actor->GetComponents<USplineMeshComponent>(Comps);
		ASSERT_THAT(IsTrue(Comps.Num() == 1, "Should have 1 component for static mobility test"));
		ASSERT_THAT(IsTrue(Comps[0]->Mobility == EComponentMobility::Static, "Spline mesh should inherit Static mobility from actor"));

		// Test Movable
		Actor->Mobility = EComponentMobility::Movable;
		Actor->RecalculateSpline();

		Comps.Empty();
		Actor->GetComponents<USplineMeshComponent>(Comps);
		ASSERT_THAT(IsTrue(Comps.Num() == 1, "Should have 1 component after updating to Movable"));
		ASSERT_THAT(IsTrue(Comps[0]->Mobility == EComponentMobility::Movable, "Spline mesh should inherit Movable mobility from actor"));
	}
};
