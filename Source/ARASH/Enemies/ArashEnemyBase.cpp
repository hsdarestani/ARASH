#include "Enemies/ArashEnemyBase.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AArashEnemyBase::AArashEnemyBase()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AArashEnemyBase::BeginPlay()
{
    Super::BeginPlay();

    CurrentHealth = MaxHealth;
    GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;
}

void AArashEnemyBase::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (!Player)
    {
        return;
    }

    FVector Direction = Player->GetActorLocation() - GetActorLocation();
    Direction.Z = 0.0f;

    if (!Direction.IsNearlyZero())
    {
        Direction.Normalize();
        AddMovementInput(Direction, 1.0f);
        SetActorRotation(Direction.Rotation());
    }
}

float AArashEnemyBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    const float FinalDamage = Applied > 0.0f ? Applied : DamageAmount;

    CurrentHealth = FMath::Max(0.0f, CurrentHealth - FinalDamage);
    OnDamaged(FinalDamage, CurrentHealth);

    if (CurrentHealth <= 0.0f)
    {
        OnDeath();
        SetLifeSpan(0.15f);
    }

    return FinalDamage;
}
