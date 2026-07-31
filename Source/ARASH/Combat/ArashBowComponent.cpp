#include "Combat/ArashBowComponent.h"

#include "Combat/MythicArrowProjectile.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

UArashBowComponent::UArashBowComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    ProjectileClass = AMythicArrowProjectile::StaticClass();
}

void UArashBowComponent::StartCharge()
{
    if (!GetWorld())
    {
        return;
    }

    bCharging = true;
    ChargeStartedAt = GetWorld()->GetTimeSeconds();
}

float UArashBowComponent::GetChargeAlpha() const
{
    if (!bCharging || !GetWorld())
    {
        return 0.0f;
    }

    return FMath::Clamp((GetWorld()->GetTimeSeconds() - ChargeStartedAt) / MaxChargeTime, 0.0f, 1.0f);
}

AMythicArrowProjectile* UArashBowComponent::ReleaseArrow()
{
    AActor* Owner = GetOwner();
    UWorld* World = GetWorld();
    if (!Owner || !World || !ProjectileClass)
    {
        bCharging = false;
        return nullptr;
    }

    const float ChargeAlpha = GetChargeAlpha();
    bCharging = false;

    const FVector Forward = Owner->GetActorForwardVector();
    const FVector SpawnLocation = Owner->GetActorLocation() + Forward * 110.0f + FVector(0.0f, 0.0f, 55.0f);
    const FRotator SpawnRotation = Forward.Rotation();

    FActorSpawnParameters Params;
    Params.Owner = Owner;
    Params.Instigator = Cast<APawn>(Owner);
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AMythicArrowProjectile* Arrow = World->SpawnActor<AMythicArrowProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, Params);
    if (Arrow && Arrow->Movement)
    {
        const float SpeedScale = FMath::Lerp(0.75f, 1.25f, ChargeAlpha);
        const float DamageScale = FMath::Lerp(0.70f, 1.45f, ChargeAlpha);
        Arrow->BaseDamage *= DamageScale;
        Arrow->Movement->Velocity = Forward * Arrow->Movement->InitialSpeed * SpeedScale;
    }

    return Arrow;
}
