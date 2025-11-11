// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for license information.

#include "SplineMeshCircuit.h"

#include "Components/SplineMeshComponent.h"
#include "Components/SceneComponent.h"

#if WITH_EDITOR
#include "SplineMeshCircuitVisualizer.h"
#include "ScopedTransaction.h"
#include "UObject/UnrealType.h"
#endif

#if WITH_EDITOR
int32 ASplineMeshCircuit::GetIndexForComponent(const USplineMeshComponent* InComp) const
{
	return SegmentComponents.IndexOfByKey(InComp);
}

// Helper: compute safe direction
static FVector CG_NormalizedOr(const FVector& V, const FVector& Fallback)
{
	const FVector N = V.GetSafeNormal();
	return N.IsNearlyZero() ? Fallback.GetSafeNormal() : N;
}

FSplineMeshSegmentDesc ASplineMeshCircuit::MakeDefaultSegment(const ASplineMeshCircuit& Actor, int32 NewIndex)
{
	FSplineMeshSegmentDesc Out;

	// Determine previous end state
	const bool bFirst = (NewIndex == 0);
	const FVector PrevStart = bFirst ? Actor.StartPos : (NewIndex == 1 ? Actor.StartPos : Actor.Segments[NewIndex - 2].EndPos);
	const FVector PrevEnd = bFirst ? Actor.StartPos : Actor.Segments[NewIndex - 1].EndPos;
	const FVector PrevTan = bFirst ? Actor.StartTangent : Actor.Segments[NewIndex - 1].EndTangent;
	const float PrevRoll = bFirst ? Actor.StartRoll : Actor.Segments[NewIndex - 1].EndRoll;
	const FVector2D PrevScale = bFirst ? Actor.StartScale : Actor.Segments[NewIndex - 1].EndScale;
	const ESplineMeshAxis::Type PrevAxis = bFirst ? ESplineMeshAxis::X : ESplineMeshAxis::Type(Actor.Segments[NewIndex - 1].ForwardAxis.GetValue());

	// Direction preference: previous tangent, else chord direction, else +X
	FVector Dir = CG_NormalizedOr(PrevTan, (PrevEnd - PrevStart));
	if (Dir.IsNearlyZero())
	{
		Dir = FVector::ForwardVector; // +X
	}

	const float DefaultLen = 100.f; // 100 cm extension
	Out.EndPos = PrevEnd + Dir * DefaultLen;
	Out.EndTangent = Dir * DefaultLen;
	Out.EndRoll = PrevRoll;
	Out.EndScale = PrevScale;
	Out.Mesh = nullptr; // use actor default
 Out.ForwardAxis = TEnumAsByte<ESplineMeshAxis::Type>(PrevAxis);
	return Out;
}

void ASplineMeshCircuit::AddSegment()
{
	FScopedTransaction Tx(NSLOCTEXT("CircuitGenerator", "AddSplineSegment", "Add Spline Segment"));
	Modify();
	const int32 NewIndex = Segments.Num();
	Segments.Add(MakeDefaultSegment(*this, NewIndex));
	// Optional local smoothing at the previous junction
	if (JoinSmoothing == ECircuitJoinSmoothing::Smooth && NewIndex > 0)
	{
		ApplyContinuityAtJunction(NewIndex - 1);
	}
	RebuildFromData();
}

// Reentrancy guard for PostEditChangeProperty
static bool G_CG_InPostEditChange = false;

void ASplineMeshCircuit::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (G_CG_InPostEditChange)
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);
		return;
	}
	G_CG_InPostEditChange = true;

	const FName PropName = PropertyChangedEvent.GetPropertyName();
	const EPropertyChangeType::Type ChangeType = PropertyChangedEvent.ChangeType;
	const bool bSegmentsProp = (PropName == GET_MEMBER_NAME_CHECKED(ASplineMeshCircuit, Segments));
	const bool bAffectsSegments = bSegmentsProp || (PropName == TEXT("EndPos")) || (PropName == TEXT("EndTangent")) || (PropName == TEXT("StartPos")) || (PropName == TEXT("StartTangent"));

	// Handle array add: initialize the new segment
	if (bSegmentsProp && (ChangeType == EPropertyChangeType::ArrayAdd))
	{
		const int32 AddedIndex = PropertyChangedEvent.GetArrayIndex(TEXT("Segments"));
		if (Segments.IsValidIndex(AddedIndex))
		{
			Segments[AddedIndex] = MakeDefaultSegment(*this, AddedIndex);
			// Smooth previous junction if any
			if (JoinSmoothing == ECircuitJoinSmoothing::Smooth && AddedIndex > 0)
			{
				ApplyContinuityAtJunction(AddedIndex - 1);
			}
		}
	}
	else if (bAffectsSegments && JoinSmoothing == ECircuitJoinSmoothing::Smooth)
	{
		// We don’t know exactly which index changed reliably in all cases; smooth all local junctions conservatively
		for (int32 i = 0; i + 1 < Segments.Num(); ++i)
		{
			ApplyContinuityAtJunction(i);
		}
	}

	// Rebuild whenever data changed
	if (bAffectsSegments)
	{
		RebuildFromData();
	}

	G_CG_InPostEditChange = false;
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

ASplineMeshCircuit::ASplineMeshCircuit()
{
	// Ensure we have a valid scene root for attaching generated components
	if (RootComponent == nullptr)
	{
		USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
		RootComponent = SceneRoot;
	}
}

