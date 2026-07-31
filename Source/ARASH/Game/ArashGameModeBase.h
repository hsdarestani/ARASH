#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ArashGameModeBase.generated.h"

class AArashEnemyBase;
class AArashCharacter;
class AStaticMeshActor;
class UMaterialInterface;
class UStaticMesh;

UCLASS()
class ARASH_API AArashGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    AArashGameModeBase();

    UFUNCTION(BlueprintCallable, Category = "Run")
    void NotifyEnemyKilled(AArashEnemyBase* Enemy);

    UFUNCTION(BlueprintCallable, Category = "Run")
    void NotifyPlayerDied();

    UFUNCTION(BlueprintCallable, Category = "Run")
    void SelectUpgrade(int32 UpgradeIndex);

    UFUNCTION(BlueprintPure, Category = "Run")
    int32 GetCurrentWave() const { return CurrentWave; }

    UFUNCTION(BlueprintPure, Category = "Run")
    int32 GetEnemiesRemaining() const { return EnemiesRemaining; }

    UFUNCTION(BlueprintPure, Category = "Run")
    bool IsWaitingForUpgrade() const { return bWaitingForUpgrade; }

    UFUNCTION(BlueprintPure, Category = "Run")
    bool IsGameOver() const { return bGameOver; }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, Category = "Prototype")
    TSubclassOf<AArashEnemyBase> EnemyClass;

    UPROPERTY()
    TObjectPtr<UStaticMesh> PrototypeCubeMesh;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> PrototypeMaterial;

private:
    void SpawnPrototypeArena();
    void SpawnPersianCourtDetails();
    void SpawnCinematicLights();
    AStaticMeshActor* SpawnArenaBlock(const FVector& Location, const FVector& Scale,
        const FVector& Color, bool bCollision);
    void StartWave();
    void SpawnWaveEnemies(int32 WaveNumber);

    int32 CurrentWave = 1;
    int32 EnemiesRemaining = 0;
    bool bWaitingForUpgrade = false;
    bool bGameOver = false;
    FVector ArenaCenter = FVector::ZeroVector;
};
