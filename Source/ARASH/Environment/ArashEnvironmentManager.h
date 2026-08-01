#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArashEnvironmentManager.generated.h"

class AStaticMeshActor;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

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
    void TuneLighting();
    void SpawnFireLight(const FVector& Location);
    void CreateAccentMaterials();
    void ApplyMaterials(UStaticMeshComponent* MeshComponent, UStaticMesh* MeshAsset);

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

    UPROPERTY()
    TObjectPtr<UMaterialInterface> BasicShapeMaterial;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> CourtFloorMaterial;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> CourtWallMaterial;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> CourtStoneMaterial;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> CourtMetalMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> GoldMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> TurquoiseMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> CrimsonMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> EmberMaterial;

    FVector ArenaCenter = FVector::ZeroVector;
};
