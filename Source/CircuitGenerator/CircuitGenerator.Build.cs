// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for license information.

using UnrealBuildTool;

public class CircuitGenerator : ModuleRules
{
    public CircuitGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "PhysicsCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
        });

        // Keep runtime module clean of editor-only dependencies.
        if (Target.bBuildEditor)
        {
            // Required for FScopedTransaction used in editor-only code paths (WITH_EDITOR)
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "UnrealEd"
            });
        }
    }
}
