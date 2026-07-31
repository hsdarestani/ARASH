#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ArashEnemyBase.generated.h"

class UStaticMeshComponent;

UCLASS()
class ARASH_API AArashEnemyBase : public ACharacter
{
    GENERATED_BODY()

public:
    AArashEnemyBase();

    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    UFUNCTION(BlueprintCallable, Category = "Enemy")
    void ConfigureForWave(int32 WaveNumber);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Prototype")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
    float MaxHealth = 60.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
    float CurrentHealth = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
    float ChaseSpeed = 360.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
    float AttackStopDistance = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
    float ContactDamage = 18.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
    float AttackCooldown = 1.0f;

protected:
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
    void OnDamaged(float DamageAmount, float HealthRemaining);

    UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
    void OnDeath();

private:
    float LastAttackTime = -1000.0f;
    float HitFeedbackRemaining = 0.0f;
    bool bDead = false;
    FVector DefaultVisualScale = FVector(0.7f, 0.7f, 1.4f);
};
