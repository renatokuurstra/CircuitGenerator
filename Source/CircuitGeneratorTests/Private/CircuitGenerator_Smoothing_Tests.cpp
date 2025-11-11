// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for license information.

#include "CoreMinimal.h"
#include "CQTest.h"

#include "SplineMeshCircuit.h"

TEST_CLASS(Circuit_Generator_JunctionSmoothing, "CircuitGenerator.Smoothing")
{
	BEFORE_EACH()
	{
	}

	AFTER_EACH()
	{
	}

	TEST_METHOD(Bisector_direction_averaged_magnitude__bent_junction)
	{
		ASplineMeshCircuit* Actor = NewObject<ASplineMeshCircuit>();
		ASSERT_THAT(IsTrue(Actor != nullptr));

		// Start at origin; first segment goes along +X 100 units, second bends towards +Y
		Actor->StartPos = FVector::ZeroVector;
		Actor->StartTangent = FVector(100.f, 0.f, 0.f);
		Actor->Segments.SetNum(2);
		Actor->Segments[0].EndPos = FVector(100.f, 0.f, 0.f);
		Actor->Segments[0].EndTangent = FVector(50.f, 0.f, 0.f); // arbitrary initial
		Actor->Segments[1].EndPos = FVector(200.f, 100.f, 0.f);
		Actor->Segments[1].EndTangent = FVector(50.f, 50.f, 0.f); // arbitrary initial
		Actor->JoinSmoothing = ECircuitJoinSmoothing::Smooth;

		// Apply smoothing at junction between segment 0 and 1
		Actor->ApplyContinuityAtJunction(0);

		const FVector LeftStartPos = Actor->StartPos;
		const FVector LeftEndPos = Actor->Segments[0].EndPos;
		const FVector RightEndPos = Actor->Segments[1].EndPos;
		const FVector DirIn = (LeftEndPos - LeftStartPos).GetSafeNormal();
		const FVector DirOut = (RightEndPos - LeftEndPos).GetSafeNormal();
		FVector ExpectedDir = (DirIn + DirOut).GetSafeNormal();
		if (ExpectedDir.IsNearlyZero())
		{
			ExpectedDir = DirOut.IsNearlyZero() ? DirIn : DirOut;
		}
		const float LenIn = (LeftEndPos - LeftStartPos).Size();
		const float LenOut = (RightEndPos - LeftEndPos).Size();
		const float ExpectedLen = 0.5f * (LenIn + LenOut);

		const FVector ActualTan = Actor->Segments[0].EndTangent;
		const float ActualLen = ActualTan.Size();
		const FVector ActualDir = ActualTan.GetSafeNormal();

		// Direction within a small angular tolerance
		const float Dot = FVector::DotProduct(ExpectedDir, ActualDir);
		ASSERT_THAT(IsTrue(Dot > 0.995f)); // ~5 deg tolerance
		// Magnitude close to average (allow some tolerance for float ops)
		ASSERT_THAT(IsNear(ActualLen, ExpectedLen, 1.0f));
	}
};
