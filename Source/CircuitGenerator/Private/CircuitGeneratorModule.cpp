// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for license information.

#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "SplineMeshCircuitVisualizer.h"
#include "UnrealEd.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "Components/SplineMeshComponent.h"
#endif

class FCircuitGeneratorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		if (GUnrealEd)
		{
			Visualizer = MakeShareable(new FSplineMeshCircuitComponentVisualizer());
			GUnrealEd->RegisterComponentVisualizer(USplineMeshComponent::StaticClass()->GetFName(), Visualizer);
			if (Visualizer.IsValid())
			{
				Visualizer->OnRegister();
			}
		}
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		if (GUnrealEd && Visualizer.IsValid())
		{
			GUnrealEd->UnregisterComponentVisualizer(USplineMeshComponent::StaticClass()->GetFName());
			Visualizer.Reset();
		}
#endif
	}

#if WITH_EDITOR
private:
	TSharedPtr<FSplineMeshCircuitComponentVisualizer> Visualizer;
#endif
};

IMPLEMENT_MODULE(FCircuitGeneratorModule, CircuitGenerator)
