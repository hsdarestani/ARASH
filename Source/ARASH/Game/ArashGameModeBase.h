#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ArashGameModeBase.generated.h"

class AArashEnemyBase;
class UStaticMesh;

UCLASS()
class ARASH_API AArashGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    AArashGameModeBase();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, Category = "Prototype")
    TSubclassOf<AArashEnemyBase> EnemyClass;

    UPROPERTY()
    TObjectPtr<UStaticMesh> PrototypeCubeMesh;

private:
    void SpawnPrototypeArena();
    void SpawnPrototypeEnemies();
};
