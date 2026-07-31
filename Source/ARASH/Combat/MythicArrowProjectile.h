#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MythicArrowProjectile.generated.h"

class UPointLightComponent;
class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;

UCLASS()
class ARASH_API AMythicArrowProjectile : public AActor
{
    GENERATED_BODY()

public:
    AMythicArrowProjectile();

    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow")
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow")
    TObjectPtr<UProjectileMovementComponent> Movement;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow")
    TObjectPtr<UStaticMeshComponent> ArrowHeadMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow")
    TObjectPtr<UPointLightComponent> GlowLight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow|Combat")
    float BaseDamage = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow|Combat", meta = (ClampMin = "0"))
    int32 MaxPierces = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow|Combat", meta = (ClampMin = "0"))
    int32 MaxBounces = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow|Combat")
    bool bReturnToOwner = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow|Combat", meta = (ClampMin = "0.05"))
    float ReturnDelay = 1.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow|Combat", meta = (ClampMin = "0.25"))
    float ReturnSpeedMultiplier = 1.0f;

    UFUNCTION(BlueprintCallable, Category = "Arrow")
    void BeginReturn();

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnArrowOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void OnProjectileBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

private:
    int32 PierceCount = 0;
    int32 BounceCount = 0;
    bool bReturning = false;
    TSet<TWeakObjectPtr<AActor>> HitActors;
};
