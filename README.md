# CircuitGenerator Plugin

## Table of Contents
- Intentions
- Data Model
- Editing Workflow (Editor Only)
- Public API
- Continuity Policy
- Notes

## Intentions
Provide a lightweight, data-driven way to author a chain of `USplineMeshComponent` instances as an `AActor`. Favor simple, local-space data and small systems over complex UObject hierarchies.

## Data Model
- Actor-local Start parameters:
  - `StartPos` (FVector), `StartTangent` (FVector), `StartRoll` (float, degrees), `StartScale` (FVector2D)
- Per-segment data only defines the End of each segment via `FSplineMeshSegmentDesc`:
  - `Mesh` (optional override, defaults to `DefaultMesh` on actor)
  - `EndPos` (FVector), `EndTangent` (FVector), `EndRoll` (float), `EndScale` (FVector2D)
  - `ForwardAxis` (ESplineMeshAxis)
- Components are mirrored from `Segments` at construction.

## Editing Workflow (Editor Only)
- Click a segment mesh to reveal handles:
  - Green handle: Start of first segment (global Start)
  - Red handles: End of each segment
- Gizmo modes supported while a handle is selected:
  - Translate: moves `StartPos` or the segment `EndPos`
  - Rotate (Roll): adjusts `StartRoll` or `EndRoll` (degrees)
  - Scale: adjusts `StartScale` or `EndScale` (X = width, Y = thickness), clamped to [0.05, 10]
- While dragging, collision is temporarily disabled on the edited segment component and restored afterwards.
- Changes are wrapped in an editor transaction for undo/redo. The actor is rebuilt live via `RerunConstructionScripts()`.

## Public API
- `USTRUCT FSplineMeshSegmentDesc` with `EditAnywhere` fields described above.
- `UPROPERTY EditAnywhere` on `ASplineMeshCircuit`:
  - `DefaultMesh`, `StartPos`, `StartTangent`, `StartRoll`, `StartScale`, `Segments`, `JoinSmoothing`.
- Helpers (Editor only): `ApplyContinuityAtJunction`, `RebuildFromData`, `GetIndexForComponent`.

## Continuity Policy
- `ECircuitJoinSmoothing::None` — Only positional continuity (C0).
- `ECircuitJoinSmoothing::Smooth` — When editing, shared tangent at a junction is aligned to the bisector of incoming/outgoing chords with magnitude equal to the average of adjacent chord lengths.

## Notes
- All data is actor-local to keep transforms stable.
- Editor-only code is compiled only for editor builds and guarded with `#if WITH_EDITOR`/`Target.bBuildEditor`.
- Prefer small, stateless systems and POD-style data.