void ASplineMeshCircuit::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Grow or create components to match Segments count
	const int32 DesiredCount = Segments.Num();
	if (SegmentComponents.Num() < DesiredCount)
	{
		SegmentComponents.Reserve(DesiredCount);
	}

	// Create/Update components
	FVector CurrStartPos = StartPos;
	FVector CurrStartTangent = StartTangent;
	float CurrStartRoll = StartRoll;
	FVector2D CurrStartScale = StartScale;

	for (int32 Index = 0; Index < DesiredCount; ++Index)
	{
		const FSplineMeshSegmentDesc& Desc = Segments[Index];

		USplineMeshComponent* Comp = nullptr;
		if (SegmentComponents.IsValidIndex(Index))
		{
			Comp = SegmentComponents[Index];
		}

		if (Comp == nullptr)
		{
			Comp = NewObject<USplineMeshComponent>(this, MakeUniqueObjectName(this, USplineMeshComponent::StaticClass(), *FString::Printf(TEXT("SplineMesh_%d"), Index)));
			Comp->SetFlags(RF_Transactional);
			Comp->SetupAttachment(RootComponent);
			Comp->RegisterComponent();

			if (SegmentComponents.IsValidIndex(Index))
			{
				SegmentComponents[Index] = Comp;
			}
			else
			{
				SegmentComponents.Add(Comp);
			}
		}
		
		// Ensure the component's local space matches the actor's data space
		// We want authored data (actor-local) to be interpreted as component-local, so reset any relative/world offsets.
		if (Comp->GetAttachParent() != RootComponent)
		{
			Comp->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		}
		Comp->SetAbsolute(false, false, false);
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetRelativeLocation(FVector::ZeroVector);
		Comp->SetRelativeRotation(FRotator::ZeroRotator);
		Comp->SetRelativeScale3D(FVector::OneVector);
		
		// Apply descriptor data (all in actor/local space)
		UStaticMesh* MeshToUse = Desc.Mesh ? Desc.Mesh : DefaultMesh;
		Comp->SetStaticMesh(MeshToUse);
		Comp->SetForwardAxis(Desc.ForwardAxis);

		const FVector EndPos = Desc.EndPos;
		const FVector EndTan = Desc.EndTangent;
		Comp->SetStartAndEnd(CurrStartPos, CurrStartTangent, EndPos, EndTan);
		Comp->SetStartRoll(CurrStartRoll);
		Comp->SetEndRoll(Desc.EndRoll);
		Comp->SetStartScale(CurrStartScale);
		Comp->SetEndScale(Desc.EndScale);

		// Next segment starts where this one ended
		CurrStartPos = EndPos;
		CurrStartTangent = EndTan;
		CurrStartRoll = Desc.EndRoll;
		CurrStartScale = Desc.EndScale;
	}

	// Remove any extra components beyond DesiredCount
	for (int32 i = SegmentComponents.Num() - 1; i >= DesiredCount; --i)
	{
		if (USplineMeshComponent* Comp = SegmentComponents[i])
		{
			Comp->DestroyComponent();
		}
		SegmentComponents.RemoveAt(i);
	}
}

#if WITH_EDITOR
void ASplineMeshCircuit::MarkSegmentDirty(int32 Index)
{
	Modify();
	if (Segments.IsValidIndex(Index))
	{
		// No body yet; reserved for editor-time updates when gizmos edit data
	}
}

void ASplineMeshCircuit::RebuildFromData()
{
	RerunConstructionScripts();
}

void ASplineMeshCircuit::ApplyContinuityAtJunction(int32 LeftIndex)
{
	if (!Segments.IsValidIndex(LeftIndex) || !Segments.IsValidIndex(LeftIndex + 1))
	{
		return;
	}

	// C0 continuity is implicit: the right segment starts at the left's EndPos.
	if (JoinSmoothing != ECircuitJoinSmoothing::Smooth)
	{
		return;
	}

	// Compute local-space chord directions before and after the junction and align the shared tangent to their bisector.
	const FVector LeftStartPos = (LeftIndex == 0) ? StartPos : Segments[LeftIndex - 1].EndPos;
	const FVector LeftEndPos = Segments[LeftIndex].EndPos;
	const FVector RightEndPos = Segments[LeftIndex + 1].EndPos;

	const FVector DirIn = (LeftEndPos - LeftStartPos).GetSafeNormal();
	const FVector DirOut = (RightEndPos - LeftEndPos).GetSafeNormal();
	FVector Bisector = DirIn + DirOut;
	if (Bisector.IsNearlyZero())
	{
		// Opposite directions: fall back to outgoing direction to avoid zero tangent
		Bisector = DirOut.IsNearlyZero() ? DirIn : DirOut;
	}
	Bisector = Bisector.GetSafeNormal();

	// Target magnitude balanced from adjacent chord lengths to keep curvature stable
	const float LenIn = (LeftEndPos - LeftStartPos).Size();
	const float LenOut = (RightEndPos - LeftEndPos).Size();
	const float TargetLen = 0.5f * (LenIn + LenOut);

	Segments[LeftIndex].EndTangent = Bisector * TargetLen;
	// Right segment's start tangent equals Left.EndTangent by construction pass.
}
#endif
