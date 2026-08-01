#include "ARASH.h"

#include "Engine/World.h"
#include "Environment/ArashEnvironmentManager.h"
#include "Modules/ModuleManager.h"

class FArashGameModule final : public FDefaultGameModuleImpl
{
public:
    virtual void StartupModule() override
    {
        FDefaultGameModuleImpl::StartupModule();

        WorldInitHandle = FWorldDelegates::OnPostWorldInitialization.AddRaw(
            this,
            &FArashGameModule::HandlePostWorldInitialization);
    }

    virtual void ShutdownModule() override
    {
        if (WorldInitHandle.IsValid())
        {
            FWorldDelegates::OnPostWorldInitialization.Remove(WorldInitHandle);
            WorldInitHandle.Reset();
        }

        FDefaultGameModuleImpl::ShutdownModule();
    }

private:
    void HandlePostWorldInitialization(
        UWorld* World,
        const UWorld::InitializationValues InitializationValues)
    {
        if (!World || !World->IsGameWorld())
        {
            return;
        }

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Params.ObjectFlags |= RF_Transient;

        World->SpawnActor<AArashEnvironmentManager>(
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            Params);
    }

    FDelegateHandle WorldInitHandle;
};

IMPLEMENT_PRIMARY_GAME_MODULE(FArashGameModule, ARASH, "ARASH");
