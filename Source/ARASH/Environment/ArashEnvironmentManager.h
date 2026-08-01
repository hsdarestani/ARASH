#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArashEnvironmentManager.generated.h"

class AStaticMeshActor;
class UStaticMesh;

UCLASS(NotBlueprintable)
class ARASH_API AArashEnvironmentManager : public AActor
{
    GENERATED_BODY()

public:
    AArashEnvironmentManager();

protected:
    virtual void BeginPlay() override;

private:
    bool HasRequiredAssets() const;
    void BuildGeneratedArena();
    void HidePrototypeGeometry();
    void SpawnFireLight(const FVector& Location);

    AStaticMeshActor* SpawnGeneratedMesh(
        UStaticMesh* MeshAsset,
        const FVector& Location,
        const FRotator& Rotation,
        bool bCollision,
        const FVector& Scale = FVector::OneVector);

    UPROPERTY()
    TObjectPtr<UStaticMesh> FloorTileMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> FloorMedallionMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> WallMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> PillarMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> BrazierMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> BannerMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> BrokenColumnMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> RubbleMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> GateMesh;

    FVector ArenaCenter = FVector::ZeroVector;
};
