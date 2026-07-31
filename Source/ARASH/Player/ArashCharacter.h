#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    TObjectPtr<UArashBowComponent> Bow;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Prototype")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

protected:
    virtual void BeginPlay() override;

private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void StartFire();
    void StopFire();
    void Dodge();
    void UpdateAim();
};
