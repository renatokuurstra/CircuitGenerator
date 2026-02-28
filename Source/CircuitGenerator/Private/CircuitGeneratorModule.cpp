// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for license information.

#include "Modules/ModuleManager.h"

#include "CoreMinimal.h"

class FCircuitGeneratorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
        // Editor-only visualizer registration moved to CircuitGeneratorEditor module.
	}

	virtual void ShutdownModule() override
	{
        // Editor-only visualizer unregistration handled by CircuitGeneratorEditor module.
	}

private:
    // No editor state here; kept runtime-only.
};

IMPLEMENT_MODULE(FCircuitGeneratorModule, CircuitGenerator)
