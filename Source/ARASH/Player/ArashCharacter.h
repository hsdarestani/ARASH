#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "ArashCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UArashBowComponent;
class UStaticMeshComponent;

UCLASS()
class ARASH_API AArashCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AArashCharacter();

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    TObjectPtr<UArashBowComponent> Bow;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
    TObjectPtr<UStaticMeshComponent> HeadMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
    TObjectPtr<UStaticMeshComponent> MantleMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
    TObjectPtr<UStaticMeshComponent> BowUpperMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
    TObjectPtr<UStaticMeshComponent> BowLowerMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
    TObjectPtr<UStaticMeshComponent> QuiverMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival")
    float MaxHealth = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival")
    float CurrentHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival")
    float DamageInvulnerabilityTime = 0.65f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow Build")
    float ArrowBaseDamage = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow Build")
    int32 ArrowMaxPierces = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow Build")
    int32 ArrowMaxBounces = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow Build")
    float ArrowReturnSpeedMultiplier = 1.0f;

    UFUNCTION(BlueprintPure, Category = "Survival")
    float GetHealthAlpha() const;

    UFUNCTION(BlueprintPure, Category = "Survival")
    bool IsDead() const { return bIsDead; }

    UFUNCTION(BlueprintCallable, Category = "Arrow Build")
    void ApplyPierceUpgrade();

    UFUNCTION(BlueprintCallable, Category = "Arrow Build")
    void ApplyRicochetUpgrade();

    UFUNCTION(BlueprintCallable, Category = "Arrow Build")
    void ApplyReturnUpgrade();

    UFUNCTION(BlueprintCallable, Category = "Combat Feedback")
    void PlayCombatFeedback(float Strength, bool bUseHitStop);

protected:
    virtual void BeginPlay() override;

private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void StartFire();
    void StopFire();
    void Dodge();
    void UpdateAim();
    void UpdateCombatFeedback(float DeltaSeconds);
    void EndHitStop();
    void SelectUpgrade1();
    void SelectUpgrade2();
    void SelectUpgrade3();
    void RestartRun();
    bool CanUseCombat() const;

    bool bIsDead = false;
    float LastDamageTime = -1000.0f;
    float CameraFeedbackRemaining = 0.0f;
    float CameraFeedbackElapsed = 0.0f;
    float CameraFeedbackStrength = 0.0f;
    FTimerHandle HitStopTimerHandle;
};
