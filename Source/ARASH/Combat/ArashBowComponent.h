#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArashBowComponent.generated.h"

class AMythicArrowProjectile;

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class ARASH_API UArashBowComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UArashBowComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bow")
    TSubclassOf<AMythicArrowProjectile> ProjectileClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bow", meta = (ClampMin = "0.1"))
    float MaxChargeTime = 1.0f;

    UFUNCTION(BlueprintCallable, Category = "Bow")
    void StartCharge();

    UFUNCTION(BlueprintCallable, Category = "Bow")
    AMythicArrowProjectile* ReleaseArrow();

    UFUNCTION(BlueprintPure, Category = "Bow")
    float GetChargeAlpha() const;

    UFUNCTION(BlueprintPure, Category = "Bow")
    bool HasActiveArrow() const { return ActiveArrow.IsValid(); }

private:
    bool bCharging = false;
    float ChargeStartedAt = 0.0f;
    TWeakObjectPtr<AMythicArrowProjectile> ActiveArrow;
};
