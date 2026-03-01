# Circuit Generator Plugin

A plugin for Unreal Engine 5 providing tools for procedural circuit and track generation using splines.

## Table of Contents
- [Intentions](#intentions)
- [Basic Usage](#basic-usage)
- [Public API](#public-api)
- [Important Classes](#important-classes)

## Intentions
The Circuit Generator plugin is designed to simplify the creation of tracks, roads, and other spline-based structures. It follows a data-driven approach, allowing developers to define a path and automatically populate it with mesh segments.

## Basic Usage
1. Place a **Spline Circuit Actor** in your level.
2. Adjust the **Spline Component** to define the desired path.
3. Assign a **Static Mesh** to the `Mesh` property in the Details panel.
4. Set the `Segment Length` (or leave it at 0 to use the mesh's boundary).
5. Press the **Recalculate Spline** button in the Details panel to generate the spline meshes.

## Public API

### ASplineCircuitActor
| Property | Type | Description |
| :--- | :--- | :--- |
| `Mesh` | `UStaticMesh*` | The mesh to be used for spline segments. |
| `SegmentLength` | `float` | The length of each segment. If 0, uses mesh bounds. |
| `SplineComponent` | `USplineComponent*` | The path along which meshes are spawned. |

| Function | Description |
| :--- | :--- |
| `RecalculateSpline()` | Clears existing segments and regenerates them along the spline. |

## Important Classes
- **ASplineCircuitActor**: The main actor responsible for managing the spline and its associated mesh components.
- **USplineMeshComponent**: Procedural components used to deform the mesh along the spline path.

## Notes
- **Editor Integration**: Using the `Recalculate Spline` button in the editor is fully transactional, meaning you can use the built-in Undo/Redo system.
- **Mesh Boundaries**: If `Segment Length` is set to 0, the system automatically uses the X-axis bounds of the assigned Static Mesh.
- **Spline Attributes**: The system automatically accounts for spline tangents, roll, and scaling to ensure smooth and accurate mesh deformation.
