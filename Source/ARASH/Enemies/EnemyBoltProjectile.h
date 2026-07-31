#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyBoltProjectile.generated.h"

class UPointLightComponent;
class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;

UCLASS()
class ARASH_API AEnemyBoltProjectile : public AActor
{
    GENERATED_BODY()

public:
    AEnemyBoltProjectile();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bolt")
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bolt")
    TObjectPtr<UProjectileMovementComponent> Movement;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bolt")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bolt")
    TObjectPtr<UPointLightComponent> GlowLight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bolt")
    float Damage = 14.0f;

protected:
    virtual void BeginPlay() override;

private:
    UFUNCTION()
    void OnBoltOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void OnBoltHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);
};
